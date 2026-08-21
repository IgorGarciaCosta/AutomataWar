#include "AWGameMode.h"
#include "AWGameState.h"
#include "AWPlayerState.h"
#include "AWPlayerController.h"
#include "AutomataWar/AI/AWAIPlanner.h"
#include "AutomataWar/Core/AutomataRules.h"
#include "AutomataWar/Core/Sim/AutomataSimulation.h"
#include "AutomataWar/Visual/AWSpectatorPawn.h"
#include "Engine/World.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogAutomataNet);

AAWGameMode::AAWGameMode()
{
    GameStateClass = AAWGameState::StaticClass();
    PlayerStateClass = AAWPlayerState::StaticClass();
    PlayerControllerClass = AAWPlayerController::StaticClass();
    DefaultPawnClass = AAWSpectatorPawn::StaticClass();
}

void AAWGameMode::InitGame(const FString &MapName, const FString &Options, FString &ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);

    bLocalMatch = (GetWorld()->GetNetMode() == NM_Standalone);
}

void AAWGameMode::PostLogin(APlayerController *NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (AAWPlayerState *PS = NewPlayer->GetPlayerState<AAWPlayerState>())
    {
        PS->CommandSlot = NextSlot;
        NextSlot = FMath::Min(NextSlot + 1, 1);
    }
}

void AAWGameMode::Logout(AController *Exiting)
{
    // Disconnect = forfeit in online mode
    if (!bLocalMatch)
    {
        if (AAWPlayerState *PS = Exiting->GetPlayerState<AAWPlayerState>())
        {
            UE_LOG(LogAutomataNet, Warning, TEXT("Player slot %d disconnected, treating as forfeit."), PS->CommandSlot);
        }

        AAWGameState *GS = GetGameState<AAWGameState>();
        if (GS && GS->Phase != EAWMatchPhase::ReplayAutopsy)
        {
            FAWResolvedRound Round = GS->ResolvedRound;
            Round.Commands0 = {EAWCommand::Wait};
            Round.Commands1 = {EAWCommand::Wait};
            Round.Outcome.bMatchEnded = true;
            Round.Outcome.WinnerSlot = (Exiting->GetPlayerState<AAWPlayerState>()->CommandSlot == 0) ? 1 : 0;
            Round.bResolved = true;
            GS->SetResolvedRound(Round);
            SetPhase(EAWMatchPhase::ReplayAutopsy);
        }
    }

    Super::Logout(Exiting);
}

void AAWGameMode::BeginLocalMatch(EAWDifficulty Difficulty, EAWArenaSize ArenaSize)
{
    bLocalMatch = true;
    bSinglePlayerMatch = false;
    SelectedArenaSize = ArenaSize;
    GetWorld()->GetTimerManager().ClearTimer(SubmissionTimerHandle);

    SubmissionState.ResetForMatch();
    PersistentRoundState = {};

    MatchArenaSeed = static_cast<uint64>(FMath::Rand()) ^ (static_cast<uint64>(FMath::Rand()) << 32);
    if (MatchArenaSeed == 0)
        MatchArenaSeed = 1;

    const FIntPoint GridSize = GetArenaGridSize(SelectedArenaSize);
    Automata::SimConfig ArenaConfig;
    ArenaConfig.gridWidth = GridSize.X;
    ArenaConfig.gridHeight = GridSize.Y;
    ArenaConfig.seed = MatchArenaSeed;
    const TArray<EAWCommand> EmptyCommands;
    Automata::Simulation ArenaGenerator;
    ArenaGenerator.RunMatch(EmptyCommands, EmptyCommands, ArenaConfig);
    PersistentRoundState = ArenaGenerator.GetFinalState();

    if (AAWGameState *GS = GetGameState<AAWGameState>())
    {
        const int32 StartingActionPoints = GetStartingActionPoints(Difficulty);
        GS->SubmissionTimeRemaining = -1.f;
        GS->SetActionPoints(0, StartingActionPoints);
        GS->SetActionPoints(1, StartingActionPoints);
        GS->SetEffects(0, {});
        GS->SetEffects(1, {});

        FAWResolvedRound Round;
        Round.RoundNumber = 1;
        Round.StartingSlot = ChooseRoundStartingSlot(
            Round.RoundNumber, GS->GetActionPoints(0), GS->GetActionPoints(1), FMath::RandRange(0, 1));
        Round.Seed = static_cast<int64>(MatchArenaSeed);
        Round.InitialActionPoints0 = StartingActionPoints;
        Round.InitialActionPoints1 = StartingActionPoints;
        const std::vector<uint8_t> EncodedState = Automata::EncodeRoundState(PersistentRoundState);
        Round.InitialArenaState.Append(EncodedState.data(), static_cast<int32>(EncodedState.size()));
        GS->SetResolvedRound(Round);
    }

    SetPhase(EAWMatchPhase::Programming);
}

void AAWGameMode::BeginSinglePlayerMatch(EAWDifficulty Difficulty, EAWArenaSize ArenaSize)
{
    BeginLocalMatch(Difficulty, ArenaSize);
    bSinglePlayerMatch = true;
    AIDifficulty = Difficulty;
}

FAWValidationResult AAWGameMode::HandleSubmission(int32 Slot, const TArray<EAWCommand> &Commands)
{
    AAWGameState *GS = GetGameState<AAWGameState>();
    if (!GS || GS->Phase != EAWMatchPhase::Programming)
    {
        FAWValidationResult R;
        R.ErrorMessage = TEXT("Submissions only accepted during Programming phase.");
        return R;
    }

    int32 CostDelta = 0;
    const FAWValidationResult SubmissionResult = SubmissionState.TrySubmit(
        Slot, Commands, Slot >= 0 && Slot <= 1 ? GS->GetActionPoints(Slot) : 0, CostDelta);
    if (!SubmissionResult.bSuccess)
        return SubmissionResult;

    GS->SetActionPoints(Slot, GS->GetActionPoints(Slot) - CostDelta);

    UE_LOG(LogAutomataGame, Log, TEXT("Slot %d submitted (%d actions)."), Slot, Commands.Num());

    if (bSinglePlayerMatch && Slot == 0 && !SubmissionState.IsSubmitted(1))
    {
        const FAWValidationResult AIResult = SubmitAICommands();
        if (!AIResult.bSuccess)
        {
            UE_LOG(LogAutomataGame, Error, TEXT("AI submission failed: %s"), *AIResult.ErrorMessage);
            return AIResult;
        }

        FAWValidationResult Result;
        Result.bSuccess = true;
        return Result;
    }

    // Check if both submitted
    if (SubmissionState.AreBothSubmitted())
    {
        OnBothSubmitted();
    }
    // Start timer on first submission in online mode
    else if (!bLocalMatch && !SubmissionTimerHandle.IsValid() && SubmissionTimerSeconds > 0.f)
    {
        GetWorld()->GetTimerManager().SetTimer(
            SubmissionTimerHandle, this, &AAWGameMode::OnSubmissionTimerExpired,
            SubmissionTimerSeconds, false);
        if (GS)
        {
            GS->SubmissionTimeRemaining = SubmissionTimerSeconds;
        }
    }

    FAWValidationResult Result;
    Result.bSuccess = true;
    return Result;
}

FAWValidationResult AAWGameMode::SubmitAICommands()
{
    AAWGameState *GS = GetGameState<AAWGameState>();
    if (!GS)
    {
        FAWValidationResult Result;
        Result.ErrorMessage = TEXT("AI planner unavailable.");
        return Result;
    }

    const int32 Seed = HashCombineFast(
        static_cast<uint32>(GS->ResolvedRound.RoundNumber), static_cast<uint32>(AIDifficulty));
    const TArray<EAWCommand> Commands = AutomataAI::GenerateCommandQueue(AIDifficulty, GS->GetActionPoints(1), Seed);
    return HandleSubmission(1, Commands);
}

FAWValidationResult AAWGameMode::WithdrawSubmission(int32 Slot)
{
    FAWValidationResult Result;
    AAWGameState *GS = GetGameState<AAWGameState>();
    if (!GS || GS->Phase != EAWMatchPhase::Programming)
    {
        Result.ErrorMessage = TEXT("Programs can only be reopened during Programming phase.");
        return Result;
    }
    Result = SubmissionState.Withdraw(Slot);
    if (!Result.bSuccess)
        return Result;
    if (!SubmissionState.IsSubmitted(0) && !SubmissionState.IsSubmitted(1))
    {
        GetWorld()->GetTimerManager().ClearTimer(SubmissionTimerHandle);
        GS->SubmissionTimeRemaining = -1.f;
    }

    Result.bSuccess = true;
    return Result;
}

void AAWGameMode::OnBothSubmitted()
{
    GetWorld()->GetTimerManager().ClearTimer(SubmissionTimerHandle);
    SetPhase(EAWMatchPhase::Submission);
    RunSimulation();
}

void AAWGameMode::OnSubmissionTimerExpired()
{
    // Use last accepted/default for non-submitted slots
    AAWGameState *GS = GetGameState<AAWGameState>();
    for (int32 i = 0; i < 2; ++i)
    {
        if (!SubmissionState.IsSubmitted(i))
        {
            int32 CostDelta = 0;
            const bool bApplyCost = SubmissionState.ExpireSlot(
                i, GS ? GS->GetActionPoints(i) : 0, CostDelta);
            if (GS && bApplyCost)
            {
                GS->SetActionPoints(i, GS->GetActionPoints(i) - CostDelta);
            }
            UE_LOG(LogAutomataNet, Log, TEXT("Slot %d timer expired, using last accepted commands."), i);
        }
    }
    OnBothSubmitted();
}

void AAWGameMode::RunSimulation()
{
    SetPhase(EAWMatchPhase::Simulation);

    uint64 Seed = MatchArenaSeed;
    if (Seed == 0)
        Seed = static_cast<uint64>(FMath::Rand()) ^ (static_cast<uint64>(FMath::Rand()) << 32);

    Automata::SimConfig Config;
    const FIntPoint GridSize = GetArenaGridSize(SelectedArenaSize);
    Config.gridWidth = GridSize.X;
    Config.gridHeight = GridSize.Y;
    Config.seed = Seed;
    Config.initialState = PersistentRoundState;

    if (Config.initialState.grid.empty())
    {
        const TArray<EAWCommand> EmptyCommands;
        Automata::Simulation ArenaGenerator;
        ArenaGenerator.RunMatch(EmptyCommands, EmptyCommands, Config);
        Config.initialState = ArenaGenerator.GetFinalState();
        PersistentRoundState = Config.initialState;
    }

    AAWGameState *GS = GetGameState<AAWGameState>();
    FAWResolvedRound Round;
    if (GS)
    {
        Round = GS->ResolvedRound;
        if (Round.StartingSlot < 0 || Round.StartingSlot > 1)
        {
            Round.StartingSlot = ChooseRoundStartingSlot(
                Round.RoundNumber, GS->GetActionPoints(0), GS->GetActionPoints(1), FMath::RandRange(0, 1));
        }
        Config.startingRobot = Round.StartingSlot;
        Config.initialActionPoints = {GS->GetActionPoints(0), GS->GetActionPoints(1)};
        Config.initialEffects = {GS->GetEffects(0), GS->GetEffects(1)};
        Round.Seed = static_cast<int64>(Seed);
        Round.InitialActionPoints0 = Config.initialActionPoints[0];
        Round.InitialActionPoints1 = Config.initialActionPoints[1];
        Round.InitialEffects0 = Config.initialEffects[0];
        Round.InitialEffects1 = Config.initialEffects[1];
        const std::vector<uint8_t> EncodedState = Automata::EncodeRoundState(Config.initialState);
        Round.InitialArenaState.Reset(static_cast<int32>(EncodedState.size()));
        if (!EncodedState.empty())
            Round.InitialArenaState.Append(EncodedState.data(), static_cast<int32>(EncodedState.size()));
        UE_LOG(LogAutomataGame, Log, TEXT("Round %d starts with slot %d."),
               Round.RoundNumber, Round.StartingSlot);
    }

    Automata::Simulation Sim;
    Automata::MatchResult Result = Sim.RunMatch(
        SubmissionState.GetCommands(0), SubmissionState.GetCommands(1), Config);
    PersistentRoundState = Sim.GetFinalState();
    uint64 FinalHash = Sim.GetFinalHash();

    // Populate GameState
    if (GS)
    {
        Round.AuthoritativeHash = static_cast<int64>(FinalHash);
        Round.Commands0 = SubmissionState.GetCommands(0);
        Round.Commands1 = SubmissionState.GetCommands(1);
        Round.Outcome.HP0 = Result.finalHP[0];
        Round.Outcome.HP1 = Result.finalHP[1];
        Round.Outcome.bMatchEnded = Result.bMatchEnded;
        Round.Outcome.EndReason = Result.endReason;
        GS->SetActionPoints(0, Result.finalActionPoints[0]);
        GS->SetActionPoints(1, Result.finalActionPoints[1]);
        GS->SetEffects(0, Result.finalEffects[0]);
        GS->SetEffects(1, Result.finalEffects[1]);
        switch (Result.outcome)
        {
        case Automata::MatchOutcome::Robot0Wins:
            Round.Outcome.WinnerSlot = 0;
            break;
        case Automata::MatchOutcome::Robot1Wins:
            Round.Outcome.WinnerSlot = 1;
            break;
        default:
            Round.Outcome.WinnerSlot = -1;
            break;
        }
        Round.bResolved = true;
        GS->SetResolvedRound(Round);
    }

    SetPhase(EAWMatchPhase::ReplayAutopsy);
}

void AAWGameMode::SetPhase(EAWMatchPhase NewPhase)
{
    AAWGameState *GS = GetGameState<AAWGameState>();
    if (!GS)
        return;

    UE_LOG(LogAutomataGame, Log, TEXT("Phase: %s -> %s"),
           *UEnum::GetValueAsString(GS->Phase), *UEnum::GetValueAsString(NewPhase));

    GS->Phase = NewPhase;
    GS->OnPhaseChanged.Broadcast(NewPhase);
}

void AAWGameMode::AdvanceToNextRound()
{
    AAWGameState *GS = GetGameState<AAWGameState>();
    if (!GS || !CanAdvanceToNextRound(
                   GS->Phase, GS->ResolvedRound.Outcome,
                   GS->ResolvedRound.Commands0, GS->ResolvedRound.Commands1))
        return;

    GS->SubmissionTimeRemaining = -1.f;
    SubmissionState.ResetForRound();

    FAWResolvedRound NextRound;
    NextRound.RoundNumber = GS->ResolvedRound.RoundNumber + 1;
    NextRound.StartingSlot = ChooseRoundStartingSlot(
        NextRound.RoundNumber, GS->GetActionPoints(0), GS->GetActionPoints(1), FMath::RandRange(0, 1));
    NextRound.Seed = static_cast<int64>(MatchArenaSeed);
    NextRound.InitialActionPoints0 = GS->GetActionPoints(0);
    NextRound.InitialActionPoints1 = GS->GetActionPoints(1);
    NextRound.InitialEffects0 = GS->GetEffects(0);
    NextRound.InitialEffects1 = GS->GetEffects(1);
    const std::vector<uint8_t> EncodedState = Automata::EncodeRoundState(PersistentRoundState);
    NextRound.InitialArenaState.Append(EncodedState.data(), static_cast<int32>(EncodedState.size()));
    GS->SetResolvedRound(NextRound);

    SetPhase(EAWMatchPhase::Programming);
}

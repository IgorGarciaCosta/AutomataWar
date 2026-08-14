#include "AWGameMode.h"
#include "AWGameState.h"
#include "AWPlayerState.h"
#include "AWPlayerController.h"
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

    AcceptedCommands[0] = {EAWCommand::Move};
    AcceptedCommands[1] = {EAWCommand::Move};
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
            // End the match as forfeit for the disconnected player
            GS->Outcome.WinnerSlot = (Exiting->GetPlayerState<AAWPlayerState>()->CommandSlot == 0) ? 1 : 0;
            SetPhase(EAWMatchPhase::ReplayAutopsy);
        }
    }

    Super::Logout(Exiting);
}

void AAWGameMode::BeginLocalMatch()
{
    bLocalMatch = true;
    GetWorld()->GetTimerManager().ClearTimer(SubmissionTimerHandle);

    AcceptedCommands[0] = {EAWCommand::Move};
    AcceptedCommands[1] = {EAWCommand::Move};
    bSlotSubmitted[0] = false;
    bSlotSubmitted[1] = false;

    if (AAWGameState *GS = GetGameState<AAWGameState>())
    {
        GS->RoundNumber = 1;
        GS->SubmissionTimeRemaining = -1.f;
        GS->RevealedCommands0.Reset();
        GS->RevealedCommands1.Reset();
        GS->AuthoritativeHash = 0;
        GS->SimSeed = 0;
        GS->Outcome = FAWMatchOutcome();
    }

    SetPhase(EAWMatchPhase::Programming);
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

    if (Slot < 0 || Slot > 1)
    {
        FAWValidationResult R;
        R.ErrorMessage = TEXT("Invalid command slot.");
        return R;
    }

    if (Commands.IsEmpty())
    {
        FAWValidationResult R;
        R.ErrorMessage = TEXT("Add at least one action before submitting.");
        return R;
    }
    if (Commands.Num() > Automata::MaxCommands)
    {
        FAWValidationResult R;
        R.ErrorMessage = FString::Printf(TEXT("A program can contain at most %d actions."), Automata::MaxCommands);
        return R;
    }
    for (EAWCommand Command : Commands)
        if (Command >= EAWCommand::Count)
        {
            FAWValidationResult R;
            R.ErrorMessage = TEXT("Command list contains an invalid action.");
            return R;
        }

    AcceptedCommands[Slot] = Commands;
    bSlotSubmitted[Slot] = true;

    UE_LOG(LogAutomataGame, Log, TEXT("Slot %d submitted (%d actions)."), Slot, Commands.Num());

    // Check if both submitted
    if (bSlotSubmitted[0] && bSlotSubmitted[1])
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

FAWValidationResult AAWGameMode::WithdrawSubmission(int32 Slot)
{
    FAWValidationResult Result;
    AAWGameState *GS = GetGameState<AAWGameState>();
    if (!GS || GS->Phase != EAWMatchPhase::Programming)
    {
        Result.ErrorMessage = TEXT("Programs can only be reopened during Programming phase.");
        return Result;
    }
    if (Slot < 0 || Slot > 1)
    {
        Result.ErrorMessage = TEXT("Invalid command slot.");
        return Result;
    }

    bSlotSubmitted[Slot] = false;
    AcceptedCommands[Slot].Reset();
    if (!bSlotSubmitted[0] && !bSlotSubmitted[1])
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
    for (int32 i = 0; i < 2; ++i)
    {
        if (!bSlotSubmitted[i])
        {
            UE_LOG(LogAutomataNet, Log, TEXT("Slot %d timer expired, using last accepted commands."), i);
            bSlotSubmitted[i] = true;
        }
    }
    OnBothSubmitted();
}

void AAWGameMode::RunSimulation()
{
    SetPhase(EAWMatchPhase::Simulation);

    uint64 Seed = static_cast<uint64>(FMath::Rand()) ^ (static_cast<uint64>(FMath::Rand()) << 32);

    Automata::SimConfig Config;
    Config.seed = Seed;

    Automata::Simulation Sim;
    Automata::MatchResult Result = Sim.RunMatch(AcceptedCommands[0], AcceptedCommands[1], Config);
    uint64 FinalHash = Sim.GetFinalHash();

    // Populate GameState
    AAWGameState *GS = GetGameState<AAWGameState>();
    if (GS)
    {
        GS->SimSeed = static_cast<int64>(Seed);
        GS->AuthoritativeHash = static_cast<int64>(FinalHash);
        GS->RevealedCommands0 = AcceptedCommands[0];
        GS->RevealedCommands1 = AcceptedCommands[1];

        GS->Outcome.HP0 = Result.finalHP[0];
        GS->Outcome.HP1 = Result.finalHP[1];
        switch (Result.outcome)
        {
        case Automata::MatchOutcome::Robot0Wins:
            GS->Outcome.WinnerSlot = 0;
            break;
        case Automata::MatchOutcome::Robot1Wins:
            GS->Outcome.WinnerSlot = 1;
            break;
        default:
            GS->Outcome.WinnerSlot = -1;
            break;
        }
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
    if (!GS || GS->Phase != EAWMatchPhase::ReplayAutopsy)
        return;

    GS->RoundNumber++;
    GS->RevealedCommands0.Reset();
    GS->RevealedCommands1.Reset();
    GS->SubmissionTimeRemaining = -1.f;
    bSlotSubmitted[0] = false;
    bSlotSubmitted[1] = false;

    SetPhase(EAWMatchPhase::Programming);
}

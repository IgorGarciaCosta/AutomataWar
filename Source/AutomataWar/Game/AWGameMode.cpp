#include "AWGameMode.h"
#include "AWGameState.h"
#include "AWPlayerState.h"
#include "AWPlayerController.h"
#include "AutomataWar/Core/Lang/AutomataCompiler.h"
#include "AutomataWar/Core/Sim/AutomataSimulation.h"
#include "Engine/World.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogAutomataNet);

AAWGameMode::AAWGameMode()
{
	GameStateClass = AAWGameState::StaticClass();
	PlayerStateClass = AAWPlayerState::StaticClass();
	PlayerControllerClass = AAWPlayerController::StaticClass();

	// Default sources to DefaultBot template
	AcceptedSource[0] = FAWExampleScripts::DefaultBot();
	AcceptedSource[1] = FAWExampleScripts::DefaultBot();
}

void AAWGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	bLocalMatch = (GetWorld()->GetNetMode() == NM_Standalone);
}

void AAWGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (AAWPlayerState* PS = NewPlayer->GetPlayerState<AAWPlayerState>())
	{
		PS->ScriptSlot = NextSlot;
		NextSlot = FMath::Min(NextSlot + 1, 1);
	}
}

void AAWGameMode::Logout(AController* Exiting)
{
	// Disconnect = forfeit in online mode
	if (!bLocalMatch)
	{
		if (AAWPlayerState* PS = Exiting->GetPlayerState<AAWPlayerState>())
		{
			UE_LOG(LogAutomataNet, Warning, TEXT("Player slot %d disconnected, treating as forfeit."), PS->ScriptSlot);
		}

		AAWGameState* GS = GetGameState<AAWGameState>();
		if (GS && GS->Phase != EAWMatchPhase::ReplayAutopsy)
		{
			// End the match as forfeit for the disconnected player
			GS->Outcome.WinnerSlot = (Exiting->GetPlayerState<AAWPlayerState>()->ScriptSlot == 0) ? 1 : 0;
			SetPhase(EAWMatchPhase::ReplayAutopsy);
		}
	}

	Super::Logout(Exiting);
}

FAWValidationResult AAWGameMode::HandleSubmission(int32 Slot, const FString& Source)
{
	AAWGameState* GS = GetGameState<AAWGameState>();
	if (!GS || GS->Phase != EAWMatchPhase::Programming)
	{
		FAWValidationResult R;
		R.ErrorMessage = TEXT("Submissions only accepted during Programming phase.");
		return R;
	}

	if (Slot < 0 || Slot > 1)
	{
		FAWValidationResult R;
		R.ErrorMessage = TEXT("Invalid script slot.");
		return R;
	}

	// Validate structure
	FAWValidationResult ValResult = FAWScriptValidator::Validate(Source);
	if (!ValResult.bSuccess)
	{
		return ValResult;
	}

	// Compile to verify correctness
	std::string StdSource = TCHAR_TO_UTF8(*Source);
	Automata::CompileResult CompResult = Automata::Compile(StdSource);
	if (!CompResult.Ok())
	{
		FAWValidationResult R;
		R.ErrorMessage = TEXT("Compilation failed: ");
		if (!CompResult.diagnostics.empty())
		{
			R.ErrorMessage += UTF8_TO_TCHAR(CompResult.diagnostics[0].message.c_str());
		}
		return R;
	}

	// Accept
	AcceptedSource[Slot] = Source;
	bSlotSubmitted[Slot] = true;

	UE_LOG(LogAutomataGame, Log, TEXT("Slot %d submitted (%d chars)."), Slot, Source.Len());

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

	ValResult.bSuccess = true;
	ValResult.ErrorMessage.Empty();
	return ValResult;
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
			UE_LOG(LogAutomataNet, Log, TEXT("Slot %d timer expired, using last accepted source."), i);
			bSlotSubmitted[i] = true;
		}
	}
	OnBothSubmitted();
}

void AAWGameMode::RunSimulation()
{
	SetPhase(EAWMatchPhase::Simulation);

	std::string Src0 = TCHAR_TO_UTF8(*AcceptedSource[0]);
	std::string Src1 = TCHAR_TO_UTF8(*AcceptedSource[1]);

	Automata::CompileResult C0 = Automata::Compile(Src0);
	Automata::CompileResult C1 = Automata::Compile(Src1);

	// Both already validated; this should not fail
	check(C0.Ok() && C1.Ok());

	// Generate seed
	uint64 Seed = static_cast<uint64>(FMath::Rand()) ^ (static_cast<uint64>(FMath::Rand()) << 32);

	Automata::SimConfig Config;
	Config.seed = Seed;

	Automata::Simulation Sim;
	Automata::MatchResult Result = Sim.RunMatch(C0.program, C1.program, Config);
	uint64 FinalHash = Sim.GetFinalHash();

	// Populate GameState
	AAWGameState* GS = GetGameState<AAWGameState>();
	if (GS)
	{
		GS->SimSeed = static_cast<int64>(Seed);
		GS->AuthoritativeHash = static_cast<int64>(FinalHash);
		GS->RevealedSource0 = AcceptedSource[0];
		GS->RevealedSource1 = AcceptedSource[1];

		GS->Outcome.FinalTick = Result.finalTick;
		GS->Outcome.HP0 = Result.finalHP[0];
		GS->Outcome.HP1 = Result.finalHP[1];
		switch (Result.outcome)
		{
		case Automata::MatchOutcome::Robot0Wins: GS->Outcome.WinnerSlot = 0; break;
		case Automata::MatchOutcome::Robot1Wins: GS->Outcome.WinnerSlot = 1; break;
		default: GS->Outcome.WinnerSlot = -1; break;
		}
	}

	SetPhase(EAWMatchPhase::ReplayAutopsy);
}

void AAWGameMode::SetPhase(EAWMatchPhase NewPhase)
{
	AAWGameState* GS = GetGameState<AAWGameState>();
	if (!GS) return;

	UE_LOG(LogAutomataGame, Log, TEXT("Phase: %s -> %s"),
		*UEnum::GetValueAsString(GS->Phase), *UEnum::GetValueAsString(NewPhase));

	GS->Phase = NewPhase;
	GS->OnPhaseChanged.Broadcast(NewPhase);
}

void AAWGameMode::AdvanceToNextRound()
{
	AAWGameState* GS = GetGameState<AAWGameState>();
	if (!GS || GS->Phase != EAWMatchPhase::ReplayAutopsy) return;

	GS->RoundNumber++;
	GS->RevealedSource0.Empty();
	GS->RevealedSource1.Empty();
	GS->SubmissionTimeRemaining = -1.f;
	bSlotSubmitted[0] = false;
	bSlotSubmitted[1] = false;

	SetPhase(EAWMatchPhase::Programming);
}

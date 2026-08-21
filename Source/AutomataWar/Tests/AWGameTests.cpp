/**
 * @file AWGameTests.cpp
 * @brief Focused game-layer storage and command-label tests.
 */

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "AutomataWar/Core/AutomataRules.h"
#include "AutomataWar/Core/Replay/AutomataReplay.h"
#include "AutomataWar/AI/AWAIPlanner.h"
#include "AutomataWar/Game/AWReplayService.h"
#include "AutomataWar/Game/AWSubmissionState.h"
#include "AutomataWar/Net/AWDesyncDetector.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReplayFilenameSafety, "AutomataWar.Game.Replay.FilenameSafety",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReplayFilenameSafety::RunTest(const FString &Parameters)
{
    const FString Safe = FAWReplayService::SanitizeFilename(TEXT("../../my<replay>?"));
    TestEqual(TEXT("Only safe filename characters remain"), Safe, FString(TEXT("myreplay")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReplayMaximumPayloadSmall, "AutomataWar.Game.Replay.MaximumPayloadSmall",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReplayMaximumPayloadSmall::RunTest(const FString &Parameters)
{
    Automata::ReplayData Replay;
    Replay.commandsA.Init(EAWCommand::Move, Automata::MaxCommands);
    Replay.commandsB.Init(EAWCommand::Fire, Automata::MaxCommands);
    TestTrue(TEXT("Maximum replay stays below 1 KiB"), Automata::EncodeReplay(Replay).size() < 1024);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommandLabels, "AutomataWar.Game.Commands.Labels",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommandLabels::RunTest(const FString &Parameters)
{
    TestEqual(TEXT("Move label"), FString(LexToString(EAWCommand::Move)), FString(TEXT("MOVE")));
    TestEqual(TEXT("Fire label"), FString(LexToString(EAWCommand::Fire)), FString(TEXT("FIRE")));
    TestEqual(TEXT("Left label"), FString(LexToString(EAWCommand::TurnLeft)), FString(TEXT("TURN LEFT")));
    TestEqual(TEXT("Right label"), FString(LexToString(EAWCommand::TurnRight)), FString(TEXT("TURN RIGHT")));
    TestEqual(TEXT("Wait label"), FString(LexToString(EAWCommand::Wait)), FString(TEXT("WAIT")));
    TestEqual(TEXT("Shield label"), FString(LexToString(EAWCommand::ChargeShield)), FString(TEXT("CHARGE SHIELD")));
    TestEqual(TEXT("Accelerate label"), FString(LexToString(EAWCommand::Accelerate)), FString(TEXT("ACCELERATE")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatchDifficultyBudgets, "AutomataWar.Game.Match.DifficultyBudgets",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** Verify every setup preset maps to its intended symmetric AP budget. */
bool FMatchDifficultyBudgets::RunTest(const FString &Parameters)
{
    TestEqual(TEXT("Easy starts at 150 AP"), GetStartingActionPoints(EAWDifficulty::Easy), 150);
    TestEqual(TEXT("Normal preserves the 100 AP baseline"), GetStartingActionPoints(EAWDifficulty::Normal), 100);
    TestEqual(TEXT("Hard starts at 75 AP"), GetStartingActionPoints(EAWDifficulty::Hard), 75);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAICommandQueues, "AutomataWar.Game.AI.CommandQueues",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAICommandQueues::RunTest(const FString &Parameters)
{
    const TArray<EAWCommand> Easy = AutomataAI::GenerateCommandQueue(EAWDifficulty::Easy, 100, 7);
    const TArray<EAWCommand> Normal = AutomataAI::GenerateCommandQueue(EAWDifficulty::Normal, 100, 7);
    const TArray<EAWCommand> Hard = AutomataAI::GenerateCommandQueue(EAWDifficulty::Hard, 100, 7);

    TestTrue(TEXT("Every difficulty produces commands"), !Easy.IsEmpty() && !Normal.IsEmpty() && !Hard.IsEmpty());
    TestTrue(TEXT("Difficulty increases planning depth"), Easy.Num() < Normal.Num() && Normal.Num() < Hard.Num());
    TestTrue(TEXT("Easy queue respects AP"), GetProgramActionPointCost(Easy) <= 100);
    TestTrue(TEXT("Normal queue respects AP"), GetProgramActionPointCost(Normal) <= 100);
    TestTrue(TEXT("Hard queue respects AP"), GetProgramActionPointCost(Hard) <= 100);
    TestEqual(TEXT("Zero AP still yields a valid wait"),
              AutomataAI::GenerateCommandQueue(EAWDifficulty::Hard, 0, 7)[0], EAWCommand::Wait);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSubmissionStateFlow, "AutomataWar.Game.Submission.StateFlow",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** Verify queue replacement, withdrawal, timeout, and round-reset bookkeeping. */
bool FSubmissionStateFlow::RunTest(const FString &Parameters)
{
    FAWSubmissionState State;
    int32 CostDelta = 0;

    const TArray<EAWCommand> FirstQueue = {EAWCommand::Move, EAWCommand::Fire};
    TestTrue(TEXT("Initial queue is accepted"), State.TrySubmit(0, FirstQueue, 100, CostDelta).bSuccess);
    TestEqual(TEXT("Initial queue reserves its full cost"), CostDelta, 30);
    TestTrue(TEXT("Submitted slot is locked"), State.IsSubmitted(0));

    const TArray<EAWCommand> ReplacementQueue = {EAWCommand::TurnLeft};
    TestTrue(TEXT("Replacement queue is accepted"), State.TrySubmit(0, ReplacementQueue, 70, CostDelta).bSuccess);
    TestEqual(TEXT("Cheaper replacement refunds only the difference"), CostDelta, -25);
    TestTrue(TEXT("Withdrawal reopens the slot"), State.Withdraw(0).bSuccess);
    TestFalse(TEXT("Withdrawn slot is no longer submitted"), State.IsSubmitted(0));
    TestEqual(TEXT("Withdrawal retains the accepted queue for timeout"),
              State.GetCommands(0)[0], EAWCommand::TurnLeft);

    TestTrue(TEXT("Retained queue can finalize at timeout"), State.ExpireSlot(0, 95, CostDelta));
    TestEqual(TEXT("Retained queue requires no additional reservation"), CostDelta, 0);
    TestTrue(TEXT("Timed-out slot becomes submitted"), State.IsSubmitted(0));

    State.ResetForRound();
    TestFalse(TEXT("Round reset unlocks slot zero"), State.IsSubmitted(0));
    TestFalse(TEXT("Round reset unlocks slot one"), State.IsSubmitted(1));
    TestTrue(TEXT("Round reset installs valid wait fallbacks"),
             State.GetCommands(0) == TArray<EAWCommand>{EAWCommand::Wait} &&
                 State.GetCommands(1) == TArray<EAWCommand>{EAWCommand::Wait});
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FResolvedRoundPayload, "AutomataWar.Game.Replay.ResolvedRoundPayload",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** Verify the reflected aggregate preserves every replay input at the Core boundary. */
bool FResolvedRoundPayload::RunTest(const FString &Parameters)
{
    FAWResolvedRound Round;
    Round.RoundNumber = 4;
    Round.StartingSlot = 1;
    Round.Seed = 987654321;
    Round.InitialActionPoints0 = 45;
    Round.InitialActionPoints1 = 60;
    Round.InitialEffects0.bShieldCharged = true;
    Round.InitialEffects1.ExtraAmmoRounds = 2;
    Round.InitialArenaState = {1, 2, 3, 4};
    Round.Commands0 = {EAWCommand::Move, EAWCommand::Fire};
    Round.Commands1 = {EAWCommand::Wait};
    Round.bResolved = true;

    TestTrue(TEXT("Complete aggregate is replay-ready"), Round.IsReadyForReplay());
    const Automata::ReplayData Data = MakeReplayData(Round);
    const FAWResolvedRound Restored = MakeResolvedRound(Data);
    TestEqual(TEXT("Starting slot survives conversion"), Restored.StartingSlot, Round.StartingSlot);
    TestEqual(TEXT("Seed survives conversion"), Restored.Seed, Round.Seed);
    TestEqual(TEXT("Player zero AP survives conversion"),
              Restored.InitialActionPoints0, Round.InitialActionPoints0);
    TestEqual(TEXT("Player one AP survives conversion"),
              Restored.InitialActionPoints1, Round.InitialActionPoints1);
    TestTrue(TEXT("Player zero effects survive conversion"), Restored.InitialEffects0.bShieldCharged);
    TestEqual(TEXT("Player one effects survive conversion"), Restored.InitialEffects1.ExtraAmmoRounds, 2);
    TestEqual(TEXT("Arena state survives conversion"), Restored.InitialArenaState, Round.InitialArenaState);
    TestEqual(TEXT("Player zero commands survive conversion"), Restored.Commands0, Round.Commands0);
    TestEqual(TEXT("Player one commands survive conversion"), Restored.Commands1, Round.Commands1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDesyncDetectorMismatch, "AutomataWar.Game.Net.DesyncMismatch",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** Verify deterministic verification accepts the authority hash and rejects a changed hash. */
bool FDesyncDetectorMismatch::RunTest(const FString &Parameters)
{
    const TArray<EAWCommand> Commands0 = {EAWCommand::Move, EAWCommand::Wait};
    const TArray<EAWCommand> Commands1 = {EAWCommand::TurnLeft, EAWCommand::Wait};
    Automata::SimConfig Config;
    Config.seed = 2468;
    Automata::Simulation Simulation;
    Simulation.RunMatch(Commands0, Commands1, Config);
    const uint64 AuthorityHash = Simulation.GetFinalHash();

    TestTrue(TEXT("Matching authoritative hash passes"),
             FAWDesyncDetector::VerifyMatch(Commands0, Commands1, Config, AuthorityHash));
    AddExpectedError(TEXT("DESYNC DETECTED"), EAutomationExpectedErrorFlags::Contains, 1);
    TestFalse(TEXT("Changed authoritative hash is rejected"),
              FAWDesyncDetector::VerifyMatch(Commands0, Commands1, Config, AuthorityHash ^ 1));
    return true;
}

#endif
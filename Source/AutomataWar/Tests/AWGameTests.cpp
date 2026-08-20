/**
 * @file AWGameTests.cpp
 * @brief Focused game-layer storage and command-label tests.
 */

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "AutomataWar/Core/AutomataRules.h"
#include "AutomataWar/Core/Replay/AutomataReplay.h"
#include "AutomataWar/AI/AWAIController.h"
#include "AutomataWar/Game/AWReplayService.h"

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
    const TArray<EAWCommand> Easy = AAWAIController::GenerateCommandQueue(EAWDifficulty::Easy, 100, 7);
    const TArray<EAWCommand> Normal = AAWAIController::GenerateCommandQueue(EAWDifficulty::Normal, 100, 7);
    const TArray<EAWCommand> Hard = AAWAIController::GenerateCommandQueue(EAWDifficulty::Hard, 100, 7);

    TestTrue(TEXT("Every difficulty produces commands"), !Easy.IsEmpty() && !Normal.IsEmpty() && !Hard.IsEmpty());
    TestTrue(TEXT("Difficulty increases planning depth"), Easy.Num() < Normal.Num() && Normal.Num() < Hard.Num());
    TestTrue(TEXT("Easy queue respects AP"), GetProgramActionPointCost(Easy) <= 100);
    TestTrue(TEXT("Normal queue respects AP"), GetProgramActionPointCost(Normal) <= 100);
    TestTrue(TEXT("Hard queue respects AP"), GetProgramActionPointCost(Hard) <= 100);
    TestEqual(TEXT("Zero AP still yields a valid wait"),
              AAWAIController::GenerateCommandQueue(EAWDifficulty::Hard, 0, 7)[0], EAWCommand::Wait);
    return true;
}

#endif
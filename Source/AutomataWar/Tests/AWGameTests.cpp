/**
 * @file AWGameTests.cpp
 * @brief Focused game-layer storage and command-label tests.
 */

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "AutomataWar/Core/AutomataRules.h"
#include "AutomataWar/Core/Replay/AutomataReplay.h"
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
    return true;
}

#endif
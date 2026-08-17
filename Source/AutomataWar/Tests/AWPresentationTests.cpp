/**
 * @file AWPresentationTests.cpp
 * @brief Focused replay-navigation and HUD contract tests.
 */

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "AutomataWar/Core/Replay/AWReplayController.h"
#include "AutomataWar/UI/AWHUDWidget.h"
#include "AutomataWar/UI/AWScreenWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommandCount, "AutomataWar.UI.Commands.CountIsSeven",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommandCount::RunTest(const FString &Parameters)
{
    TestEqual(TEXT("Exactly seven selectable commands"), static_cast<int32>(EAWCommand::Count), 7);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReplayControllerSteps, "AutomataWar.UI.Replay.ControllerSteps",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReplayControllerSteps::RunTest(const FString &Parameters)
{
    const TArray<EAWCommand> CommandsA = {
        EAWCommand::Move, EAWCommand::TurnLeft, EAWCommand::Move, EAWCommand::Fire};
    const TArray<EAWCommand> CommandsB = {EAWCommand::TurnRight, EAWCommand::Move};

    Automata::FAWReplayController Controller;
    TestTrue(TEXT("Controller initializes"), Controller.Initialize(CommandsA, CommandsB, 12345));
    TestEqual(TEXT("Starts at first step"), Controller.GetCurrentStep(), 0);
    TestEqual(TEXT("Every queued command receives a replay step"), Controller.GetTotalSteps(), 6);
    Controller.StepForward();
    TestEqual(TEXT("Steps forward"), Controller.GetCurrentStep(), 1);
    Controller.SeekToStep(3);
    Controller.StepBackward();
    TestEqual(TEXT("Steps backward"), Controller.GetCurrentStep(), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHUDScreenCount, "AutomataWar.UI.HUD.ScreenCountIs7",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHUDScreenCount::RunTest(const FString &Parameters)
{
    TestEqual(TEXT("EAWScreen count"), static_cast<int32>(EAWScreen::LanguageReference) + 1, 7);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatchResultLabels, "AutomataWar.UI.MatchResult.Labels",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** Verify personal and shared-screen match result wording. */
bool FMatchResultLabels::RunTest(const FString &Parameters)
{
    TestEqual(TEXT("Viewer win"), UAWMatchResultPopupWidget::FormatResultText(0, 0, false).ToString(), FString(TEXT("YOU WON")));
    TestEqual(TEXT("Viewer loss"), UAWMatchResultPopupWidget::FormatResultText(1, 0, false).ToString(), FString(TEXT("YOU LOSE")));
    TestEqual(TEXT("Local P1 win"), UAWMatchResultPopupWidget::FormatResultText(0, 0, true).ToString(), FString(TEXT("P1 WON")));
    TestEqual(TEXT("Local P2 win"), UAWMatchResultPopupWidget::FormatResultText(1, 0, true).ToString(), FString(TEXT("P2 WON")));
    return true;
}

#endif
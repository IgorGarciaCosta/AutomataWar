/**
 * @file AWPresentationTests.cpp
 * @brief Focused replay-navigation and HUD contract tests.
 */

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "AutomataWar/Core/Replay/AWReplayController.h"
#include "AutomataWar/UI/AWHUDWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommandCount, "AutomataWar.UI.Commands.CountIsFour",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommandCount::RunTest(const FString &Parameters)
{
    TestEqual(TEXT("Exactly four selectable commands"), static_cast<int32>(EAWCommand::Count), 4);
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
    TestEqual(TEXT("Longer list determines step count"), Controller.GetTotalSteps(), 4);
    Controller.StepForward();
    TestEqual(TEXT("Steps forward"), Controller.GetCurrentStep(), 1);
    Controller.SeekToStep(3);
    Controller.StepBackward();
    TestEqual(TEXT("Steps backward"), Controller.GetCurrentStep(), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHUDScreenCount, "AutomataWar.UI.HUD.ScreenCountIs6",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHUDScreenCount::RunTest(const FString &Parameters)
{
    TestEqual(TEXT("EAWScreen count"), static_cast<int32>(EAWScreen::LanguageReference) + 1, 6);
    return true;
}

#endif
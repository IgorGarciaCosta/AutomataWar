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
#include "AutomataWar/Visual/AWVisualTypes.h"
#include "Materials/Material.h"
#include "NiagaraSystem.h"
#include "UObject/MetaData.h"
#include "UObject/Package.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHUDScreenCount, "AutomataWar.UI.HUD.ScreenCountIs6",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHUDScreenCount::RunTest(const FString &Parameters)
{
    TestEqual(TEXT("EAWScreen count"), static_cast<int32>(EAWScreen::LanguageReference) + 1, 6);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNextRoundAvailability, "AutomataWar.UI.Replay.NextRoundAvailability",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** Verify Next Round is exposed only after both queues finish a non-terminal round. */
bool FNextRoundAvailability::RunTest(const FString &Parameters)
{
    const TArray<EAWCommand> Commands = {EAWCommand::Wait};
    FAWMatchOutcome Outcome;

    TestFalse(TEXT("Planning phase cannot advance"),
              CanAdvanceToNextRound(EAWMatchPhase::Programming, Outcome, Commands, Commands));
    TestFalse(TEXT("Replay without both revealed queues cannot advance"),
              CanAdvanceToNextRound(EAWMatchPhase::ReplayAutopsy, Outcome, Commands, {}));
    TestTrue(TEXT("Completed non-terminal replay can advance"),
             CanAdvanceToNextRound(EAWMatchPhase::ReplayAutopsy, Outcome, Commands, Commands));
    Outcome.bMatchEnded = true;
    TestFalse(TEXT("Terminal match cannot advance"),
              CanAdvanceToNextRound(EAWMatchPhase::ReplayAutopsy, Outcome, Commands, Commands));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatchResultTiming, "AutomataWar.UI.MatchResult.AfterReplay",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** Verify terminal results stay hidden until the completed round replay has finished. */
bool FMatchResultTiming::RunTest(const FString &Parameters)
{
    FAWMatchOutcome Outcome;
    Outcome.bMatchEnded = true;

    TestFalse(TEXT("Programming cannot reveal a result"),
              CanRevealMatchResult(EAWMatchPhase::Programming, Outcome, true));
    TestFalse(TEXT("Simulation cannot reveal a result"),
              CanRevealMatchResult(EAWMatchPhase::Simulation, Outcome, true));
    TestFalse(TEXT("Replay must finish before revealing a result"),
              CanRevealMatchResult(EAWMatchPhase::ReplayAutopsy, Outcome, false));
    TestTrue(TEXT("Completed terminal replay reveals its result"),
             CanRevealMatchResult(EAWMatchPhase::ReplayAutopsy, Outcome, true));
    Outcome.bMatchEnded = false;
    TestFalse(TEXT("Non-terminal replay has no result popup"),
              CanRevealMatchResult(EAWMatchPhase::ReplayAutopsy, Outcome, true));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatchResultLabels, "AutomataWar.UI.MatchResult.Labels",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** Verify the Content-authored messages for every result reason and viewpoint. */
bool FMatchResultLabels::RunTest(const FString &Parameters)
{
    const UEnum *MessageEnum = LoadObject<UEnum>(nullptr,
                                                 TEXT("/Game/UI/Data/E_MatchResultMessage.E_MatchResultMessage"));
    TestNotNull(TEXT("Match result message enum exists in Content"), MessageEnum);
    if (!MessageEnum)
        return false;

    TestEqual(TEXT("Enum owns exactly five messages"), MessageEnum->NumEnums() - 1, 5);
    TestEqual(TEXT("Solo HP win"), UAWMatchResultPopupWidget::FormatResultText(0, 0, EAWMatchEndReason::Health, false, MessageEnum).ToString(),
              FString(TEXT("You crushed your opponent")));
    TestEqual(TEXT("Solo AP win"), UAWMatchResultPopupWidget::FormatResultText(0, 0, EAWMatchEndReason::ActionPoints, false, MessageEnum).ToString(),
              FString(TEXT("That loser get no munny")));
    TestEqual(TEXT("Solo HP loss"), UAWMatchResultPopupWidget::FormatResultText(1, 0, EAWMatchEndReason::Health, false, MessageEnum).ToString(),
              FString(TEXT("Wasted, crushed, massacrated")));
    TestEqual(TEXT("Solo AP loss"), UAWMatchResultPopupWidget::FormatResultText(1, 0, EAWMatchEndReason::ActionPoints, false, MessageEnum).ToString(),
              FString(TEXT("No more moves for you, bro")));
    TestEqual(TEXT("Local P1 viewpoint"), UAWMatchResultPopupWidget::FormatResultText(0, 0, EAWMatchEndReason::Health, true, MessageEnum).ToString(),
              FString(TEXT("P1: You crushed your opponent")));
    TestEqual(TEXT("Local P2 viewpoint"), UAWMatchResultPopupWidget::FormatResultText(0, 1, EAWMatchEndReason::Health, true, MessageEnum).ToString(),
              FString(TEXT("P2: Wasted, crushed, massacrated")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVFXAssetContract, "AutomataWar.Visual.VFX.AssetContract",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** Verify production VFX assets and the hard lifetime bound for looping muzzle systems. */
bool FVFXAssetContract::RunTest(const FString &Parameters)
{
    UNiagaraSystem *Muzzle = LoadObject<UNiagaraSystem>(nullptr, AWVisualAssets::NS_MuzzleFlash);
    UNiagaraSystem *Impact = LoadObject<UNiagaraSystem>(nullptr, AWVisualAssets::NS_Impact);
    UMaterial *ShieldMaterial = LoadObject<UMaterial>(nullptr, AWVisualAssets::M_ShieldEnergy);
    TestNotNull(TEXT("Muzzle system exists"), Muzzle);
    TestNotNull(TEXT("Impact system exists"), Impact);
    TestNotNull(TEXT("Shield energy material exists"), ShieldMaterial);
    if (!Muzzle || !Impact || !ShieldMaterial)
        return false;

    AddInfo(FString::Printf(TEXT("Muzzle source IsLooping=%s; runtime lifespan cap=%.2fs"),
                            Muzzle->IsLooping() ? TEXT("true") : TEXT("false"),
                            AWVisualConfig::MuzzleFlashLifespan));
    TestTrue(TEXT("A looping muzzle source is forcibly bounded to one short flash"),
             !Muzzle->IsLooping() || AWVisualConfig::MuzzleFlashLifespan <= 0.2f);
    TestFalse(TEXT("Impact explosion and smoke system is finite"), Impact->IsLooping());
    TestTrue(TEXT("Impact smoke remains visible after projectile arrival"),
             AWVisualConfig::ImpactVFXLifespan >= 1.f);

    UMetaData *MetaData = Impact->GetOutermost()->GetMetaData();
    TestEqual(TEXT("Impact uses the SimpleExplosion source"),
              FString(MetaData->GetValue(Impact, TEXT("AutomataWar.SourcePackage"))),
              FString(TEXT("/Niagara/DefaultAssets/Templates/Systems/SimpleExplosion.SimpleExplosion")));
    TestTrue(TEXT("Shield shell is translucent"), ShieldMaterial->GetBlendMode() == BLEND_Translucent);
    TestTrue(TEXT("Shield shell is unlit energy"),
             ShieldMaterial->GetShadingModels().HasShadingModel(MSM_Unlit));
    TestTrue(TEXT("Shield shell renders from inside and outside"), ShieldMaterial->IsTwoSided());
    return true;
}

#endif
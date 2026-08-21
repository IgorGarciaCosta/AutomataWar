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
#include "AutomataWar/UI/AWTypewriterTextBlock.h"
#include "AutomataWar/Visual/AWPlanVisualizationSubsystem.h"
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

    Automata::ReplayData Data;
    Data.seed = 12345;
    Data.commandsA = CommandsA;
    Data.commandsB = CommandsB;
    Automata::FAWReplayController Controller;
    TestTrue(TEXT("Controller initializes"), Controller.Initialize(Data));
    TestEqual(TEXT("Starts at first step"), Controller.GetCurrentStep(), 0);
    TestEqual(TEXT("Every queued command receives a replay step"), Controller.GetTotalSteps(), 6);
    Controller.StepForward();
    TestEqual(TEXT("Steps forward"), Controller.GetCurrentStep(), 1);
    Controller.SeekToStep(3);
    Controller.StepBackward();
    TestEqual(TEXT("Steps backward"), Controller.GetCurrentStep(), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlanProjectionSteps, "AutomataWar.Visual.PlanProjection.StepsMatchQueue",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlanProjectionSteps::RunTest(const FString &Parameters)
{
    Automata::SimConfig Config;
    Config.gridWidth = 5;
    Config.gridHeight = 5;
    Config.initialState.gridWidth = Config.gridWidth;
    Config.initialState.gridHeight = Config.gridHeight;
    Config.initialState.grid.assign(25, Automata::CellType::Empty);
    Config.initialState.obstacleHealth.assign(25, 0);
    Config.initialState.actionPointItemValues.assign(25, 0);
    Config.initialState.robotX = {1, 3};
    Config.initialState.robotY = {1, 3};
    Config.initialState.robotFacing = {Automata::Dir::East, Automata::Dir::North};

    const TArray<EAWCommand> Commands = {
        EAWCommand::Move, EAWCommand::TurnRight, EAWCommand::Move,
        EAWCommand::Fire, EAWCommand::ChargeShield};
    Automata::RobotState InitialRobot;
    TArray<Automata::StepSnapshot> Snapshots;
    TArray<Automata::SimEvent> Events;

    TestTrue(TEXT("Projection builds from round-start state"),
             UAWPlanVisualizationSubsystem::BuildProjection(
                 0, Commands, Config, InitialRobot, Snapshots, Events));
    TestEqual(TEXT("Projection starts at the replicated tank X"), InitialRobot.x, 1);
    TestEqual(TEXT("Projection starts at the replicated tank Y"), InitialRobot.y, 1);
    TestEqual(TEXT("Every queued command receives a projection step"), Snapshots.Num(), Commands.Num());
    if (Snapshots.Num() == Commands.Num())
    {
        const Automata::RobotState &FinalRobot = Snapshots.Last().robots[0];
        TestEqual(TEXT("Projected movement reaches the expected X"), FinalRobot.x, 2);
        TestEqual(TEXT("Projected movement reaches the expected Y"), FinalRobot.y, 2);
        TestTrue(TEXT("Projected charge is cosmetic state only"), FinalRobot.effects.bShieldCharged);
    }

    const TArray<EAWCommand> ShortenedCommands = {
        EAWCommand::Move, EAWCommand::TurnRight, EAWCommand::Move};
    TestTrue(TEXT("Projection rebuilds after removing commands"),
             UAWPlanVisualizationSubsystem::BuildProjection(
                 0, ShortenedCommands, Config, InitialRobot, Snapshots, Events));
    TestEqual(TEXT("Rollback removes projection steps"), Snapshots.Num(), ShortenedCommands.Num());
    TestFalse(TEXT("Rollback removes charged shield state"),
              Snapshots.IsEmpty() ? true : Snapshots.Last().robots[0].effects.bShieldCharged);
    TestFalse(TEXT("Rollback removes firing cosmetics"),
              Events.ContainsByPredicate([](const Automata::SimEvent &Event)
                                         { return Event.type == Automata::EventType::Fire; }));

    Config.startingRobot = 0;
    const TArray<EAWCommand> PlayerTwoCommands = {EAWCommand::TurnLeft, EAWCommand::Move};
    TestTrue(TEXT("Slot 2 projection builds when slot 1 owns initiative"),
             UAWPlanVisualizationSubsystem::BuildProjection(
                 1, PlayerTwoCommands, Config, InitialRobot, Snapshots, Events));
    TestEqual(TEXT("Empty opponent queue adds no projection steps"),
              Snapshots.Num(), PlayerTwoCommands.Num());
    TestFalse(TEXT("Every slot 2 snapshot belongs to slot 2"),
              Snapshots.ContainsByPredicate([](const Automata::StepSnapshot &Snapshot)
                                            { return Snapshot.robots[1].currentCommand == INDEX_NONE; }));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHUDScreenCount, "AutomataWar.UI.HUD.ScreenCountIs6",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHUDScreenCount::RunTest(const FString &Parameters)
{
    TestEqual(TEXT("EAWScreen count"), static_cast<int32>(EAWScreen::LanguageReference) + 1, 6);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTypewriterFrames, "AutomataWar.UI.Typewriter.Frames",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** Verify title frames reveal characters while preserving the target's line layout. */
bool FTypewriterFrames::RunTest(const FString &Parameters)
{
    const FString Title = TEXT("AB\nC");
    TestEqual(TEXT("Initial frame"), UAWTypewriterTextBlock::FormatFrame(Title, 0, TEXT("|")),
              FString(TEXT("|  \n ")));
    TestEqual(TEXT("Mid-animation frame"), UAWTypewriterTextBlock::FormatFrame(Title, 2, TEXT("|")),
              FString(TEXT("AB|\n ")));
    TestEqual(TEXT("Completed frame"), UAWTypewriterTextBlock::FormatFrame(Title, Title.Len(), TEXT("|")),
              FString(TEXT("AB\nC|")));
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
              FString(TEXT("Wasted, crushed, massacred")));
    TestEqual(TEXT("Solo AP loss"), UAWMatchResultPopupWidget::FormatResultText(1, 0, EAWMatchEndReason::ActionPoints, false, MessageEnum).ToString(),
              FString(TEXT("No more moves for you, bro")));
    TestEqual(TEXT("Local P1 viewpoint"), UAWMatchResultPopupWidget::FormatResultText(0, 0, EAWMatchEndReason::Health, true, MessageEnum).ToString(),
              FString(TEXT("P1: You crushed your opponent")));
    TestEqual(TEXT("Local P2 viewpoint"), UAWMatchResultPopupWidget::FormatResultText(0, 1, EAWMatchEndReason::Health, true, MessageEnum).ToString(),
              FString(TEXT("P2: Wasted, crushed, massacred")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVFXAssetContract, "AutomataWar.Visual.VFX.AssetContract",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** Verify production VFX assets and the hard lifetime bound for looping muzzle systems. */
bool FVFXAssetContract::RunTest(const FString &Parameters)
{
    UNiagaraSystem *Muzzle = LoadObject<UNiagaraSystem>(nullptr, AWVisualAssets::NS_MuzzleFlash);
    UNiagaraSystem *Impact = LoadObject<UNiagaraSystem>(nullptr, AWVisualAssets::NS_Impact);
    UMaterial *EffectMaterial = LoadObject<UMaterial>(nullptr, AWVisualAssets::M_Effect);
    UMaterial *ShieldMaterial = LoadObject<UMaterial>(nullptr, AWVisualAssets::M_ShieldEnergy);
    TestNotNull(TEXT("Muzzle system exists"), Muzzle);
    TestNotNull(TEXT("Impact system exists"), Impact);
    TestNotNull(TEXT("Plan hologram material exists"), EffectMaterial);
    TestNotNull(TEXT("Shield energy material exists"), ShieldMaterial);
    if (!Muzzle || !Impact || !EffectMaterial || !ShieldMaterial)
        return false;

    AddInfo(FString::Printf(TEXT("Muzzle source IsLooping=%s; runtime lifespan cap=%.2fs"),
                            Muzzle->IsLooping() ? TEXT("true") : TEXT("false"),
                            AWVisualConfig::MuzzleFlashLifespan));
    TestTrue(TEXT("A looping muzzle source is forcibly bounded to one short flash"),
             !Muzzle->IsLooping() || AWVisualConfig::MuzzleFlashLifespan <= 0.2f);
    TestFalse(TEXT("Impact explosion and smoke system is finite"), Impact->IsLooping());
    TestTrue(TEXT("Impact smoke remains visible after projectile arrival"),
             AWVisualConfig::ImpactVFXLifespan >= 1.f);
    TestTrue(TEXT("Plan hologram supports the tank's skeletal cannon"),
             EffectMaterial->GetUsageByFlag(MATUSAGE_SkeletalMesh));

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
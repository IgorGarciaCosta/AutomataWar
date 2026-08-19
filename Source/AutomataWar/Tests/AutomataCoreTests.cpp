/**
 * @file AutomataCoreTests.cpp
 * @brief Focused tests for finite command execution and replay encoding.
 */

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "AutomataWar/Core/Replay/AutomataReplay.h"
#include "AutomataWar/Core/Sim/AutomataSimulation.h"
#include <algorithm>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommandsRunOnce, "AutomataWar.Core.Commands.RunOnce",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommandsRunOnce::RunTest(const FString &Parameters)
{
    Automata::Simulation Simulation;
    const TArray<EAWCommand> CommandsA = {EAWCommand::Move, EAWCommand::TurnLeft, EAWCommand::Fire};
    const TArray<EAWCommand> CommandsB = {EAWCommand::TurnRight};
    const Automata::MatchResult Result = Simulation.RunMatch(CommandsA, CommandsB);

    TestEqual(TEXT("Round length includes both command lists"), Result.stepsExecuted, 4);
    TestEqual(TEXT("One snapshot per tank command"), static_cast<int32>(Simulation.GetSnapshots().size()), 4);
    for (const Automata::StepSnapshot &Snapshot : Simulation.GetSnapshots())
    {
        const int32 ActiveTankCount = (Snapshot.robots[0].currentCommand >= 0 ? 1 : 0) +
                                      (Snapshot.robots[1].currentCommand >= 0 ? 1 : 0);
        TestEqual(TEXT("Exactly one tank acts in each snapshot"), ActiveTankCount, 1);
    }
    TestEqual(TEXT("Starter runs command 1 first"), Simulation.GetSnapshots()[0].robots[0].currentCommand, 0);
    TestEqual(TEXT("Starter runs command 2 without yielding"), Simulation.GetSnapshots()[1].robots[0].currentCommand, 1);
    TestEqual(TEXT("Starter finishes its queue before yielding"), Simulation.GetSnapshots()[2].robots[0].currentCommand, 2);
    TestEqual(TEXT("Opponent runs only after the starter queue ends"), Simulation.GetSnapshots()[3].robots[1].currentCommand, 0);
    const Automata::StepSnapshot &Final = Simulation.GetSnapshots().back();
    TestEqual(TEXT("P1 consumes every command once"), Final.robots[0].nextCommand, 3);
    TestEqual(TEXT("P2 does not wrap its short list"), Final.robots[1].nextCommand, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FResourceDepletionEndsMatch, "AutomataWar.Core.Match.ResourceDepletion",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** Verify end-of-round AP affordability, HP defeat, and simultaneous AP tiebreaks. */
bool FResourceDepletionEndsMatch::RunTest(const FString &Parameters)
{
    Automata::SimConfig Config;
    Config.initialActionPoints = {GetMinimumPositiveActionPointCost() - 1,
                                  GetMinimumPositiveActionPointCost()};

    Automata::Simulation Simulation;
    const Automata::MatchResult Result = Simulation.RunMatch({EAWCommand::Wait}, {EAWCommand::Wait}, Config);

    TestEqual(TEXT("Minimum positive command cost is derived from commands"),
              GetMinimumPositiveActionPointCost(), Automata::TurnActionPointCost);
    TestTrue(TEXT("AP below the cheapest paid command ends the match"), Result.bMatchEnded);
    TestEqual(TEXT("AP defeat is checked after both queues finish"), Result.stepsExecuted, 2);
    TestTrue(TEXT("The tank that can still buy a paid command wins"),
             Result.outcome == Automata::MatchOutcome::Robot1Wins);
    TestTrue(TEXT("AP defeat retains its reason"),
             Result.endReason == EAWMatchEndReason::ActionPoints);

    Automata::SimConfig TieBreakConfig;
    TieBreakConfig.gridWidth = 3;
    TieBreakConfig.gridHeight = 4;
    TieBreakConfig.initialActionPoints = {0, 0};
    Automata::Simulation TieBreakSimulation;
    const Automata::MatchResult TieBreakResult = TieBreakSimulation.RunMatch(
        {EAWCommand::Fire}, {EAWCommand::Wait}, TieBreakConfig);

    TestTrue(TEXT("Both tanks without affordable paid commands end the match"), TieBreakResult.bMatchEnded);
    TestEqual(TEXT("Both command queues finish before the AP tiebreak"), TieBreakResult.stepsExecuted, 2);
    TestTrue(TEXT("Higher HP wins simultaneous AP depletion"),
             TieBreakResult.outcome == Automata::MatchOutcome::Robot0Wins);
    TestTrue(TEXT("The simultaneous AP winner is reported as an HP tiebreak"),
             TieBreakResult.endReason == EAWMatchEndReason::Health);

    Automata::SimConfig HealthConfig;
    HealthConfig.gridWidth = 3;
    HealthConfig.gridHeight = 4;
    TArray<EAWCommand> LethalCommands;
    LethalCommands.Init(EAWCommand::Fire,
                        FMath::DivideAndRoundUp(Automata::MaxHP, Automata::ProjectileDamage));

    Automata::Simulation HealthSimulation;
    const Automata::MatchResult HealthResult = HealthSimulation.RunMatch(
        LethalCommands, {EAWCommand::Wait}, HealthConfig);

    TestTrue(TEXT("Zero HP ends the match"), HealthResult.bMatchEnded);
    TestTrue(TEXT("The defeated tank reaches zero HP"), HealthResult.finalHP[1] <= 0);
    TestTrue(TEXT("The tank with HP remaining wins"),
             HealthResult.outcome == Automata::MatchOutcome::Robot0Wins);
    TestTrue(TEXT("HP defeat retains its reason"),
             HealthResult.endReason == EAWMatchEndReason::Health);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTurnInitiative, "AutomataWar.Core.Turns.Initiative",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTurnInitiative::RunTest(const FString &Parameters)
{
    TestEqual(TEXT("Round one uses the random slot even when AP differs"),
              ChooseRoundStartingSlot(1, 200, 10, 1), 1);
    TestEqual(TEXT("Later round starts with higher P1 AP"),
              ChooseRoundStartingSlot(2, 80, 20, 1), 0);
    TestEqual(TEXT("Later round starts with higher P2 AP"),
              ChooseRoundStartingSlot(3, 10, 40, 0), 1);
    TestEqual(TEXT("Equal AP uses the supplied random tie-break"),
              ChooseRoundStartingSlot(4, 30, 30, 1), 1);

    Automata::SimConfig Config;
    Config.startingRobot = 1;
    Automata::Simulation Simulation;
    Simulation.RunMatch({EAWCommand::Wait, EAWCommand::Wait},
                        {EAWCommand::Wait, EAWCommand::Wait}, Config);
    const std::vector<Automata::StepSnapshot> &Snapshots = Simulation.GetSnapshots();
    TestEqual(TEXT("Selected starter acts first"), Snapshots[0].robots[1].currentCommand, 0);
    TestEqual(TEXT("Starter finishes its queue before yielding"), Snapshots[1].robots[1].currentCommand, 1);
    TestEqual(TEXT("Other tank starts after the starter finishes"), Snapshots[2].robots[0].currentCommand, 0);
    TestEqual(TEXT("Other tank then finishes its queue"), Snapshots[3].robots[0].currentCommand, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommandsTurnRelativeToTank, "AutomataWar.Core.Commands.TurnRelativeToTank",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommandsTurnRelativeToTank::RunTest(const FString &Parameters)
{
    Automata::SimConfig Config;
    Config.gridWidth = 4;
    Config.gridHeight = 4;

    Automata::Simulation Simulation;
    Simulation.RunMatch({EAWCommand::TurnLeft, EAWCommand::Move}, {EAWCommand::TurnRight}, Config);
    const Automata::StepSnapshot &Final = Simulation.GetSnapshots().back();

    TestEqual(TEXT("Left from south faces east"), Final.robots[0].facing, Automata::Dir::East);
    TestEqual(TEXT("Move follows the new facing"), Final.robots[0].x, 2);
    TestEqual(TEXT("Right from north faces east"), Final.robots[1].facing, Automata::Dir::East);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommandsFireForward, "AutomataWar.Core.Commands.FireForward",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommandsFireForward::RunTest(const FString &Parameters)
{
    Automata::SimConfig Config;
    Config.gridWidth = 3;
    Config.gridHeight = 4;

    Automata::Simulation Simulation;
    const Automata::MatchResult Result = Simulation.RunMatch(
        {EAWCommand::Fire}, {EAWCommand::TurnRight}, Config);

    TestEqual(TEXT("Fire hits the adjacent tank ahead"), Result.finalHP[1], Automata::MaxHP - Automata::ProjectileDamage);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommandsDeterministic, "AutomataWar.Core.Commands.Deterministic",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommandsDeterministic::RunTest(const FString &Parameters)
{
    const TArray<EAWCommand> CommandsA = {EAWCommand::Move, EAWCommand::Move, EAWCommand::Fire};
    const TArray<EAWCommand> CommandsB = {EAWCommand::TurnLeft, EAWCommand::Move, EAWCommand::Fire};
    Automata::SimConfig Config;
    Config.seed = 987654;

    Automata::Simulation First;
    Automata::Simulation Second;
    First.RunMatch(CommandsA, CommandsB, Config);
    Second.RunMatch(CommandsA, CommandsB, Config);
    TestEqual(TEXT("Same commands and seed produce the same hash"), First.GetFinalHash(), Second.GetFinalHash());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoundStatePersists, "AutomataWar.Core.Rounds.StatePersists",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRoundStatePersists::RunTest(const FString &Parameters)
{
    Automata::SimConfig FirstConfig;
    FirstConfig.gridWidth = 4;
    FirstConfig.gridHeight = 4;

    Automata::Simulation First;
    First.RunMatch({EAWCommand::TurnLeft, EAWCommand::Move}, {EAWCommand::Wait}, FirstConfig);

    const std::vector<uint8_t> EncodedState = Automata::EncodeRoundState(First.GetFinalState());
    Automata::RoundState RestoredState;
    TestTrue(TEXT("Final round state decodes"),
             Automata::DecodeRoundState(EncodedState.data(), EncodedState.size(), RestoredState));

    Automata::SimConfig SecondConfig = FirstConfig;
    SecondConfig.initialState = MoveTemp(RestoredState);
    Automata::Simulation Second;
    Second.RunMatch({EAWCommand::Wait}, {EAWCommand::Wait}, SecondConfig);

    const Automata::StepSnapshot &FirstFinal = First.GetSnapshots().back();
    const Automata::StepSnapshot &SecondStart = Second.GetSnapshots().front();
    for (int32 RobotIndex = 0; RobotIndex < 2; ++RobotIndex)
    {
        TestEqual(TEXT("Tank X carries into the next round"), SecondStart.robots[RobotIndex].x, FirstFinal.robots[RobotIndex].x);
        TestEqual(TEXT("Tank Y carries into the next round"), SecondStart.robots[RobotIndex].y, FirstFinal.robots[RobotIndex].y);
        TestEqual(TEXT("Tank facing carries into the next round"), SecondStart.robots[RobotIndex].facing, FirstFinal.robots[RobotIndex].facing);
    }
    TestTrue(TEXT("The first round collected an item"), First.GetGrid() != First.GetFinalState().grid);
    TestTrue(TEXT("Collected items remain absent in the next round"), Second.GetGrid() == First.GetFinalState().grid);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActionPointRules, "AutomataWar.Core.ActionPoints.Rules",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FActionPointRules::RunTest(const FString &Parameters)
{
    TestEqual(TEXT("Compact arena is half the default dimensions"),
              GetArenaGridSize(EAWArenaSize::Compact),
              FIntPoint(Automata::CompactGridWidth, Automata::CompactGridHeight));
    TestEqual(TEXT("Standard arena retains the default dimensions"),
              GetArenaGridSize(EAWArenaSize::Standard),
              FIntPoint(Automata::DefaultGridWidth, Automata::DefaultGridHeight));
    TestEqual(TEXT("Expanded arena is twice the default dimensions"),
              GetArenaGridSize(EAWArenaSize::Expanded),
              FIntPoint(Automata::ExpandedGridWidth, Automata::ExpandedGridHeight));
    TestEqual(TEXT("Move costs 10 AP"), GetActionPointCost(EAWCommand::Move), 10);
    TestEqual(TEXT("Fire costs 20 AP"), GetActionPointCost(EAWCommand::Fire), 20);
    TestEqual(TEXT("Turns cost 5 AP"), GetActionPointCost(EAWCommand::TurnLeft), 5);
    TestEqual(TEXT("Wait costs no AP"), GetActionPointCost(EAWCommand::Wait), 0);
    TestEqual(TEXT("Charge shield costs 20 AP"), GetActionPointCost(EAWCommand::ChargeShield), 20);
    TestEqual(TEXT("Accelerate costs 30 AP"), GetActionPointCost(EAWCommand::Accelerate), 30);

    Automata::SimConfig Config;
    Config.gridWidth = 4;
    Config.gridHeight = 4;
    Config.seed = 314159;
    Config.initialActionPoints = {17, 23};

    Automata::Simulation First;
    Automata::Simulation Second;
    const Automata::MatchResult FirstResult = First.RunMatch({EAWCommand::Move}, {EAWCommand::Fire}, Config);
    const Automata::MatchResult SecondResult = Second.RunMatch({EAWCommand::Move}, {EAWCommand::Fire}, Config);

    TestTrue(TEXT("Moving onto the only free south cell awards at least 10 AP"), FirstResult.finalActionPoints[0] >= 27);
    TestTrue(TEXT("A pickup awards no more than 20 AP"), FirstResult.finalActionPoints[0] <= 37);
    TestEqual(TEXT("Pickup awards are deterministic"), FirstResult.finalActionPoints[0], SecondResult.finalActionPoints[0]);

    Automata::Simulation StandardBoard;
    StandardBoard.RunMatch({EAWCommand::Move}, {EAWCommand::Move});
    int32 PickupCount = 0;
    int32 ExtraAmmoCount = 0;
    int32 ShieldCount = 0;
    int32 AcceleratorCount = 0;
    const std::vector<Automata::CellType> &Grid = StandardBoard.GetGrid();
    for (Automata::CellType Cell : Grid)
    {
        PickupCount += Cell == Automata::CellType::ActionPointItem ? 1 : 0;
        ExtraAmmoCount += Cell == Automata::CellType::ExtraAmmoItem ? 1 : 0;
        ShieldCount += Cell == Automata::CellType::ShieldItem ? 1 : 0;
        AcceleratorCount += Cell == Automata::CellType::AcceleratorItem ? 1 : 0;
    }
    TestEqual(TEXT("Standard board spawns exactly 12 AP items"), PickupCount, Automata::ActionPointItemCount);
    TestEqual(TEXT("Standard board spawns extra ammo"), ExtraAmmoCount, Automata::ExtraAmmoItemCount);
    TestEqual(TEXT("Standard board spawns temporary shields"), ShieldCount, Automata::ShieldItemCount);
    TestEqual(TEXT("Standard board spawns accelerators"), AcceleratorCount, Automata::AcceleratorItemCount);
    TestEqual(TEXT("P1 start is never occupied"), Grid[Automata::DefaultGridWidth + 1], Automata::CellType::Empty);
    TestEqual(TEXT("P2 start is never occupied"), Grid[(Automata::DefaultGridHeight - 2) * Automata::DefaultGridWidth + Automata::DefaultGridWidth - 2], Automata::CellType::Empty);

    auto CountArenaContent = [](int32 Width, int32 Height, Automata::CellType Type)
    {
        Automata::SimConfig ArenaConfig;
        ArenaConfig.gridWidth = Width;
        ArenaConfig.gridHeight = Height;
        ArenaConfig.seed = 314159;
        Automata::Simulation ArenaSimulation;
        ArenaSimulation.RunMatch({EAWCommand::Wait}, {EAWCommand::Wait}, ArenaConfig);
        return static_cast<int32>(std::count(ArenaSimulation.GetGrid().begin(), ArenaSimulation.GetGrid().end(), Type));
    };
    const int32 CompactItems = CountArenaContent(Automata::CompactGridWidth, Automata::CompactGridHeight,
                                                 Automata::CellType::ActionPointItem);
    const int32 ExpandedItems = CountArenaContent(Automata::ExpandedGridWidth, Automata::ExpandedGridHeight,
                                                  Automata::CellType::ActionPointItem);
    const int32 CompactCover = CountArenaContent(Automata::CompactGridWidth, Automata::CompactGridHeight,
                                                 Automata::CellType::Cover);
    const int32 StandardCover = CountArenaContent(Automata::DefaultGridWidth, Automata::DefaultGridHeight,
                                                  Automata::CellType::Cover);
    const int32 ExpandedCover = CountArenaContent(Automata::ExpandedGridWidth, Automata::ExpandedGridHeight,
                                                  Automata::CellType::Cover);
    TestEqual(TEXT("Compact board scales AP items down"), CompactItems, 3);
    TestEqual(TEXT("Expanded board scales AP items up"), ExpandedItems, 48);
    TestTrue(TEXT("Cover count grows with arena size"), CompactCover < StandardCover && StandardCover < ExpandedCover);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommandEffects, "AutomataWar.Core.Commands.Effects",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommandEffects::RunTest(const FString &Parameters)
{
    Automata::SimConfig Config;
    Config.gridWidth = 5;
    Config.gridHeight = 5;

    Automata::Simulation Accelerated;
    Accelerated.RunMatch({EAWCommand::Accelerate, EAWCommand::Move}, {EAWCommand::Wait, EAWCommand::Wait}, Config);
    TestEqual(TEXT("Accelerate makes the next move cross two cells"), Accelerated.GetSnapshots().back().robots[0].y, 3);
    TestFalse(TEXT("Accelerate is consumed by that move"), Accelerated.GetSnapshots().back().robots[0].effects.bAccelerateNextMove);

    Config.gridWidth = 3;
    Config.gridHeight = 4;
    Automata::Simulation Shielded;
    const Automata::MatchResult ShieldedResult = Shielded.RunMatch(
        {EAWCommand::ChargeShield}, {EAWCommand::Fire}, Config);
    TestEqual(TEXT("Charged shield halves the next hit"), ShieldedResult.finalHP[0],
              Automata::MaxHP - Automata::ProjectileDamage / 2);
    TestFalse(TEXT("Charged shield is consumed by damage"), ShieldedResult.finalEffects[0].bShieldCharged);
    TestFalse(TEXT("Consumed one-shot shield is no longer visually active"),
              HasActiveShield(ShieldedResult.finalEffects[0]));

    Automata::SimConfig TimedShieldConfig = Config;
    TimedShieldConfig.initialEffects[0].ShieldRounds = 2;
    Automata::Simulation TimedShield;
    const Automata::MatchResult TimedShieldResult = TimedShield.RunMatch(
        {EAWCommand::Wait}, {EAWCommand::Fire}, TimedShieldConfig);
    TestTrue(TEXT("Timed shield remains visually active after absorbing a hit"),
             HasActiveShield(TimedShieldResult.finalEffects[0]));

    Config.initialEffects[0].ExtraAmmoRounds = 1;
    Automata::Simulation ExtraAmmo;
    const Automata::MatchResult ExtraAmmoResult = ExtraAmmo.RunMatch(
        {EAWCommand::Fire}, {EAWCommand::Wait}, Config);
    TestEqual(TEXT("Extra ammo increases shot damage"), ExtraAmmoResult.finalHP[1],
              Automata::MaxHP - Automata::ProjectileDamage - Automata::ExtraAmmoDamageBonus);
    TestEqual(TEXT("Timed power-up expires after its remaining round"), ExtraAmmoResult.finalEffects[0].ExtraAmmoRounds, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommandReplayRoundTrip, "AutomataWar.Core.Replay.CommandRoundTrip",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommandReplayRoundTrip::RunTest(const FString &Parameters)
{
    Automata::ReplayData Data;
    Data.seed = 42;
    Data.startingRobot = 1;
    Data.initialActionPointsA = 37;
    Data.initialActionPointsB = 81;
    Data.initialEffectsA.bShieldCharged = true;
    Data.initialEffectsA.ExtraAmmoRounds = 2;
    Data.initialEffectsB.bAccelerateNextMove = true;
    Data.initialEffectsB.AcceleratorRounds = 1;
    Automata::Simulation StateSource;
    StateSource.RunMatch({EAWCommand::Move}, {EAWCommand::Wait});
    Data.initialState = Automata::EncodeRoundState(StateSource.GetFinalState());
    Data.commandsA = {EAWCommand::Move, EAWCommand::Fire};
    Data.commandsB = {EAWCommand::TurnLeft, EAWCommand::TurnRight};

    const std::vector<uint8_t> Encoded = Automata::EncodeReplay(Data);
    const Automata::ReplayDecodeResult Decoded = Automata::DecodeReplay(Encoded);
    TestTrue(TEXT("Replay decodes"), Decoded.Ok());
    TestEqual(TEXT("Seed survives"), Decoded.data.seed, Data.seed);
    TestEqual(TEXT("Starting robot survives"), Decoded.data.startingRobot, 1);
    TestEqual(TEXT("P1 initial AP survives"), Decoded.data.initialActionPointsA, Data.initialActionPointsA);
    TestEqual(TEXT("P2 initial AP survives"), Decoded.data.initialActionPointsB, Data.initialActionPointsB);
    TestTrue(TEXT("P1 charged shield survives"), Decoded.data.initialEffectsA.bShieldCharged);
    TestEqual(TEXT("P1 extra ammo duration survives"), Decoded.data.initialEffectsA.ExtraAmmoRounds, 2);
    TestTrue(TEXT("P2 charged acceleration survives"), Decoded.data.initialEffectsB.bAccelerateNextMove);
    TestEqual(TEXT("P2 accelerator duration survives"), Decoded.data.initialEffectsB.AcceleratorRounds, 1);
    TestTrue(TEXT("Initial arena state survives"), Decoded.data.initialState == Data.initialState);
    TestTrue(TEXT("P1 commands survive"), Decoded.data.commandsA == Data.commandsA);
    TestTrue(TEXT("P2 commands survive"), Decoded.data.commandsB == Data.commandsB);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommandReplayRejectsInvalidAction, "AutomataWar.Core.Replay.RejectsInvalidAction",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommandReplayRejectsInvalidAction::RunTest(const FString &Parameters)
{
    Automata::ReplayData Data;
    Data.commandsA = {EAWCommand::Move};
    Data.commandsB = {EAWCommand::Fire};
    std::vector<uint8_t> Encoded = Automata::EncodeReplay(Data);
    Encoded[43] = 255;

    const Automata::ReplayDecodeResult Decoded = Automata::DecodeReplay(Encoded);
    TestEqual(TEXT("Invalid enum byte is rejected"), Decoded.error, Automata::ReplayError::InvalidCommands);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommandReplayRejectsInvalidStarter, "AutomataWar.Core.Replay.RejectsInvalidStarter",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommandReplayRejectsInvalidStarter::RunTest(const FString &Parameters)
{
    Automata::ReplayData Data;
    Data.commandsA = {EAWCommand::Wait};
    Data.commandsB = {EAWCommand::Wait};
    std::vector<uint8_t> Encoded = Automata::EncodeReplay(Data);
    Encoded[38] = 2;

    const Automata::ReplayDecodeResult Decoded = Automata::DecodeReplay(Encoded);
    TestEqual(TEXT("Invalid starting slot is rejected"), Decoded.error, Automata::ReplayError::InvalidStartingRobot);
    return true;
}

#endif
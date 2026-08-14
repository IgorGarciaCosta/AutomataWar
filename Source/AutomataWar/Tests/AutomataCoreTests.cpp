/**
 * @file AutomataCoreTests.cpp
 * @brief Focused tests for finite command execution and replay encoding.
 */

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "AutomataWar/Core/Replay/AutomataReplay.h"
#include "AutomataWar/Core/Sim/AutomataSimulation.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommandsRunOnce, "AutomataWar.Core.Commands.RunOnce",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommandsRunOnce::RunTest(const FString &Parameters)
{
    Automata::Simulation Simulation;
    const TArray<EAWCommand> CommandsA = {EAWCommand::Move, EAWCommand::TurnLeft, EAWCommand::Fire};
    const TArray<EAWCommand> CommandsB = {EAWCommand::TurnRight};
    const Automata::MatchResult Result = Simulation.RunMatch(CommandsA, CommandsB);

    TestEqual(TEXT("Round length follows the longer list"), Result.stepsExecuted, 3);
    TestEqual(TEXT("One snapshot per command step"), static_cast<int32>(Simulation.GetSnapshots().size()), 3);
    const Automata::StepSnapshot &Final = Simulation.GetSnapshots().back();
    TestEqual(TEXT("P1 consumes every command once"), Final.robots[0].nextCommand, 3);
    TestEqual(TEXT("P2 does not wrap its short list"), Final.robots[1].nextCommand, 1);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActionPointRules, "AutomataWar.Core.ActionPoints.Rules",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FActionPointRules::RunTest(const FString &Parameters)
{
    TestEqual(TEXT("Move costs 10 AP"), GetActionPointCost(EAWCommand::Move), 10);
    TestEqual(TEXT("Fire costs 20 AP"), GetActionPointCost(EAWCommand::Fire), 20);
    TestEqual(TEXT("Turns cost 5 AP"), GetActionPointCost(EAWCommand::TurnLeft), 5);

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
    const std::vector<Automata::CellType> &Grid = StandardBoard.GetGrid();
    for (Automata::CellType Cell : Grid)
        PickupCount += Cell == Automata::CellType::ActionPointItem ? 1 : 0;
    TestEqual(TEXT("Standard board spawns exactly 12 AP items"), PickupCount, Automata::ActionPointItemCount);
    TestEqual(TEXT("P1 start is never occupied"), Grid[Automata::DefaultGridWidth + 1], Automata::CellType::Empty);
    TestEqual(TEXT("P2 start is never occupied"), Grid[(Automata::DefaultGridHeight - 2) * Automata::DefaultGridWidth +
                                                       Automata::DefaultGridWidth - 2], Automata::CellType::Empty);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommandReplayRoundTrip, "AutomataWar.Core.Replay.CommandRoundTrip",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommandReplayRoundTrip::RunTest(const FString &Parameters)
{
    Automata::ReplayData Data;
    Data.seed = 42;
    Data.initialActionPointsA = 37;
    Data.initialActionPointsB = 81;
    Data.commandsA = {EAWCommand::Move, EAWCommand::Fire};
    Data.commandsB = {EAWCommand::TurnLeft, EAWCommand::TurnRight};

    const std::vector<uint8_t> Encoded = Automata::EncodeReplay(Data);
    const Automata::ReplayDecodeResult Decoded = Automata::DecodeReplay(Encoded);
    TestTrue(TEXT("Replay decodes"), Decoded.Ok());
    TestEqual(TEXT("Seed survives"), Decoded.data.seed, Data.seed);
    TestEqual(TEXT("P1 initial AP survives"), Decoded.data.initialActionPointsA, Data.initialActionPointsA);
    TestEqual(TEXT("P2 initial AP survives"), Decoded.data.initialActionPointsB, Data.initialActionPointsB);
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
    Encoded[32] = 255;

    const Automata::ReplayDecodeResult Decoded = Automata::DecodeReplay(Encoded);
    TestEqual(TEXT("Invalid enum byte is rejected"), Decoded.error, Automata::ReplayError::InvalidCommands);
    return true;
}

#endif
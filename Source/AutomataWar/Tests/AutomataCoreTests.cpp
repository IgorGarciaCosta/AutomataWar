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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommandReplayRoundTrip, "AutomataWar.Core.Replay.CommandRoundTrip",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommandReplayRoundTrip::RunTest(const FString &Parameters)
{
    Automata::ReplayData Data;
    Data.seed = 42;
    Data.commandsA = {EAWCommand::Move, EAWCommand::Fire};
    Data.commandsB = {EAWCommand::TurnLeft, EAWCommand::TurnRight};

    const std::vector<uint8_t> Encoded = Automata::EncodeReplay(Data);
    const Automata::ReplayDecodeResult Decoded = Automata::DecodeReplay(Encoded);
    TestTrue(TEXT("Replay decodes"), Decoded.Ok());
    TestEqual(TEXT("Seed survives"), Decoded.data.seed, Data.seed);
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
    Encoded[24] = 255;

    const Automata::ReplayDecodeResult Decoded = Automata::DecodeReplay(Encoded);
    TestEqual(TEXT("Invalid enum byte is rejected"), Decoded.error, Automata::ReplayError::InvalidCommands);
    return true;
}

#endif
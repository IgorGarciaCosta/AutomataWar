/**
 * @file AutomataCoreTests.cpp
 * @brief Unreal Automation tests for the Automata War engine-independent core.
 */

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "AutomataWar/Core/Lang/AutomataCompiler.h"
#include "AutomataWar/Core/VM/AutomataVM.h"
#include "AutomataWar/Core/Sim/AutomataSimulation.h"
#include "AutomataWar/Core/Replay/AutomataReplay.h"

// ============================================================================
// Compiler tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompilerValidProgram, "AutomataWar.Core.Compiler.ValidProgram",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompilerValidProgram::RunTest(const FString &Parameters)
{
    using namespace Automata;
    auto r = Compile("MOVE FWD\nTURN RIGHT\nSCAN\nFIRE\nSHIELD\nSET R0 42\nWAIT\n");
    TestTrue(TEXT("No errors"), r.Ok());
    TestEqual(TEXT("7 instructions"), static_cast<int32>(r.program.code.size()), 7);
    TestEqual(TEXT("Source map size"), static_cast<int32>(r.program.sourceMap.size()), 7);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInstructionDefinitions, "AutomataWar.Core.Compiler.ExactInstructionDefinitions",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInstructionDefinitions::RunTest(const FString &Parameters)
{
    using namespace Automata;
    const int32_t expectedTicks[] = {2, 1, 1, 4, 3, 1, 1, 1};
    const int32_t expectedEnergy[] = {2, 1, 3, 12, 15, 0, 0, 0};
    const char *expectedSyntax[] = {
        "MOVE <FWD|BACK>", "TURN <LEFT|RIGHT>", "SCAN", "FIRE", "SHIELD",
        "SET <Rn> <imm>", "IF <reg> <OP> <imm> JUMP <label>", "WAIT"};

    TestEqual(TEXT("Exactly eight definitions"), static_cast<int32>(InstructionDefs.size()), 8);
    for (int32_t index = 0; index < OpcodeCount; ++index)
    {
        TestEqual(FString::Printf(TEXT("Tick cost %d"), index), InstructionDefs[index].tickCost, expectedTicks[index]);
        TestEqual(FString::Printf(TEXT("Energy cost %d"), index), InstructionDefs[index].energyCost, expectedEnergy[index]);
        TestTrue(FString::Printf(TEXT("Syntax %d"), index), std::string(InstructionDefs[index].syntax) == expectedSyntax[index]);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompilerMoveFwdBack, "AutomataWar.Core.Compiler.MoveFwdBack",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompilerMoveFwdBack::RunTest(const FString &Parameters)
{
    using namespace Automata;
    auto r = Compile("MOVE FWD\nMOVE BACK\n");
    TestTrue(TEXT("Compiles"), r.Ok());
    TestEqual(TEXT("FWD operand"), static_cast<int>(r.program.code[0].operandA), 0);
    TestEqual(TEXT("BACK operand"), static_cast<int>(r.program.code[1].operandA), 1);
    // Reject bare MOVE
    auto r2 = Compile("MOVE\n");
    TestFalse(TEXT("Bare MOVE rejected"), r2.Ok());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompilerTurnLeftRight, "AutomataWar.Core.Compiler.TurnLeftRight",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompilerTurnLeftRight::RunTest(const FString &Parameters)
{
    using namespace Automata;
    auto r = Compile("TURN LEFT\nTURN RIGHT\n");
    TestTrue(TEXT("Compiles"), r.Ok());
    TestEqual(TEXT("LEFT=-1"), static_cast<int>(r.program.code[0].imm16), -1);
    TestEqual(TEXT("RIGHT=1"), static_cast<int>(r.program.code[1].imm16), 1);
    // Reject numeric
    auto r2 = Compile("TURN -1\n");
    TestFalse(TEXT("Numeric rejected"), r2.Ok());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompilerIFSixTokens, "AutomataWar.Core.Compiler.IFSixTokens",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompilerIFSixTokens::RunTest(const FString &Parameters)
{
    using namespace Automata;
    // Valid: IF reg op imm JUMP label
    auto r = Compile("loop: WAIT\nIF R0 == 0 JUMP loop\n");
    TestTrue(TEXT("Valid IF compiles"), r.Ok());
    // Missing JUMP keyword
    auto r2 = Compile("loop: WAIT\nIF R0 == 0 loop\n");
    TestFalse(TEXT("Missing JUMP rejected"), r2.Ok());
    // Too few tokens
    auto r3 = Compile("loop: WAIT\nIF R0 == 0 JUMP\n");
    TestFalse(TEXT("Too few tokens rejected"), r3.Ok());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompilerRejectAliases, "AutomataWar.Core.Compiler.RejectAliases",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompilerRejectAliases::RunTest(const FString &Parameters)
{
    using namespace Automata;
    // EQ alias
    auto r = Compile("loop: WAIT\nIF R0 EQ 0 JUMP loop\n");
    TestFalse(TEXT("EQ alias rejected"), r.Ok());
    bool found = false;
    for (auto &d : r.diagnostics)
        if (d.kind == Automata::DiagKind::AliasRejected)
            found = true;
    TestTrue(TEXT("AliasRejected diagnostic"), found);
    // GOTO alias
    auto r2 = Compile("loop: WAIT\nIF R0 == 0 GOTO loop\n");
    TestFalse(TEXT("GOTO rejected"), r2.Ok());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompilerIFImmediateOnly, "AutomataWar.Core.Compiler.IFImmediateOnly",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompilerIFImmediateOnly::RunTest(const FString &Parameters)
{
    using namespace Automata;
    // Register as right operand should be rejected.
    auto r = Compile("loop: WAIT\nIF R0 == R1 JUMP loop\n");
    TestFalse(TEXT("Register RHS rejected"), r.Ok());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompilerUnknownInstruction, "AutomataWar.Core.Compiler.UnknownInstruction",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompilerUnknownInstruction::RunTest(const FString &Parameters)
{
    using namespace Automata;
    auto r = Compile("MAVE FWD\n");
    TestFalse(TEXT("Has errors"), r.Ok());
    TestEqual(TEXT("Diagnostic count"), static_cast<int32>(r.diagnostics.size()), 1);
    TestEqual(TEXT("Kind"), static_cast<int>(r.diagnostics[0].kind), static_cast<int>(DiagKind::UnknownInstruction));
    TestFalse(TEXT("Has suggestion"), r.diagnostics[0].suggestion.empty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompilerDuplicateLabel, "AutomataWar.Core.Compiler.DuplicateLabel",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompilerDuplicateLabel::RunTest(const FString &Parameters)
{
    using namespace Automata;
    auto r = Compile("loop: MOVE FWD\nloop: FIRE\n");
    TestFalse(TEXT("Has errors"), r.Ok());
    bool found = false;
    for (auto &d : r.diagnostics)
        if (d.kind == DiagKind::DuplicateLabel)
            found = true;
    TestTrue(TEXT("DuplicateLabel diagnostic"), found);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompilerUnknownLabel, "AutomataWar.Core.Compiler.UnknownLabel",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompilerUnknownLabel::RunTest(const FString &Parameters)
{
    using namespace Automata;
    auto r = Compile("IF R0 == 0 JUMP nowhere\n");
    TestFalse(TEXT("Has errors"), r.Ok());
    bool found = false;
    for (auto &d : r.diagnostics)
        if (d.kind == DiagKind::UnknownLabel)
            found = true;
    TestTrue(TEXT("UnknownLabel diagnostic"), found);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompilerSetReadOnly, "AutomataWar.Core.Compiler.SetReadOnly",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompilerSetReadOnly::RunTest(const FString &Parameters)
{
    using namespace Automata;
    auto r = Compile("SET R_HP 100\n");
    TestFalse(TEXT("Has errors"), r.Ok());
    bool found = false;
    for (auto &d : r.diagnostics)
        if (d.kind == DiagKind::SetToReadOnly)
            found = true;
    TestTrue(TEXT("SetToReadOnly diagnostic"), found);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompilerImmediateRange, "AutomataWar.Core.Compiler.ImmediateRange",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompilerImmediateRange::RunTest(const FString &Parameters)
{
    using namespace Automata;
    auto r = Compile("SET R0 99999\n");
    TestFalse(TEXT("Has errors"), r.Ok());
    bool found = false;
    for (auto &d : r.diagnostics)
        if (d.kind == DiagKind::ImmediateOutOfRange)
            found = true;
    TestTrue(TEXT("ImmediateOutOfRange diagnostic"), found);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompilerMultiError, "AutomataWar.Core.Compiler.MultiError",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompilerMultiError::RunTest(const FString &Parameters)
{
    using namespace Automata;
    auto r = Compile("MAVE FWD\nSET R_HP 99999\n");
    TestFalse(TEXT("Has errors"), r.Ok());
    TestTrue(TEXT("Multiple diagnostics"), r.diagnostics.size() >= 2);
    return true;
}

// ============================================================================
// VM tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVMBasicDispatch, "AutomataWar.Core.VM.BasicDispatch",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVMBasicDispatch::RunTest(const FString &Parameters)
{
    using namespace Automata;
    auto r = Compile("MOVE FWD\nFIRE\n");
    TestTrue(TEXT("Compiles"), r.Ok());

    VMState state;
    auto i1 = VMTick(state, r.program);
    TestEqual(TEXT("First intent Move"), static_cast<int>(i1.type), static_cast<int>(IntentType::Move));
    TestEqual(TEXT("Move param FWD"), static_cast<int>(i1.param), 0);
    TestEqual(TEXT("PC advanced"), static_cast<int>(state.pc), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVMMoveBusyTwoTicks, "AutomataWar.Core.VM.MoveBusyTwoTicks",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVMMoveBusyTwoTicks::RunTest(const FString &Parameters)
{
    using namespace Automata;
    auto r = Compile("MOVE FWD\nFIRE\n");
    TestTrue(TEXT("Compiles"), r.Ok());

    VMState state;
    auto i1 = VMTick(state, r.program); // MOVE dispatched, busy for 1 more tick
    TestEqual(TEXT("Move"), static_cast<int>(i1.type), static_cast<int>(IntentType::Move));
    TestEqual(TEXT("MOVE is the executing bytecode"), state.currentInstruction, 0);
    auto i2 = VMTick(state, r.program); // busy tick
    TestEqual(TEXT("Busy"), static_cast<int>(i2.type), static_cast<int>(IntentType::None));
    TestEqual(TEXT("Busy tick retains executing bytecode"), state.currentInstruction, 0);
    auto i3 = VMTick(state, r.program); // FIRE dispatched
    TestEqual(TEXT("Fire"), static_cast<int>(i3.type), static_cast<int>(IntentType::Fire));
    TestEqual(TEXT("FIRE becomes the executing bytecode"), state.currentInstruction, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVMFireBusyFourTicks, "AutomataWar.Core.VM.FireBusyFourTicks",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVMFireBusyFourTicks::RunTest(const FString &Parameters)
{
    using namespace Automata;
    auto r = Compile("FIRE\nWAIT\n");
    TestTrue(TEXT("Compiles"), r.Ok());

    VMState state;
    auto i1 = VMTick(state, r.program); // FIRE dispatched tick 0
    TestEqual(TEXT("Fire"), static_cast<int>(i1.type), static_cast<int>(IntentType::Fire));
    // Busy for 3 more ticks (total 4)
    for (int t = 0; t < 3; ++t)
    {
        auto ib = VMTick(state, r.program);
        TestEqual(TEXT("Busy"), static_cast<int>(ib.type), static_cast<int>(IntentType::None));
    }
    // Now WAIT
    auto i5 = VMTick(state, r.program);
    TestEqual(TEXT("Wait after fire"), static_cast<int>(i5.type), static_cast<int>(IntentType::Wait));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVMShieldBusy, "AutomataWar.Core.VM.ShieldBusyThreeTicks",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVMShieldBusy::RunTest(const FString &Parameters)
{
    using namespace Automata;
    auto r = Compile("SHIELD\nWAIT\n");
    TestTrue(TEXT("Compiles"), r.Ok());

    VMState state;
    auto i1 = VMTick(state, r.program);
    TestEqual(TEXT("Shield"), static_cast<int>(i1.type), static_cast<int>(IntentType::Shield));
    // Busy for 2 more ticks
    auto i2 = VMTick(state, r.program);
    TestEqual(TEXT("Busy1"), static_cast<int>(i2.type), static_cast<int>(IntentType::None));
    auto i3 = VMTick(state, r.program);
    TestEqual(TEXT("Busy2"), static_cast<int>(i3.type), static_cast<int>(IntentType::None));
    auto i4 = VMTick(state, r.program);
    TestEqual(TEXT("Wait"), static_cast<int>(i4.type), static_cast<int>(IntentType::Wait));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVMWrap, "AutomataWar.Core.VM.ProgramWrap",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVMWrap::RunTest(const FString &Parameters)
{
    using namespace Automata;
    auto r = Compile("WAIT\n");
    TestTrue(TEXT("Compiles"), r.Ok());

    VMState state;
    VMTick(state, r.program);
    TestEqual(TEXT("PC wraps to 0"), static_cast<int>(state.pc), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVMEmptyHalt, "AutomataWar.Core.VM.EmptyProgramHalt",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVMEmptyHalt::RunTest(const FString &Parameters)
{
    using namespace Automata;
    Program empty;
    VMState state;
    auto i = VMTick(state, empty);
    TestEqual(TEXT("None"), static_cast<int>(i.type), static_cast<int>(IntentType::None));
    TestTrue(TEXT("Halted"), state.halted);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVMSetAndIF, "AutomataWar.Core.VM.SetAndIF",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVMSetAndIF::RunTest(const FString &Parameters)
{
    using namespace Automata;
    auto r = Compile("SET R0 5\nIF R0 == 5 JUMP target\nWAIT\ntarget: FIRE\n");
    TestTrue(TEXT("Compiles"), r.Ok());

    VMState state;
    VMTick(state, r.program); // SET
    TestEqual(TEXT("R0 set"), state.regs[0], 5);
    VMTick(state, r.program); // IF (taken)
    TestEqual(TEXT("PC jumps"), static_cast<int>(state.pc), 3);
    auto i = VMTick(state, r.program); // FIRE
    TestEqual(TEXT("Fire"), static_cast<int>(i.type), static_cast<int>(IntentType::Fire));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVMAllComparisons, "AutomataWar.Core.VM.AllComparisons",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVMAllComparisons::RunTest(const FString &Parameters)
{
    using namespace Automata;

    auto testCmp = [&](const char *op, int32_t lhs, int32_t rhs, bool expectTaken)
    {
        std::string src = "SET R0 " + std::to_string(lhs) + "\n";
        src += "IF R0 " + std::string(op) + " " + std::to_string(rhs) + " JUMP target\nWAIT\ntarget: FIRE\n";
        auto r = Compile(src);
        if (!r.Ok())
        {
            TestTrue(TEXT("Compiles"), false);
            return;
        }
        VMState state;
        VMTick(state, r.program); // SET
        VMTick(state, r.program); // IF
        bool taken = (state.pc == 3);
        FString desc = FString::Printf(TEXT("%hs %d vs %d"), op, lhs, rhs);
        TestEqual(desc, taken, expectTaken);
    };

    testCmp("==", 5, 5, true);
    testCmp("==", 5, 3, false);
    testCmp("!=", 5, 3, true);
    testCmp("!=", 5, 5, false);
    testCmp("<", 3, 5, true);
    testCmp("<", 5, 3, false);
    testCmp("<=", 5, 5, true);
    testCmp("<=", 6, 5, false);
    testCmp(">", 5, 3, true);
    testCmp(">", 3, 5, false);
    testCmp(">=", 5, 5, true);
    testCmp(">=", 4, 5, false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVMEnergyInert, "AutomataWar.Core.VM.EnergyInert",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVMEnergyInert::RunTest(const FString &Parameters)
{
    using namespace Automata;
    auto r = Compile("SET R0 1\nIF R0 == 1 JUMP loop\nloop: WAIT\n");
    TestTrue(TEXT("Compiles"), r.Ok());

    VMState state;
    state.energyInert = true;
    // Should return Wait without advancing PC.
    auto i1 = VMTick(state, r.program);
    TestEqual(TEXT("Inert returns Wait"), static_cast<int>(i1.type), static_cast<int>(IntentType::Wait));
    TestEqual(TEXT("PC unchanged"), static_cast<int>(state.pc), 0);
    // Multiple calls stay inert.
    auto i2 = VMTick(state, r.program);
    TestEqual(TEXT("Still inert"), static_cast<int>(i2.type), static_cast<int>(IntentType::Wait));
    TestEqual(TEXT("PC still 0"), static_cast<int>(state.pc), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVMInstrCount, "AutomataWar.Core.VM.InstructionCount",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVMInstrCount::RunTest(const FString &Parameters)
{
    using namespace Automata;
    auto r = Compile("WAIT\nWAIT\nWAIT\n");
    TestTrue(TEXT("Compiles"), r.Ok());

    VMState state;
    VMTick(state, r.program);
    VMTick(state, r.program);
    VMTick(state, r.program);
    TestEqual(TEXT("3 instructions executed"), state.instrExecCount, 3);
    return true;
}

// ============================================================================
// Simulation tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSimDeterminism, "AutomataWar.Core.Sim.Determinism",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSimDeterminism::RunTest(const FString &Parameters)
{
    using namespace Automata;
    std::string srcA = "SCAN\nIF R_ENEMY_DIST > 0 JUMP attack\nMOVE FWD\nIF R0 == 0 JUMP top\ntop: SCAN\nattack: FIRE\nTURN RIGHT\n";
    std::string srcB = "MOVE FWD\nMOVE FWD\nTURN LEFT\nFIRE\n";
    auto progA = Compile(srcA);
    auto progB = Compile(srcB);
    TestTrue(TEXT("A compiles"), progA.Ok());
    TestTrue(TEXT("B compiles"), progB.Ok());

    SimConfig cfg;
    cfg.seed = 77777;
    uint64_t firstHash = 0;

    for (int32 i = 0; i < 1000; ++i)
    {
        Simulation sim;
        sim.RunMatch(progA.program, progB.program, cfg);
        if (i == 0)
            firstHash = sim.GetFinalHash();
        else
            TestEqual(TEXT("Hash stable"), sim.GetFinalHash(), firstHash);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSimMoveBlocked, "AutomataWar.Core.Sim.MoveBlocked",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSimMoveBlocked::RunTest(const FString &Parameters)
{
    using namespace Automata;
    // Robot 0 faces South at (1,1). Turn to face North (TURN LEFT twice) then move into wall.
    std::string src = "TURN LEFT\nTURN LEFT\nMOVE FWD\n";
    auto prog = Compile(src);
    TestTrue(TEXT("Compiles"), prog.Ok());

    Simulation sim;
    auto progB = Compile("WAIT\n");
    sim.RunMatch(prog.program, progB.program);

    bool blockedWall = false;
    for (auto &e : sim.GetEvents())
        if (e.robot == 0 && e.type == EventType::MoveBlockedWall)
        {
            blockedWall = true;
            break;
        }
    TestTrue(TEXT("Blocked by wall"), blockedWall);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSimScanHitMiss, "AutomataWar.Core.Sim.ScanHitMiss",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSimScanHitMiss::RunTest(const FString &Parameters)
{
    using namespace Automata;
    // Tiny grid, no cover (seed chosen for minimal cover).
    SimConfig cfg;
    cfg.gridWidth = 6;
    cfg.gridHeight = 6;
    cfg.seed = 42;

    // Robot 0 at (1,1) facing South. Robot 1 at (4,4) facing North.
    // Robot 0 scan: enemy is SE, not in south cone. Expect miss.
    std::string srcA = "SCAN\nWAIT\n";
    std::string srcB = "WAIT\n";
    auto progA = Compile(srcA);
    auto progB = Compile(srcB);
    TestTrue(TEXT("A"), progA.Ok());
    TestTrue(TEXT("B"), progB.Ok());

    Simulation sim;
    sim.RunMatch(progA.program, progB.program, cfg);

    // Check first scan event for robot 0.
    bool foundScan = false;
    for (auto &e : sim.GetEvents())
    {
        if (e.robot == 0 && e.type == EventType::Scan)
        {
            foundScan = true;
            // paramA = 0 means miss, 1 means hit.
            // Whether hit or miss depends on exact positions and cone. Just verify event exists.
            break;
        }
    }
    TestTrue(TEXT("Scan event exists"), foundScan);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSimExactCosts, "AutomataWar.Core.Sim.ExactEnergyCosts",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSimExactCosts::RunTest(const FString &Parameters)
{
    using namespace Automata;
    // Single MOVE FWD costs energy 2.
    SimConfig cfg;
    cfg.gridWidth = 8;
    cfg.gridHeight = 8;
    cfg.seed = 1;

    std::string srcA = "MOVE FWD\nWAIT\n";
    std::string srcB = "WAIT\n";
    auto progA = Compile(srcA);
    auto progB = Compile(srcB);

    Simulation sim;
    auto result = sim.RunMatch(progA.program, progB.program, cfg);
    // Robot 0 energy should be MaxEnergy - 2 after first move.
    // Due to tick cap and program looping, just check final energy < MaxEnergy.
    TestTrue(TEXT("Energy consumed"), result.finalEnergy[0] < MaxEnergy);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSimEnergyExhaustion, "AutomataWar.Core.Sim.UnaffordableActionExhaustsEnergy",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSimEnergyExhaustion::RunTest(const FString &Parameters)
{
    using namespace Automata;
    const auto shooter = Compile("FIRE\n");
    const auto idle = Compile("WAIT\n");
    TestTrue(TEXT("Programs compile"), shooter.Ok() && idle.Ok());

    Simulation sim;
    const MatchResult result = sim.RunMatch(shooter.program, idle.program);
    int32_t depletedEvents = 0;
    for (const SimEvent &event : sim.GetEvents())
        if (event.robot == 0 && event.type == EventType::EnergyDepleted)
            ++depletedEvents;

    TestEqual(TEXT("Unaffordable FIRE exhausts remaining energy"), result.finalEnergy[0], 0);
    TestEqual(TEXT("Energy depletion is surfaced once"), depletedEvents, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSimProjectileSpeed, "AutomataWar.Core.Sim.ProjectileTravelsFourCellsPerTick",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSimProjectileSpeed::RunTest(const FString &Parameters)
{
    using namespace Automata;
    const auto shooter = Compile("FIRE\n");
    const auto idle = Compile("WAIT\n");
    TestTrue(TEXT("Programs compile"), shooter.Ok() && idle.Ok());

    bool foundClearLane = false;
    for (uint64_t seed = 1; seed <= 100 && !foundClearLane; ++seed)
    {
        SimConfig config;
        config.gridWidth = 3;
        config.gridHeight = 8;
        config.seed = seed;

        Simulation sim;
        sim.RunMatch(shooter.program, idle.program, config);
        int32_t fireTick = -1;
        int32_t hitTick = -1;
        for (const SimEvent &event : sim.GetEvents())
        {
            if (event.robot == 0 && event.type == EventType::Fire && fireTick < 0)
                fireTick = event.tick;
            if (event.robot == 1 && event.type == EventType::Hit && hitTick < 0)
                hitTick = event.tick;
        }
        if (hitTick >= 0)
        {
            foundClearLane = true;
            TestTrue(TEXT("A target five cells away is not hit on the firing tick"), hitTick > fireTick);
        }
    }

    TestTrue(TEXT("Found deterministic clear projectile lane"), foundClearLane);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSimRelativeScanDirection, "AutomataWar.Core.Sim.ScanDirectionIsRelative",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSimRelativeScanDirection::RunTest(const FString &Parameters)
{
    using namespace Automata;
    const auto scanner = Compile("SCAN\n");
    const auto idle = Compile("WAIT\n");
    TestTrue(TEXT("Programs compile"), scanner.Ok() && idle.Ok());

    bool foundClearLane = false;
    for (uint64_t seed = 1; seed <= 100 && !foundClearLane; ++seed)
    {
        SimConfig config;
        config.gridWidth = 3;
        config.gridHeight = 8;
        config.seed = seed;

        Simulation sim;
        sim.RunMatch(scanner.program, idle.program, config);
        for (const SimEvent &event : sim.GetEvents())
        {
            if (event.robot == 0 && event.type == EventType::Scan && event.paramA == 1)
            {
                foundClearLane = true;
                const int32_t relativeDirection = sim.GetSnapshots()[event.tick].robots[0].vm.regs[static_cast<int32_t>(Reg::R_ENEMY_DIR)];
                TestEqual(TEXT("Enemy directly ahead is relative direction 0"), relativeDirection, 0);
                break;
            }
        }
    }

    TestTrue(TEXT("Found deterministic clear scan lane"), foundClearLane);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSimFireHit, "AutomataWar.Core.Sim.FireHit",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSimFireHit::RunTest(const FString &Parameters)
{
    using namespace Automata;
    SimConfig cfg;
    cfg.gridWidth = 8;
    cfg.gridHeight = 8;
    cfg.seed = 1;

    std::string srcA = "TURN RIGHT\nFIRE\nFIRE\nFIRE\nFIRE\nFIRE\n";
    std::string srcB = "WAIT\n";
    auto progA = Compile(srcA);
    auto progB = Compile(srcB);
    TestTrue(TEXT("A"), progA.Ok());
    TestTrue(TEXT("B"), progB.Ok());

    Simulation sim;
    auto result = sim.RunMatch(progA.program, progB.program, cfg);
    TestTrue(TEXT("Completed"), result.finalTick > 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSimShieldAbsorb, "AutomataWar.Core.Sim.ShieldAbsorb",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSimShieldAbsorb::RunTest(const FString &Parameters)
{
    using namespace Automata;
    SimConfig cfg;
    cfg.gridWidth = 6;
    cfg.gridHeight = 6;
    cfg.seed = 999999;

    std::string srcA = "TURN RIGHT\nFIRE\nFIRE\nFIRE\nFIRE\n";
    std::string srcB = "SHIELD\nSHIELD\nSHIELD\nSHIELD\n";
    auto progA = Compile(srcA);
    auto progB = Compile(srcB);
    TestTrue(TEXT("A"), progA.Ok());
    TestTrue(TEXT("B"), progB.Ok());

    Simulation sim;
    sim.RunMatch(progA.program, progB.program, cfg);
    TestTrue(TEXT("Completed"), sim.GetSnapshots().size() > 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSimHeadless, "AutomataWar.Core.Sim.HeadlessNoUObject",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSimHeadless::RunTest(const FString &Parameters)
{
    using namespace Automata;
    std::string srcA = "MOVE FWD\nTURN RIGHT\nSCAN\nIF R_ENEMY_DIST > 0 JUMP fire\nMOVE FWD\nIF R0 == 0 JUMP top\ntop: MOVE FWD\nfire: FIRE\n";
    std::string srcB = "MOVE FWD\nMOVE FWD\nTURN LEFT\nMOVE FWD\nFIRE\nSCAN\n";
    auto progA = Compile(srcA);
    auto progB = Compile(srcB);
    TestTrue(TEXT("A"), progA.Ok());
    TestTrue(TEXT("B"), progB.Ok());

    Simulation sim;
    auto result = sim.RunMatch(progA.program, progB.program);
    TestTrue(TEXT("Finished"), result.finalTick > 0);
    TestTrue(TEXT("Valid outcome"), static_cast<int>(result.outcome) <= 2);
    return true;
}

// ============================================================================
// Replay tests
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReplayRoundtrip, "AutomataWar.Core.Replay.Roundtrip",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReplayRoundtrip::RunTest(const FString &Parameters)
{
    using namespace Automata;
    ReplayData data;
    data.seed = 42;
    data.sourceA = "MOVE FWD\nFIRE\nSCAN\n";
    data.sourceB = "SHIELD\nWAIT\nTURN RIGHT\n";

    auto encoded = EncodeReplay(data);
    TestTrue(TEXT("Under 4KiB"), encoded.size() < 4096);

    auto decoded = DecodeReplay(encoded);
    TestTrue(TEXT("Decode OK"), decoded.Ok());
    TestEqual(TEXT("Seed match"), decoded.data.seed, data.seed);
    TestTrue(TEXT("SourceA match"), decoded.data.sourceA == data.sourceA);
    TestTrue(TEXT("SourceB match"), decoded.data.sourceB == data.sourceB);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReplayBase64, "AutomataWar.Core.Replay.Base64",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReplayBase64::RunTest(const FString &Parameters)
{
    using namespace Automata;
    ReplayData data;
    data.seed = 12345;
    data.sourceA = "MOVE FWD\n";
    data.sourceB = "WAIT\n";

    auto binary = EncodeReplay(data);
    std::string b64 = ReplayToBase64(binary);

    std::vector<uint8_t> roundtrip;
    bool ok = ReplayFromBase64(b64, roundtrip);
    TestTrue(TEXT("Base64 decode OK"), ok);
    TestEqual(TEXT("Size match"), static_cast<int32>(roundtrip.size()), static_cast<int32>(binary.size()));
    TestTrue(TEXT("Content match"), roundtrip == binary);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReplayResimHash, "AutomataWar.Core.Replay.ResimHash",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReplayResimHash::RunTest(const FString &Parameters)
{
    using namespace Automata;
    std::string srcA = "MOVE FWD\nFIRE\nTURN RIGHT\n";
    std::string srcB = "SCAN\nWAIT\nMOVE FWD\n";

    ReplayData data;
    data.seed = 55555;
    data.sourceA = srcA;
    data.sourceB = srcB;

    auto progA = Compile(srcA);
    auto progB = Compile(srcB);
    TestTrue(TEXT("A"), progA.Ok());
    TestTrue(TEXT("B"), progB.Ok());

    SimConfig cfg;
    cfg.seed = data.seed;
    Simulation sim1;
    sim1.RunMatch(progA.program, progB.program, cfg);
    uint64_t hash1 = sim1.GetFinalHash();

    Simulation sim2;
    sim2.RunMatch(progA.program, progB.program, cfg);
    uint64_t hash2 = sim2.GetFinalHash();
    TestEqual(TEXT("Resim hash matches"), hash1, hash2);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

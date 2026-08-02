#pragma once

/**
 * @file AutomataVM.h
 * @brief Deterministic virtual machine that executes compiled Automata bytecode.
 *
 * Emits fixed-size intents for the simulation. One instruction dispatch max per tick.
 * Engine-independent: no UObject, no Unreal types.
 */

#include "AutomataWar/Core/AutomataRules.h"
#include "AutomataWar/Core/Lang/AutomataCompiler.h"
#include <array>
#include <cstdint>

namespace Automata
{

    // --- Intent ------------------------------------------------------------------

    /** Arena-agnostic action categories emitted by a VM dispatch. */
    enum class IntentType : uint8_t
    {
        None, // busy or halted
        Move,
        Turn,
        Scan,
        Fire,
        Shield,
        Wait
    };

    /** Fixed-size request for the simulation to validate and apply. */
    struct Intent
    {
        IntentType type = IntentType::None;
        int16_t param = 0; // TURN: -1/1. MOVE: 0=FWD, 1=BACK.
    };

    // --- VM state ----------------------------------------------------------------

    /** Complete deterministic execution state owned by one robot VM. */
    struct VMState
    {
        std::array<int32_t, TotalRegisterCount> regs = {};
        uint16_t pc = 0;
        int32_t busyLeft = 0;
        int32_t currentInstruction = -1; // Bytecode index executing during this tick, including busy ticks.
        bool halted = false;
        bool energyInert = false;   // set by simulation when energy==0
        int32_t instrExecCount = 0; // total instructions dispatched
    };

    // --- VM interface ------------------------------------------------------------

    /**
     * Execute one VM tick with at most one instruction dispatch.
     * @param state Mutable execution state for one robot.
     * @param program Immutable compiled bytecode.
     * @return An arena-agnostic intent, or None while busy/halted.
     *
     * Energy checking is external (simulation sets energyInert). When energyInert
     * the VM returns Wait unconditionally without advancing PC, preventing
     * zero-cost loops. Program wraps at end.
     */
    Intent VMTick(VMState &state, const Program &program);

} // namespace Automata

#pragma once

/**
 * @file AutomataVM.h
 * @brief Deterministic virtual machine that executes compiled Automata bytecode.
 *
 * The VM has no arena/opponent access. It emits fixed-size intents that the
 * simulation consumes. One instruction dispatch per tick maximum.
 * Engine-independent: no UObject, no Unreal types.
 */

#include "AutomataWar/Core/AutomataRules.h"
#include "AutomataWar/Core/Lang/AutomataCompiler.h"
#include <array>
#include <cstdint>

namespace Automata
{

// ─── Intent ──────────────────────────────────────────────────────────────────

/** Action the VM requests for the current tick. */
enum class IntentType : uint8_t
{
    None,    ///< No action (busy or halted).
    Move,
    Turn,
    Scan,
    Fire,
    Shield,
    Wait
};

/** Fixed-size intent emitted by the VM each tick. */
struct Intent
{
    IntentType type    = IntentType::None;
    int16_t    param   = 0; ///< Direction for TURN (-1/1), otherwise unused.
};

// ─── VM state ────────────────────────────────────────────────────────────────

/** Per-robot VM execution state. */
struct VMState
{
    std::array<int32_t, TotalRegisterCount> regs = {}; ///< Register file.
    uint16_t pc       = 0;     ///< Program counter.
    int32_t  busyLeft = 0;     ///< Ticks remaining for multi-tick instruction.
    bool     halted   = false; ///< Set if PC goes out-of-range on empty program.
};

// ─── VM interface ────────────────────────────────────────────────────────────

/**
 * @brief Execute one tick of the VM for a robot.
 * @param state Mutable VM state (registers, PC, busy counter).
 * @param program The compiled bytecode.
 * @return Intent describing what the robot wants to do this tick.
 *
 * Guarantees:
 * - At most one instruction dispatched per call.
 * - PC out-of-range causes safe halt (returns None).
 * - No dynamic allocation.
 * - Energy is tracked externally by the simulation.
 */
Intent VMTick(VMState& state, const Program& program);

} // namespace Automata

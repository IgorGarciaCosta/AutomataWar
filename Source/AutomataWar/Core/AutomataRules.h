#pragma once

/**
 * @file AutomataRules.h
 * @brief Single source-of-truth for all balance constants, instruction definitions, and
 *        register layout used by the Automata War core engine.
 *
 * Engine-independent: no UObject, no Unreal headers, plain C++20.
 */

#include <cstdint>
#include <array>

namespace Automata
{

// ─── Grid ────────────────────────────────────────────────────────────────────

/** Default arena width in cells. */
inline constexpr int32_t DefaultGridWidth = 16;
/** Default arena height in cells. */
inline constexpr int32_t DefaultGridHeight = 16;

// ─── Robot stats ─────────────────────────────────────────────────────────────

/** Starting and maximum hit-points. */
inline constexpr int32_t MaxHP = 100;
/** Starting and maximum energy pool. */
inline constexpr int32_t MaxEnergy = 500;
/** Absolute tick cap before forced win evaluation. */
inline constexpr int32_t TickCap = 1800;

// ─── Instruction set ─────────────────────────────────────────────────────────

/** Opcode identifiers matching bytecode encoding (4 bits). */
enum class Opcode : uint8_t
{
    MOVE   = 0, ///< Advance 1 cell in facing direction.
    TURN   = 1, ///< Turn 90 degrees; operand: -1 (left) or 1 (right).
    SCAN   = 2, ///< Scan 90-degree cone; result in R0.
    FIRE   = 3, ///< Launch projectile in facing direction.
    SHIELD = 4, ///< Activate shield (busy 3 ticks, absorbs next hit).
    SET    = 5, ///< SET Rx, imm16 — load immediate into register.
    IF     = 6, ///< IF Rx cmp Ry/imm GOTO label — conditional jump.
    WAIT   = 7, ///< Do nothing for 1 tick.
    COUNT  = 8  ///< Sentinel — not a valid opcode.
};

/** Number of valid opcodes. */
inline constexpr int32_t OpcodeCount = static_cast<int32_t>(Opcode::COUNT);

// ─── Energy / tick costs per instruction ─────────────────────────────────────

/** Energy cost per instruction execution. Index by Opcode value. */
inline constexpr std::array<int32_t, OpcodeCount> EnergyCost = {
    /* MOVE */   5,
    /* TURN */   2,
    /* SCAN */  10,
    /* FIRE */  25,
    /* SHIELD*/  0,
    /* SET */    0,
    /* IF */     0,
    /* WAIT */   1
};

/** Tick duration cost per instruction (how many ticks the robot is busy). */
inline constexpr std::array<int32_t, OpcodeCount> TickCost = {
    /* MOVE */   1,
    /* TURN */   1,
    /* SCAN */   1,
    /* FIRE */   1,
    /* SHIELD */ 3,
    /* SET */    1,
    /* IF */     1,
    /* WAIT */   1
};

// ─── Combat / physics ────────────────────────────────────────────────────────

/** Projectile speed in cells per tick. */
inline constexpr int32_t ProjectileSpeed = 4;
/** Damage dealt by a single projectile hit. */
inline constexpr int32_t ProjectileDamage = 20;
/** SCAN range in cells. */
inline constexpr int32_t ScanRange = 8;

// ─── Registers ───────────────────────────────────────────────────────────────

/** Number of general-purpose registers (R0–R3). */
inline constexpr int32_t GPRegisterCount = 4;
/** Total register count (GP + system read-only). */
inline constexpr int32_t TotalRegisterCount = 9;

/** Register indices. */
enum class Reg : uint8_t
{
    R0 = 0,
    R1 = 1,
    R2 = 2,
    R3 = 3,
    R_HP         = 4,  ///< Current hit-points (read-only).
    R_ENEMY_DIST = 5,  ///< Manhattan distance to opponent (read-only).
    R_ENEMY_DIR  = 6,  ///< Relative direction to opponent 0-3 (read-only).
    R_ENERGY     = 7,  ///< Current energy (read-only).
    R_TICK       = 8   ///< Current simulation tick (read-only).
};

/** First system (read-only) register index. */
inline constexpr uint8_t FirstSystemReg = static_cast<uint8_t>(Reg::R_HP);

// ─── Comparison operators for IF ─────────────────────────────────────────────

/** Comparison operators used in conditional instructions. */
enum class CmpOp : uint8_t
{
    EQ = 0, ///< ==
    NE = 1, ///< !=
    LT = 2, ///< <
    LE = 3, ///< <=
    GT = 4, ///< >
    GE = 5  ///< >=
};

// ─── Directions ──────────────────────────────────────────────────────────────

/** Cardinal direction indices (clockwise from North). */
enum class Dir : uint8_t
{
    North = 0,
    East  = 1,
    South = 2,
    West  = 3
};

/** Delta-X for each direction. */
inline constexpr std::array<int32_t, 4> DirDX = { 0, 1, 0, -1 };
/** Delta-Y for each direction (Y increases southward). */
inline constexpr std::array<int32_t, 4> DirDY = { -1, 0, 1, 0 };

// ─── Bytecode limits ─────────────────────────────────────────────────────────

/** Maximum instructions per program (anti-abuse). */
inline constexpr int32_t MaxProgramLength = 256;
/** Maximum source lines accepted. */
inline constexpr int32_t MaxSourceLines = 512;
/** Immediate value range. */
inline constexpr int16_t ImmMin = -32768;
inline constexpr int16_t ImmMax = 32767;

// ─── Simulation limits ───────────────────────────────────────────────────────

/** Maximum simultaneous projectiles per robot. */
inline constexpr int32_t MaxProjectiles = 16;

// ─── Replay ──────────────────────────────────────────────────────────────────

/** Current replay format version. */
inline constexpr uint16_t ReplayVersion = 1;

// ─── Ruleset hash (compile-time fingerprint of balance table) ────────────────

/** FNV-1a of the concatenated balance constants for mismatch detection. */
inline constexpr uint64_t RulesetHash = []() constexpr -> uint64_t {
    uint64_t h = 14695981039346656037ULL;
    auto mix = [&](int64_t v) {
        for (int i = 0; i < 8; ++i) { h ^= static_cast<uint8_t>(v >> (i*8)); h *= 1099511628211ULL; }
    };
    mix(MaxHP); mix(MaxEnergy); mix(TickCap);
    mix(ProjectileSpeed); mix(ProjectileDamage); mix(ScanRange);
    for (auto c : EnergyCost) mix(c);
    for (auto c : TickCost) mix(c);
    mix(MaxProgramLength); mix(DefaultGridWidth); mix(DefaultGridHeight);
    return h;
}();

} // namespace Automata

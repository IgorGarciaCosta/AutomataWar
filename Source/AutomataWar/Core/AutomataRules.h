#pragma once

/**
 * @file AutomataRules.h
 * @brief Single compile-time source of truth for all balance constants, instruction
 *        definitions, register layout, and enumeration types for the Automata War core.
 *
 * Engine-independent: no UObject, no Unreal headers, plain C++20.
 */

#include <cstdint>
#include <array>

namespace Automata
{

// --- Grid -------------------------------------------------------------------

inline constexpr int32_t DefaultGridWidth = 16;
inline constexpr int32_t DefaultGridHeight = 16;

// --- Robot stats ------------------------------------------------------------

inline constexpr int32_t MaxHP = 100;
inline constexpr int32_t MaxEnergy = 500;
inline constexpr int32_t TickCap = 1800;

// --- Opcode identifiers (4 bits) --------------------------------------------

enum class Opcode : uint8_t
{
    MOVE   = 0,
    TURN   = 1,
    SCAN   = 2,
    FIRE   = 3,
    SHIELD = 4,
    SET    = 5,
    IF     = 6,
    WAIT   = 7,
    COUNT  = 8
};

inline constexpr int32_t OpcodeCount = static_cast<int32_t>(Opcode::COUNT);

// --- Instruction definition table (the authoritative balance sheet) ----------

struct InstructionDefinition
{
    Opcode      opcode;
    const char* mnemonic;
    const char* syntax;
    const char* description;
    int32_t     tickCost;
    int32_t     energyCost;
};

inline constexpr std::array<InstructionDefinition, 8> InstructionDefs = {{
    {Opcode::MOVE,   "MOVE",   "MOVE <FWD|BACK>",
     "Move one cell forward or backward relative to facing. Blocked by wall/cover/robot.",
     2, 2},
    {Opcode::TURN,   "TURN",   "TURN <LEFT|RIGHT>",
     "Rotate 90 degrees left or right.",
     1, 1},
    {Opcode::SCAN,   "SCAN",   "SCAN",
        "Scan forward 90-degree cone range 8. Cover blocks. Hit writes distance and relative direction (-1 left, 0 ahead, 1 right). Miss writes distance 0.",
     1, 3},
    {Opcode::FIRE,   "FIRE",   "FIRE",
     "Launch projectile in facing direction. Speed 4 cells/tick, damage 20. Cover blocks.",
     4, 12},
    {Opcode::SHIELD, "SHIELD", "SHIELD",
     "Activate shield absorbing next hit entirely. Robot is busy/immobilized for full duration.",
     3, 15},
    {Opcode::SET,    "SET",    "SET <Rn> <imm>",
     "Load immediate [-32768,32767] into R0..R3.",
     1, 0},
    {Opcode::IF,     "IF",     "IF <reg> <OP> <imm> JUMP <label>",
     "Conditional jump. OP: == != < > <= >=. Right operand is immediate only. JUMP keyword required.",
     1, 0},
    {Opcode::WAIT,   "WAIT",   "WAIT",
     "Do nothing for 1 tick.",
     1, 0},
}};

// Convenience cost arrays indexed by Opcode value.
inline constexpr std::array<int32_t, OpcodeCount> EnergyCost = {
    /* MOVE */    2,
    /* TURN */    1,
    /* SCAN */    3,
    /* FIRE */   12,
    /* SHIELD */ 15,
    /* SET */     0,
    /* IF */      0,
    /* WAIT */    0
};

inline constexpr std::array<int32_t, OpcodeCount> TickCost = {
    /* MOVE */    2,
    /* TURN */    1,
    /* SCAN */    1,
    /* FIRE */    4,
    /* SHIELD */  3,
    /* SET */     1,
    /* IF */      1,
    /* WAIT */    1
};

// Static assertions: cost arrays must equal the InstructionDefs table.
static_assert(EnergyCost[0] == InstructionDefs[0].energyCost);
static_assert(EnergyCost[1] == InstructionDefs[1].energyCost);
static_assert(EnergyCost[2] == InstructionDefs[2].energyCost);
static_assert(EnergyCost[3] == InstructionDefs[3].energyCost);
static_assert(EnergyCost[4] == InstructionDefs[4].energyCost);
static_assert(EnergyCost[5] == InstructionDefs[5].energyCost);
static_assert(EnergyCost[6] == InstructionDefs[6].energyCost);
static_assert(EnergyCost[7] == InstructionDefs[7].energyCost);
static_assert(TickCost[0] == InstructionDefs[0].tickCost);
static_assert(TickCost[1] == InstructionDefs[1].tickCost);
static_assert(TickCost[2] == InstructionDefs[2].tickCost);
static_assert(TickCost[3] == InstructionDefs[3].tickCost);
static_assert(TickCost[4] == InstructionDefs[4].tickCost);
static_assert(TickCost[5] == InstructionDefs[5].tickCost);
static_assert(TickCost[6] == InstructionDefs[6].tickCost);
static_assert(TickCost[7] == InstructionDefs[7].tickCost);

// --- Combat / physics -------------------------------------------------------

inline constexpr int32_t ProjectileSpeed = 4;
inline constexpr int32_t ProjectileDamage = 20;
inline constexpr int32_t ScanRange = 8;

// --- Registers --------------------------------------------------------------

inline constexpr int32_t GPRegisterCount = 4;
inline constexpr int32_t TotalRegisterCount = 9;

enum class Reg : uint8_t
{
    R0 = 0,
    R1 = 1,
    R2 = 2,
    R3 = 3,
    R_HP         = 4,
    R_ENEMY_DIST = 5,
    R_ENEMY_DIR  = 6,
    R_ENERGY     = 7,
    R_TICK       = 8
};

inline constexpr uint8_t FirstSystemReg = static_cast<uint8_t>(Reg::R_HP);

// Register definition table for UI/documentation consumption.
struct RegisterDefinition
{
    const char* name;
    const char* description;
    bool        readOnly;
};

inline constexpr std::array<RegisterDefinition, TotalRegisterCount> RegisterDefs = {{
    {"R0",           "General purpose register 0", false},
    {"R1",           "General purpose register 1", false},
    {"R2",           "General purpose register 2", false},
    {"R3",           "General purpose register 3", false},
    {"R_HP",         "Current hit-points",         true},
    {"R_ENEMY_DIST", "Distance to enemy (SCAN hit writes manhattan distance; miss writes 0)", true},
    {"R_ENEMY_DIR",  "Relative direction from last SCAN hit (-1 left, 0 ahead, 1 right)", true},
    {"R_ENERGY",     "Current energy pool",        true},
    {"R_TICK",       "Current simulation tick",    true},
}};

/** Relative direction values written to R_ENEMY_DIR by a successful SCAN. */
enum class RelativeEnemyDir : int32_t
{
    Left  = -1,
    Ahead = 0,
    Right = 1
};

// --- Comparison operators for IF --------------------------------------------

enum class CmpOp : uint8_t
{
    EQ = 0, // ==
    NE = 1, // !=
    LT = 2, // <
    LE = 3, // <=
    GT = 4, // >
    GE = 5  // >=
};

// --- Directions -------------------------------------------------------------

enum class Dir : uint8_t
{
    North = 0,
    East  = 1,
    South = 2,
    West  = 3
};

inline constexpr std::array<int32_t, 4> DirDX = { 0, 1, 0, -1 };
inline constexpr std::array<int32_t, 4> DirDY = { -1, 0, 1, 0 };

// --- Move direction (relative to facing) ------------------------------------

enum class MoveDir : uint8_t
{
    Forward  = 0,
    Backward = 1
};

// --- Bytecode limits --------------------------------------------------------

inline constexpr int32_t MaxProgramLength = 256;
inline constexpr int32_t MaxSourceLines = 512;
inline constexpr int16_t ImmMin = -32768;
inline constexpr int16_t ImmMax = 32767;

// --- Simulation limits ------------------------------------------------------

inline constexpr int32_t MaxProjectiles = 16;

// --- Replay -----------------------------------------------------------------

inline constexpr uint16_t ReplayVersion = 2;

// --- Ruleset hash (compile-time FNV-1a of all balance values) ---------------

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
    mix(GPRegisterCount); mix(TotalRegisterCount);
    return h;
}();

} // namespace Automata

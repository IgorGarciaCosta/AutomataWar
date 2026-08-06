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

    /** Default arena width: enough maneuvering room while keeping scans relevant. */
    inline constexpr int32_t DefaultGridWidth = 16;
    /** Default arena height, kept square for symmetric opposite-corner spawns. */
    inline constexpr int32_t DefaultGridHeight = 16;

    // --- Robot stats ------------------------------------------------------------

    /** Starting HP; five unshielded projectile hits destroy a robot. */
    inline constexpr int32_t MaxHP = 100;
    /** Starting energy; long enough for tactics while still rewarding WAIT. */
    inline constexpr int32_t MaxEnergy = 500;
    /** Hard match cap preventing behavior loops from running indefinitely. */
    inline constexpr int32_t TickCap = 900;

    // --- Opcode identifiers (4 bits) --------------------------------------------

    /** Stable bytecode opcode identifiers for the complete v1 instruction set. */
    enum class Opcode : uint8_t
    {
        MOVE = 0,
        TURN = 1,
        SCAN = 2,
        FIRE = 3,
        SHIELD = 4,
        SET = 5,
        IF = 6,
        WAIT = 7,
        COUNT = 8
    };

    inline constexpr int32_t OpcodeCount = static_cast<int32_t>(Opcode::COUNT);

    // --- Instruction definition table (the authoritative balance sheet) ----------

    /** Metadata shared by compiler validation, VM costs, UI reference, and docs. */
    struct InstructionDefinition
    {
        Opcode opcode;
        const char *mnemonic;
        const char *syntax;
        const char *description;
        int32_t tickCost;
        int32_t energyCost;
    };

    inline constexpr std::array<InstructionDefinition, 8> InstructionDefs = {{
        {Opcode::MOVE, "MOVE", "MOVE <FWD|BACK>",
         "Move one cell forward or backward relative to facing. Blocked by wall/cover/robot.",
         2, 2},
        {Opcode::TURN, "TURN", "TURN <LEFT|RIGHT>",
         "Rotate 90 degrees left or right.",
         1, 1},
        {Opcode::SCAN, "SCAN", "SCAN",
         "Scan forward 90-degree cone range 8. Cover blocks. Hit writes distance and relative direction (-1 left, 0 ahead, 1 right). Miss writes distance 0.",
         1, 3},
        {Opcode::FIRE, "FIRE", "FIRE",
         "Launch projectile in facing direction. Speed 4 cells/tick, damage 20. Cover blocks.",
         4, 12},
        {Opcode::SHIELD, "SHIELD", "SHIELD",
         "Activate shield absorbing next hit entirely. Robot is busy/immobilized for full duration.",
         3, 15},
        {Opcode::SET, "SET", "SET <Rn> <imm>",
         "Load immediate [-32768,32767] into R0..R3.",
         1, 0},
        {Opcode::IF, "IF", "IF <reg> <OP> <imm> JUMP <label>",
         "Conditional jump. OP: == != < > <= >=. Right operand is immediate only. JUMP keyword required.",
         1, 0},
        {Opcode::WAIT, "WAIT", "WAIT",
         "Do nothing for 1 tick.",
         1, 0},
    }};

    // --- Combat / physics -------------------------------------------------------

    /** Cells advanced per simulation tick, making projectiles visible but dangerous. */
    inline constexpr int32_t ProjectileSpeed = 4;
    /** Damage per hit; exactly one fifth of starting HP. */
    inline constexpr int32_t ProjectileDamage = 20;
    /** Starting health for destructible cover; three projectile hits destroy it. */
    inline constexpr int32_t ObstacleMaxHealth = 60;
    /** Maximum scan distance, covering half the default arena. */
    inline constexpr int32_t ScanRange = 8;

    // --- Registers --------------------------------------------------------------

    inline constexpr int32_t GPRegisterCount = 4;
    inline constexpr int32_t TotalRegisterCount = 9;

    /** Stable indices for writable and simulation-maintained registers. */
    enum class Reg : uint8_t
    {
        R0 = 0,
        R1 = 1,
        R2 = 2,
        R3 = 3,
        R_HP = 4,
        R_ENEMY_DIST = 5,
        R_ENEMY_DIR = 6,
        R_ENERGY = 7,
        R_TICK = 8
    };

    inline constexpr uint8_t FirstSystemReg = static_cast<uint8_t>(Reg::R_HP);

    // Register definition table for UI/documentation consumption.
    /** UI/compiler metadata describing a register and whether SET may write it. */
    struct RegisterDefinition
    {
        const char *name;
        const char *description;
        bool readOnly;
    };

    inline constexpr std::array<RegisterDefinition, TotalRegisterCount> RegisterDefs = {{
        {"R0", "General purpose register 0", false},
        {"R1", "General purpose register 1", false},
        {"R2", "General purpose register 2", false},
        {"R3", "General purpose register 3", false},
        {"R_HP", "Current hit-points", true},
        {"R_ENEMY_DIST", "Distance to enemy (SCAN hit writes manhattan distance; miss writes 0)", true},
        {"R_ENEMY_DIR", "Relative direction from last SCAN hit (-1 left, 0 ahead, 1 right)", true},
        {"R_ENERGY", "Current energy pool", true},
        {"R_TICK", "Current simulation tick", true},
    }};

    /** Relative direction values written to R_ENEMY_DIR by a successful SCAN. */
    enum class RelativeEnemyDir : int32_t
    {
        Left = -1,
        Ahead = 0,
        Right = 1
    };

    // --- Comparison operators for IF --------------------------------------------

    /** Comparison operators encoded in the operand byte of IF instructions. */
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

    /** Cardinal facing values ordered clockwise for deterministic rotation math. */
    enum class Dir : uint8_t
    {
        North = 0,
        East = 1,
        South = 2,
        West = 3
    };

    inline constexpr std::array<int32_t, 4> DirDX = {0, 1, 0, -1};
    inline constexpr std::array<int32_t, 4> DirDY = {-1, 0, 1, 0};

    // --- Move direction (relative to facing) ------------------------------------

    /** Relative movement operands accepted by MOVE. */
    enum class MoveDir : uint8_t
    {
        Forward = 0,
        Backward = 1
    };

    // --- Bytecode limits --------------------------------------------------------

    /** Bytecode cap bounding memory, PC targets, and hostile-input compile work. */
    inline constexpr int32_t MaxProgramLength = 256;
    /** Source-line cap allowing comments without permitting unbounded input. */
    inline constexpr int32_t MaxSourceLines = 512;
    /** Minimum immediate representable by the fixed 16-bit bytecode field. */
    inline constexpr int16_t ImmMin = -32768;
    /** Maximum immediate representable by the fixed 16-bit bytecode field. */
    inline constexpr int16_t ImmMax = 32767;

    // --- Simulation limits ------------------------------------------------------

    /** Fixed projectile slots per player, avoiding per-tick allocation. */
    inline constexpr int32_t MaxProjectiles = 16;

    // --- Replay -----------------------------------------------------------------

    /** Current compact replay binary format version. */
    inline constexpr uint16_t ReplayVersion = 2;

    // --- Ruleset hash (compile-time FNV-1a of all balance values) ---------------

    inline constexpr uint64_t RulesetHash = []() constexpr -> uint64_t
    {
        uint64_t h = 14695981039346656037ULL;
        auto mix = [&](int64_t v)
        {
            for (int i = 0; i < 8; ++i)
            {
                h ^= static_cast<uint8_t>(v >> (i * 8));
                h *= 1099511628211ULL;
            }
        };
        mix(MaxHP);
        mix(MaxEnergy);
        mix(TickCap);
        mix(ProjectileSpeed);
        mix(ProjectileDamage);
        mix(ScanRange);
        for (const InstructionDefinition &Definition : InstructionDefs)
            mix(Definition.energyCost);
        for (const InstructionDefinition &Definition : InstructionDefs)
            mix(Definition.tickCost);
        mix(MaxProgramLength);
        mix(DefaultGridWidth);
        mix(DefaultGridHeight);
        mix(GPRegisterCount);
        mix(TotalRegisterCount);
        return h;
    }();

} // namespace Automata

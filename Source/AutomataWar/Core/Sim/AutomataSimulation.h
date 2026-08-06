#pragma once

/**
 * @file AutomataSimulation.h
 * @brief Deterministic headless simulation of an Automata War match.
 *
 * Integer-only logic, no dynamic allocation in per-tick path, deterministic
 * seeded xorshift PRNG, fixed-capacity projectile arrays.
 * Engine-independent: no UObject, no Unreal types.
 */

#include "AutomataWar/Core/AutomataRules.h"
#include "AutomataWar/Core/Lang/AutomataCompiler.h"
#include "AutomataWar/Core/VM/AutomataVM.h"
#include <array>
#include <vector>
#include <cstdint>
#include <string>

namespace Automata
{

    // --- Grid cell ---------------------------------------------------------------

    /** Canonical integer grid occupancy used for movement and line of sight. */
    enum class CellType : uint8_t
    {
        Empty,
        Wall,
        Cover
    };

    // --- Projectile --------------------------------------------------------------

    /** One fixed-pool projectile in canonical grid coordinates. */
    struct Projectile
    {
        int32_t x = 0;
        int32_t y = 0;
        Dir dir = Dir::North;
        int32_t owner = -1;
        bool active = false;
    };

    // --- Robot state -------------------------------------------------------------

    /** Complete canonical combat and VM state for one robot. */
    struct RobotState
    {
        int32_t x = 0;
        int32_t y = 0;
        Dir facing = Dir::North;
        int32_t hp = MaxHP;
        int32_t energy = MaxEnergy;
        bool shielded = false;
        VMState vm;
    };

    // --- Simulation event --------------------------------------------------------

    /** Replay-visible semantic event categories emitted by simulation effects. */
    enum class EventType : uint8_t
    {
        Move,
        MoveBlockedWall,
        MoveBlockedCover,
        MoveBlockedRobot,
        Turn,
        Scan,
        Fire,
        ShieldActivate,
        ShieldAbsorb,
        Hit,
        ProjectileBlocked,
        Wait,
        Halt,
        EnergyDepleted
    };

    /** Compact event record used by the debugger, VFX, and audio layers. */
    struct SimEvent
    {
        int32_t tick = 0;
        int32_t robot = 0;
        EventType type = EventType::Wait;
        int32_t paramA = 0;
        int32_t paramB = 0;
    };

    // --- Tick snapshot -----------------------------------------------------------

    /** Read-only presentation/debug snapshot captured after one canonical tick. */
    struct TickSnapshot
    {
        int32_t tick = 0;
        std::array<RobotState, 2> robots;
        std::vector<int32_t> obstacleHealth;
        uint64_t stateHash = 0;
    };

    // --- Match result ------------------------------------------------------------

    /** Final winner classification after all ordered tie-break rules. */
    enum class MatchOutcome : uint8_t
    {
        Robot0Wins,
        Robot1Wins,
        Draw
    };

    /** Final match metrics required by gameplay UI and deterministic tests. */
    struct MatchResult
    {
        MatchOutcome outcome = MatchOutcome::Draw;
        int32_t finalTick = 0;
        std::array<int32_t, 2> finalHP = {};
        std::array<int32_t, 2> finalEnergy = {};
        std::array<int32_t, 2> instrCount = {};
    };

    // --- Simulation config -------------------------------------------------------

    /** Explicit deterministic inputs controlling arena dimensions and PRNG seed. */
    struct SimConfig
    {
        int32_t gridWidth = DefaultGridWidth;
        int32_t gridHeight = DefaultGridHeight;
        uint64_t seed = 12345;
    };

    // --- Simulation class --------------------------------------------------------

    /** Headless deterministic match runner with no Unreal world dependency. */
    class Simulation
    {
    public:
        /**
         * Run one complete match from initial state through a win or tick cap.
         * @param programA Compiled behavior for robot zero.
         * @param programB Compiled behavior for robot one.
         * @param config Grid dimensions and deterministic seed.
         * @return Final outcome and tie-break metrics.
         */
        MatchResult RunMatch(const Program &programA, const Program &programB, const SimConfig &config = {});

        /** @return Ordered semantic events from the most recent match. */
        const std::vector<SimEvent> &GetEvents() const { return events_; }
        /** @return Per-tick snapshots from the most recent match. */
        const std::vector<TickSnapshot> &GetSnapshots() const { return snapshots_; }
        /** @return Canonical final-state hash from the most recent match. */
        uint64_t GetFinalHash() const { return finalHash_; }
        /** @return Row-major initial grid generated for the most recent match. */
        const std::vector<CellType> &GetGrid() const { return initialGrid_; }

    private:
        /** Explicit deterministic xorshift64 generator; zero is normalized by setup. */
        struct Xorshift64
        {
            uint64_t state = 1;
            uint64_t Next()
            {
                state ^= state << 13;
                state ^= state >> 7;
                state ^= state << 17;
                return state;
            }
        };

        void InitGrid(int32_t w, int32_t h, Xorshift64 &rng);
        void SpawnRobots(int32_t w, int32_t h);
        void UpdateSystemRegisters(int32_t robotIdx, int32_t tick);
        void ResolveIntent(int32_t robotIdx, const Intent &intent, int32_t tick);
        void AdvanceProjectiles(int32_t tick);
        bool InBounds(int32_t x, int32_t y) const;
        CellType CellAt(int32_t x, int32_t y) const;
        bool HasLOS(int32_t x0, int32_t y0, int32_t x1, int32_t y1) const;
        uint64_t ComputeHash() const;

        int32_t gridWidth_ = 0;
        int32_t gridHeight_ = 0;
        std::vector<CellType> grid_;
        std::vector<CellType> initialGrid_;
        std::vector<int32_t> obstacleHealth_;
        std::array<RobotState, 2> robots_;
        std::array<std::array<Projectile, MaxProjectiles>, 2> projectiles_ = {};
        std::vector<SimEvent> events_;
        std::vector<TickSnapshot> snapshots_;
        uint64_t finalHash_ = 0;
    };

} // namespace Automata

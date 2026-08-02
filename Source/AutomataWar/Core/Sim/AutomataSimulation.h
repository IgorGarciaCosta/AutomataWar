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

enum class CellType : uint8_t
{
    Empty,
    Wall,
    Cover
};

// --- Projectile --------------------------------------------------------------

struct Projectile
{
    int32_t x      = 0;
    int32_t y      = 0;
    Dir     dir    = Dir::North;
    int32_t owner  = -1;
    bool    active = false;
};

// --- Robot state -------------------------------------------------------------

struct RobotState
{
    int32_t x        = 0;
    int32_t y        = 0;
    Dir     facing   = Dir::North;
    int32_t hp       = MaxHP;
    int32_t energy   = MaxEnergy;
    bool    shielded = false;
    VMState vm;
};

// --- Simulation event --------------------------------------------------------

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

struct SimEvent
{
    int32_t   tick   = 0;
    int32_t   robot  = 0;
    EventType type   = EventType::Wait;
    int32_t   paramA = 0;
    int32_t   paramB = 0;
};

// --- Tick snapshot -----------------------------------------------------------

struct TickSnapshot
{
    int32_t tick = 0;
    std::array<RobotState, 2> robots;
    uint64_t stateHash = 0;
};

// --- Match result ------------------------------------------------------------

enum class MatchOutcome : uint8_t
{
    Robot0Wins,
    Robot1Wins,
    Draw
};

struct MatchResult
{
    MatchOutcome outcome    = MatchOutcome::Draw;
    int32_t      finalTick  = 0;
    std::array<int32_t, 2> finalHP     = {};
    std::array<int32_t, 2> finalEnergy = {};
    std::array<int32_t, 2> instrCount  = {};
};

// --- Simulation config -------------------------------------------------------

struct SimConfig
{
    int32_t gridWidth  = DefaultGridWidth;
    int32_t gridHeight = DefaultGridHeight;
    uint64_t seed      = 12345;
};

// --- Simulation class --------------------------------------------------------

class Simulation
{
public:
    MatchResult RunMatch(const Program& programA, const Program& programB, const SimConfig& config = {});

    const std::vector<SimEvent>& GetEvents() const { return events_; }
    const std::vector<TickSnapshot>& GetSnapshots() const { return snapshots_; }
    uint64_t GetFinalHash() const { return finalHash_; }
    const std::vector<CellType>& GetGrid() const { return grid_; }
    int32_t GetGridWidth() const { return gridWidth_; }
    int32_t GetGridHeight() const { return gridHeight_; }

private:
    struct Xorshift64
    {
        uint64_t state = 1;
        uint64_t Next() { state ^= state << 13; state ^= state >> 7; state ^= state << 17; return state; }
    };

    void InitGrid(int32_t w, int32_t h, Xorshift64& rng);
    void SpawnRobots(int32_t w, int32_t h);
    void UpdateSystemRegisters(int32_t robotIdx, int32_t tick);
    void ResolveIntent(int32_t robotIdx, const Intent& intent, int32_t tick);
    void AdvanceProjectiles(int32_t tick);
    bool InBounds(int32_t x, int32_t y) const;
    CellType CellAt(int32_t x, int32_t y) const;
    bool HasLOS(int32_t x0, int32_t y0, int32_t x1, int32_t y1) const;
    uint64_t ComputeHash() const;

    int32_t gridWidth_  = 0;
    int32_t gridHeight_ = 0;
    std::vector<CellType> grid_;
    std::array<RobotState, 2> robots_;
    std::array<std::array<Projectile, MaxProjectiles>, 2> projectiles_ = {};
    std::vector<SimEvent> events_;
    std::vector<TickSnapshot> snapshots_;
    uint64_t finalHash_ = 0;
};

} // namespace Automata

/**
 * @file AutomataSimulation.cpp
 * @brief Implementation of the deterministic Automata War simulation.
 */

#include "AutomataSimulation.h"
#include <algorithm>
#include <cstdlib>

namespace Automata
{

// --- Grid initialization -----------------------------------------------------

void Simulation::InitGrid(int32_t w, int32_t h, Xorshift64& rng)
{
    gridWidth_ = w;
    gridHeight_ = h;
    grid_.assign(static_cast<size_t>(w * h), CellType::Empty);

    for (int32_t y = 1; y < h - 1; ++y)
    {
        for (int32_t x = 1; x < w - 1; ++x)
        {
            if ((x < 3 && y < 3) || (x >= w - 3 && y >= h - 3))
                continue;
            if ((rng.Next() % 100) < 10)
                grid_[static_cast<size_t>(y * w + x)] = CellType::Cover;
        }
    }

    for (int32_t x = 0; x < w; ++x)
    {
        grid_[static_cast<size_t>(x)] = CellType::Wall;
        grid_[static_cast<size_t>((h - 1) * w + x)] = CellType::Wall;
    }
    for (int32_t y = 0; y < h; ++y)
    {
        grid_[static_cast<size_t>(y * w)] = CellType::Wall;
        grid_[static_cast<size_t>(y * w + (w - 1))] = CellType::Wall;
    }
}

void Simulation::SpawnRobots(int32_t w, int32_t h)
{
    robots_[0] = {};
    robots_[0].x = 1;
    robots_[0].y = 1;
    robots_[0].facing = Dir::South;
    robots_[0].hp = MaxHP;
    robots_[0].energy = MaxEnergy;

    robots_[1] = {};
    robots_[1].x = w - 2;
    robots_[1].y = h - 2;
    robots_[1].facing = Dir::North;
    robots_[1].hp = MaxHP;
    robots_[1].energy = MaxEnergy;
}

// --- Utilities ---------------------------------------------------------------

bool Simulation::InBounds(int32_t x, int32_t y) const
{
    return x >= 0 && x < gridWidth_ && y >= 0 && y < gridHeight_;
}

CellType Simulation::CellAt(int32_t x, int32_t y) const
{
    if (!InBounds(x, y)) return CellType::Wall;
    return grid_[static_cast<size_t>(y * gridWidth_ + x)];
}

bool Simulation::HasLOS(int32_t x0, int32_t y0, int32_t x1, int32_t y1) const
{
    int32_t dx = std::abs(x1 - x0), dy = std::abs(y1 - y0);
    int32_t sx = (x0 < x1) ? 1 : -1;
    int32_t sy = (y0 < y1) ? 1 : -1;
    int32_t err = dx - dy;
    int32_t cx = x0, cy = y0;
    while (cx != x1 || cy != y1)
    {
        int32_t e2 = 2 * err;
        if (e2 > -dy) { err -= dy; cx += sx; }
        if (e2 < dx)  { err += dx; cy += sy; }
        if (cx == x1 && cy == y1) break;
        if (InBounds(cx, cy) && grid_[static_cast<size_t>(cy * gridWidth_ + cx)] == CellType::Cover)
            return false;
    }
    return true;
}

uint64_t Simulation::ComputeHash() const
{
    uint64_t h = 14695981039346656037ULL;
    auto mix = [&](int64_t v) {
        for (int i = 0; i < 8; ++i) { h ^= static_cast<uint8_t>(v >> (i*8)); h *= 1099511628211ULL; }
    };
    mix(gridWidth_); mix(gridHeight_);
    for (CellType cell : grid_) mix(static_cast<int64_t>(cell));
    for (int32_t i = 0; i < 2; ++i)
    {
        mix(robots_[i].x); mix(robots_[i].y);
        mix(static_cast<int64_t>(robots_[i].facing));
        mix(robots_[i].hp); mix(robots_[i].energy);
        mix(robots_[i].shielded ? 1 : 0);
        mix(robots_[i].vm.pc); mix(robots_[i].vm.busyLeft);
        mix(robots_[i].vm.currentInstruction);
        mix(robots_[i].vm.halted ? 1 : 0);
        mix(robots_[i].vm.energyInert ? 1 : 0);
        mix(robots_[i].vm.instrExecCount);
        for (auto r : robots_[i].vm.regs) mix(r);
    }
    for (int32_t i = 0; i < 2; ++i)
        for (const auto& p : projectiles_[i])
        {
            mix(p.active ? 1 : 0);
            mix(p.x); mix(p.y); mix(static_cast<int64_t>(p.dir)); mix(p.owner);
        }
    return h;
}

// --- System registers --------------------------------------------------------

void Simulation::UpdateSystemRegisters(int32_t robotIdx, int32_t tick)
{
    auto& r = robots_[robotIdx];
    auto& o = robots_[1 - robotIdx];
    r.vm.regs[static_cast<int>(Reg::R_HP)]     = r.hp;
    r.vm.regs[static_cast<int>(Reg::R_ENERGY)] = r.energy;
    r.vm.regs[static_cast<int>(Reg::R_TICK)]   = tick;
    // R_ENEMY_DIST and R_ENEMY_DIR are updated by SCAN only (not always-visible).
    // Keep current values from last scan (initialized to 0).
}

// --- Intent resolution -------------------------------------------------------

void Simulation::ResolveIntent(int32_t robotIdx, const Intent& intent, int32_t tick)
{
    auto& r = robots_[robotIdx];

    switch (intent.type)
    {
    case IntentType::Move:
    {
        // param: 0=FWD, 1=BACK
        int32_t facingIdx = static_cast<int32_t>(r.facing);
        int32_t dirMul = (intent.param == 0) ? 1 : -1;
        int32_t nx = r.x + DirDX[facingIdx] * dirMul;
        int32_t ny = r.y + DirDY[facingIdx] * dirMul;

        if (!InBounds(nx, ny))
        {
            events_.push_back({tick, robotIdx, EventType::MoveBlockedWall, nx, ny});
        }
        else
        {
            CellType cell = CellAt(nx, ny);
            if (cell == CellType::Wall)
            {
                events_.push_back({tick, robotIdx, EventType::MoveBlockedWall, nx, ny});
            }
            else if (cell == CellType::Cover)
            {
                events_.push_back({tick, robotIdx, EventType::MoveBlockedCover, nx, ny});
            }
            else
            {
                // Check robot collision.
                auto& other = robots_[1 - robotIdx];
                if (other.x == nx && other.y == ny)
                {
                    events_.push_back({tick, robotIdx, EventType::MoveBlockedRobot, nx, ny});
                }
                else
                {
                    r.x = nx;
                    r.y = ny;
                    events_.push_back({tick, robotIdx, EventType::Move, nx, ny});
                }
            }
        }
        break;
    }
    case IntentType::Turn:
    {
        int32_t d = (static_cast<int32_t>(r.facing) + intent.param + 4) % 4;
        r.facing = static_cast<Dir>(d);
        events_.push_back({tick, robotIdx, EventType::Turn, d, 0});
        break;
    }
    case IntentType::Scan:
    {
        // 90-degree cardinal cone forward, range ScanRange, cover blocks LOS.
        auto& other = robots_[1 - robotIdx];
        int32_t dx = other.x - r.x;
        int32_t dy = other.y - r.y;
        int32_t dist = std::abs(dx) + std::abs(dy);
        bool detected = false;
        if (dist <= ScanRange && dist > 0)
        {
            Dir facing = r.facing;
            bool inCone = false;
            switch (facing)
            {
            case Dir::North: inCone = (dy < 0 && std::abs(dx) <= std::abs(dy)); break;
            case Dir::South: inCone = (dy > 0 && std::abs(dx) <= std::abs(dy)); break;
            case Dir::East:  inCone = (dx > 0 && std::abs(dy) <= std::abs(dx)); break;
            case Dir::West:  inCone = (dx < 0 && std::abs(dy) <= std::abs(dx)); break;
            }
            if (inCone && HasLOS(r.x, r.y, other.x, other.y))
                detected = true;
        }

        if (detected)
        {
            // The signed cross product gives the enemy's side relative to facing.
            r.vm.regs[static_cast<int>(Reg::R_ENEMY_DIST)] = dist;
            const int32_t facingIndex = static_cast<int32_t>(r.facing);
            const int32_t cross = DirDX[facingIndex] * dy - DirDY[facingIndex] * dx;
            r.vm.regs[static_cast<int>(Reg::R_ENEMY_DIR)] =
                cross < 0 ? static_cast<int32_t>(RelativeEnemyDir::Left) :
                cross > 0 ? static_cast<int32_t>(RelativeEnemyDir::Right) :
                            static_cast<int32_t>(RelativeEnemyDir::Ahead);
        }
        else
        {
            // Miss: R_ENEMY_DIST = 0.
            r.vm.regs[static_cast<int>(Reg::R_ENEMY_DIST)] = 0;
        }
        events_.push_back({tick, robotIdx, EventType::Scan, detected ? 1 : 0, 0});
        break;
    }
    case IntentType::Fire:
    {
        for (auto& p : projectiles_[robotIdx])
        {
            if (!p.active)
            {
                p.active = true;
                p.x = r.x;
                p.y = r.y;
                p.dir = r.facing;
                p.owner = robotIdx;
                break;
            }
        }
        events_.push_back({tick, robotIdx, EventType::Fire, 0, 0});
        break;
    }
    case IntentType::Shield:
        r.shielded = true;
        events_.push_back({tick, robotIdx, EventType::ShieldActivate, 0, 0});
        break;

    case IntentType::Wait:
        events_.push_back({tick, robotIdx, EventType::Wait, 0, 0});
        break;

    case IntentType::None:
        break;
    }
}

// --- Projectile advancement --------------------------------------------------

void Simulation::AdvanceProjectiles(int32_t tick)
{
    for (int32_t owner = 0; owner < 2; ++owner)
    {
        for (auto& p : projectiles_[owner])
        {
            if (!p.active) continue;
            for (int32_t step = 0; step < ProjectileSpeed; ++step)
            {
                if (!p.active) break;
                int32_t target = 1 - owner;
                p.x += DirDX[static_cast<int>(p.dir)];
                p.y += DirDY[static_cast<int>(p.dir)];

                if (!InBounds(p.x, p.y))
                {
                    p.active = false;
                    break;
                }
                CellType cell = grid_[static_cast<size_t>(p.y * gridWidth_ + p.x)];
                if (cell == CellType::Wall || cell == CellType::Cover)
                {
                    events_.push_back({tick, owner, EventType::ProjectileBlocked, p.x, p.y});
                    p.active = false;
                    break;
                }

                if (p.x == robots_[target].x && p.y == robots_[target].y)
                {
                    if (robots_[target].shielded)
                    {
                        robots_[target].shielded = false;
                        events_.push_back({tick, target, EventType::ShieldAbsorb, owner, 0});
                    }
                    else
                    {
                        robots_[target].hp -= ProjectileDamage;
                        events_.push_back({tick, target, EventType::Hit, ProjectileDamage, owner});
                    }
                    p.active = false;
                    break;
                }
            }
        }
    }
}

// --- Match execution ---------------------------------------------------------

MatchResult Simulation::RunMatch(const Program& programA, const Program& programB, const SimConfig& config)
{
    Xorshift64 rng{config.seed};
    InitGrid(config.gridWidth, config.gridHeight, rng);
    SpawnRobots(config.gridWidth, config.gridHeight);

    events_.clear();
    events_.reserve(static_cast<size_t>(TickCap * (2 + 2 * MaxProjectiles)));
    snapshots_.clear();
    snapshots_.reserve(TickCap);
    projectiles_ = {};

    const Program* programs[2] = {&programA, &programB};

    for (int32_t tick = 0; tick < TickCap; ++tick)
    {
        // Fair alternating: first-mover alternates each tick.
        int32_t first  = tick % 2;
        int32_t second = 1 - first;

        for (int32_t pass = 0; pass < 2; ++pass)
        {
            int32_t idx = (pass == 0) ? first : second;
            auto& r = robots_[idx];

            if (r.hp <= 0) continue;

            UpdateSystemRegisters(idx, tick);

            if (r.energy <= 0)
            {
                if (!r.vm.energyInert)
                    events_.push_back({tick, idx, EventType::EnergyDepleted, 0, 0});
                r.vm.energyInert = true;
                ResolveIntent(idx, {IntentType::Wait, 0}, tick);
                continue;
            }

            const Program& program = *programs[idx];
            int32_t pendingCost = 0;
            if (!r.vm.halted && r.vm.busyLeft == 0 && !program.code.empty())
            {
                const size_t instructionIndex = static_cast<size_t>(r.vm.pc) % program.code.size();
                const int32_t opcodeIndex = static_cast<int32_t>(program.code[instructionIndex].opcode);
                if (opcodeIndex >= 0 && opcodeIndex < OpcodeCount)
                {
                    pendingCost = EnergyCost[opcodeIndex];
                    if (pendingCost > r.energy)
                    {
                        r.energy = 0;
                        r.vm.energyInert = true;
                        events_.push_back({tick, idx, EventType::EnergyDepleted, 0, 0});
                        ResolveIntent(idx, {IntentType::Wait, 0}, tick);
                        continue;
                    }
                }
            }

            const int32_t previousInstructionCount = r.vm.instrExecCount;
            Intent intent = VMTick(r.vm, program);
            if (r.vm.instrExecCount != previousInstructionCount)
                r.energy -= pendingCost;

            ResolveIntent(idx, intent, tick);
        }

        AdvanceProjectiles(tick);

        // Clamp HP.
        for (auto& r : robots_)
            if (r.hp < 0) r.hp = 0;

        // Snapshots expose system registers after every effect in this tick.
        UpdateSystemRegisters(0, tick);
        UpdateSystemRegisters(1, tick);

        // Snapshot.
        TickSnapshot snap;
        snap.tick = tick;
        snap.robots = robots_;
        snap.stateHash = ComputeHash();
        snapshots_.push_back(snap);

        if (robots_[0].hp <= 0 || robots_[1].hp <= 0)
            break;
    }

    finalHash_ = ComputeHash();

    MatchResult result;
    result.finalTick = static_cast<int32_t>(snapshots_.size());
    result.finalHP = {robots_[0].hp, robots_[1].hp};
    result.finalEnergy = {robots_[0].energy, robots_[1].energy};
    result.instrCount = {robots_[0].vm.instrExecCount, robots_[1].vm.instrExecCount};

    if (robots_[0].hp > robots_[1].hp)
        result.outcome = MatchOutcome::Robot0Wins;
    else if (robots_[1].hp > robots_[0].hp)
        result.outcome = MatchOutcome::Robot1Wins;
    else if (robots_[0].energy > robots_[1].energy)
        result.outcome = MatchOutcome::Robot0Wins;
    else if (robots_[1].energy > robots_[0].energy)
        result.outcome = MatchOutcome::Robot1Wins;
    else if (robots_[0].vm.instrExecCount < robots_[1].vm.instrExecCount)
        result.outcome = MatchOutcome::Robot0Wins;
    else if (robots_[1].vm.instrExecCount < robots_[0].vm.instrExecCount)
        result.outcome = MatchOutcome::Robot1Wins;
    else
        result.outcome = MatchOutcome::Draw;

    return result;
}

} // namespace Automata

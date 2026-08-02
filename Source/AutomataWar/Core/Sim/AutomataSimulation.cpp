/**
 * @file AutomataSimulation.cpp
 * @brief Implementation of the deterministic Automata War simulation.
 */

#include "AutomataSimulation.h"
#include <cmath>
#include <algorithm>

namespace Automata
{

// ─── Grid initialization ─────────────────────────────────────────────────────

void Simulation::InitGrid(int32_t w, int32_t h, Xorshift64& rng)
{
    gridWidth_ = w;
    gridHeight_ = h;
    grid_.assign(static_cast<size_t>(w * h), CellType::Empty);

    // Place deterministic cover: ~10% of interior cells.
    for (int32_t y = 1; y < h - 1; ++y)
    {
        for (int32_t x = 1; x < w - 1; ++x)
        {
            // Keep spawn corners clear (2x2 in each corner).
            if ((x < 3 && y < 3) || (x >= w - 3 && y >= h - 3))
                continue;
            if ((rng.Next() % 100) < 10)
                grid_[static_cast<size_t>(y * w + x)] = CellType::Cover;
        }
    }

    // Border walls.
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
    // Robot 0: top-left corner facing south-east (South).
    robots_[0] = {};
    robots_[0].x = 1;
    robots_[0].y = 1;
    robots_[0].facing = Dir::South;
    robots_[0].hp = MaxHP;
    robots_[0].energy = MaxEnergy;

    // Robot 1: bottom-right corner facing north-west (North).
    robots_[1] = {};
    robots_[1].x = w - 2;
    robots_[1].y = h - 2;
    robots_[1].facing = Dir::North;
    robots_[1].hp = MaxHP;
    robots_[1].energy = MaxEnergy;
}

// ─── Utilities ───────────────────────────────────────────────────────────────

bool Simulation::InBounds(int32_t x, int32_t y) const
{
    return x >= 0 && x < gridWidth_ && y >= 0 && y < gridHeight_;
}

bool Simulation::IsBlocked(int32_t x, int32_t y) const
{
    if (!InBounds(x, y)) return true;
    CellType c = grid_[static_cast<size_t>(y * gridWidth_ + x)];
    if (c == CellType::Wall || c == CellType::Cover) return true;
    // Check robot occupation.
    for (int32_t i = 0; i < 2; ++i)
        if (robots_[i].x == x && robots_[i].y == y) return true;
    return false;
}

bool Simulation::HasLOS(int32_t x0, int32_t y0, int32_t x1, int32_t y1) const
{
    // Bresenham line; cover blocks LOS.
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
    for (int32_t i = 0; i < 2; ++i)
    {
        mix(robots_[i].x); mix(robots_[i].y);
        mix(static_cast<int64_t>(robots_[i].facing));
        mix(robots_[i].hp); mix(robots_[i].energy);
        mix(robots_[i].shielded ? 1 : 0);
        mix(robots_[i].vm.pc); mix(robots_[i].vm.busyLeft);
        for (auto r : robots_[i].vm.regs) mix(r);
    }
    for (int32_t i = 0; i < 2; ++i)
        for (auto& p : projectiles_[i])
            if (p.active) { mix(p.x); mix(p.y); mix(static_cast<int64_t>(p.dir)); }
    return h;
}

// ─── System registers ────────────────────────────────────────────────────────

void Simulation::UpdateSystemRegisters(int32_t robotIdx, int32_t tick)
{
    auto& r = robots_[robotIdx];
    auto& o = robots_[1 - robotIdx];
    r.vm.regs[static_cast<int>(Reg::R_HP)]         = r.hp;
    r.vm.regs[static_cast<int>(Reg::R_ENERGY)]     = r.energy;
    r.vm.regs[static_cast<int>(Reg::R_TICK)]       = tick;
    r.vm.regs[static_cast<int>(Reg::R_ENEMY_DIST)] = std::abs(r.x - o.x) + std::abs(r.y - o.y);

    // Relative direction: which cardinal direction is the opponent in?
    int32_t dx = o.x - r.x;
    int32_t dy = o.y - r.y;
    Dir eDir;
    if (std::abs(dx) >= std::abs(dy))
        eDir = (dx > 0) ? Dir::East : Dir::West;
    else
        eDir = (dy > 0) ? Dir::South : Dir::North;
    r.vm.regs[static_cast<int>(Reg::R_ENEMY_DIR)] = static_cast<int32_t>(eDir);
}

// ─── Intent resolution ───────────────────────────────────────────────────────

void Simulation::ResolveIntent(int32_t robotIdx, const Intent& intent, int32_t tick)
{
    auto& r = robots_[robotIdx];

    switch (intent.type)
    {
    case IntentType::Move:
    {
        int32_t nx = r.x + DirDX[static_cast<int>(r.facing)];
        int32_t ny = r.y + DirDY[static_cast<int>(r.facing)];
        if (!InBounds(nx, ny) || grid_[static_cast<size_t>(ny * gridWidth_ + nx)] != CellType::Empty)
        {
            events_.push_back({tick, robotIdx, EventType::MoveBlocked, nx, ny});
        }
        else
        {
            // Check robot collision.
            auto& other = robots_[1 - robotIdx];
            if (other.x == nx && other.y == ny)
            {
                events_.push_back({tick, robotIdx, EventType::MoveBlocked, nx, ny});
            }
            else
            {
                r.x = nx;
                r.y = ny;
                events_.push_back({tick, robotIdx, EventType::Move, nx, ny});
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
        // 90-degree cardinal cone, range ScanRange, requires LOS.
        auto& other = robots_[1 - robotIdx];
        int32_t dx = other.x - r.x;
        int32_t dy = other.y - r.y;
        int32_t dist = std::abs(dx) + std::abs(dy);
        bool detected = false;
        if (dist <= ScanRange && dist > 0)
        {
            // Check if enemy is in the cone (within 45 degrees of facing).
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
        r.vm.regs[0] = detected ? 1 : 0;
        events_.push_back({tick, robotIdx, EventType::Scan, detected ? 1 : 0, 0});
        break;
    }
    case IntentType::Fire:
    {
        // Find free projectile slot.
        for (auto& p : projectiles_[robotIdx])
        {
            if (!p.active)
            {
                p.active = true;
                p.x = r.x + DirDX[static_cast<int>(r.facing)];
                p.y = r.y + DirDY[static_cast<int>(r.facing)];
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

// ─── Projectile advancement ──────────────────────────────────────────────────

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
                // Check hit on robots at current position before moving.
                int32_t target = 1 - owner;
                if (p.x == robots_[target].x && p.y == robots_[target].y)
                {
                    if (robots_[target].shielded)
                    {
                        robots_[target].shielded = false;
                        events_.push_back({tick, target, EventType::ShieldAbsorb, 0, 0});
                    }
                    else
                    {
                        robots_[target].hp -= ProjectileDamage;
                        events_.push_back({tick, target, EventType::Hit, ProjectileDamage, 0});
                    }
                    p.active = false;
                    break;
                }

                // Advance.
                p.x += DirDX[static_cast<int>(p.dir)];
                p.y += DirDY[static_cast<int>(p.dir)];

                // Bounds/cover/wall check.
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

                // Hit check after move.
                if (p.x == robots_[target].x && p.y == robots_[target].y)
                {
                    if (robots_[target].shielded)
                    {
                        robots_[target].shielded = false;
                        events_.push_back({tick, target, EventType::ShieldAbsorb, 0, 0});
                    }
                    else
                    {
                        robots_[target].hp -= ProjectileDamage;
                        events_.push_back({tick, target, EventType::Hit, ProjectileDamage, 0});
                    }
                    p.active = false;
                    break;
                }
            }
        }
    }
}

// ─── Match execution ─────────────────────────────────────────────────────────

MatchResult Simulation::RunMatch(const Program& programA, const Program& programB, const SimConfig& config)
{
    Xorshift64 rng{config.seed};
    InitGrid(config.gridWidth, config.gridHeight, rng);
    SpawnRobots(config.gridWidth, config.gridHeight);

    events_.clear();
    snapshots_.clear();
    instrCount_ = {0, 0};
    projectiles_ = {};

    const Program* programs[2] = {&programA, &programB};

    for (int32_t tick = 0; tick < TickCap; ++tick)
    {
        // Determine execution order: robot (tick % 2) goes first.
        int32_t first  = tick % 2;
        int32_t second = 1 - first;

        for (int32_t pass = 0; pass < 2; ++pass)
        {
            int32_t idx = (pass == 0) ? first : second;
            auto& r = robots_[idx];

            if (r.hp <= 0) continue;

            UpdateSystemRegisters(idx, tick);

            // Check energy.
            Intent intent = VMTick(r.vm, *programs[idx]);
            if (intent.type != IntentType::None && intent.type != IntentType::Wait)
            {
                int32_t opIdx = -1;
                switch (intent.type)
                {
                case IntentType::Move:   opIdx = static_cast<int>(Opcode::MOVE); break;
                case IntentType::Turn:   opIdx = static_cast<int>(Opcode::TURN); break;
                case IntentType::Scan:   opIdx = static_cast<int>(Opcode::SCAN); break;
                case IntentType::Fire:   opIdx = static_cast<int>(Opcode::FIRE); break;
                case IntentType::Shield: opIdx = static_cast<int>(Opcode::SHIELD); break;
                default: break;
                }
                if (opIdx >= 0 && r.energy < EnergyCost[opIdx])
                {
                    events_.push_back({tick, idx, EventType::EnergyDepleted, 0, 0});
                    intent = {IntentType::Wait, 0};
                }
                else if (opIdx >= 0)
                {
                    r.energy -= EnergyCost[opIdx];
                }
            }

            if (intent.type != IntentType::None)
                instrCount_[idx]++;

            ResolveIntent(idx, intent, tick);
        }

        // Advance projectiles.
        AdvanceProjectiles(tick);

        // Clamp HP.
        for (auto& r : robots_)
            if (r.hp < 0) r.hp = 0;

        // Snapshot.
        TickSnapshot snap;
        snap.tick = tick;
        snap.robots = robots_;
        snap.stateHash = ComputeHash();
        snapshots_.push_back(snap);

        // Check termination.
        if (robots_[0].hp <= 0 || robots_[1].hp <= 0)
            break;
    }

    finalHash_ = ComputeHash();

    // Determine outcome.
    MatchResult result;
    result.finalTick = static_cast<int32_t>(snapshots_.size());
    result.finalHP = {robots_[0].hp, robots_[1].hp};
    result.finalEnergy = {robots_[0].energy, robots_[1].energy};
    result.instrCount = instrCount_;

    if (robots_[0].hp > robots_[1].hp)
        result.outcome = MatchOutcome::Robot0Wins;
    else if (robots_[1].hp > robots_[0].hp)
        result.outcome = MatchOutcome::Robot1Wins;
    else if (robots_[0].energy > robots_[1].energy)
        result.outcome = MatchOutcome::Robot0Wins;
    else if (robots_[1].energy > robots_[0].energy)
        result.outcome = MatchOutcome::Robot1Wins;
    else if (instrCount_[0] < instrCount_[1])
        result.outcome = MatchOutcome::Robot0Wins;
    else if (instrCount_[1] < instrCount_[0])
        result.outcome = MatchOutcome::Robot1Wins;
    else
        result.outcome = MatchOutcome::Draw;

    return result;
}

} // namespace Automata

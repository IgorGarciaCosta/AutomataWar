/**
 * @file AutomataSimulation.cpp
 * @brief Direct finite-command simulation implementation.
 */

#include "AutomataSimulation.h"
#include <algorithm>

namespace Automata
{

    void Simulation::InitGrid(int32_t Width, int32_t Height, Xorshift64 &Rng)
    {
        gridWidth_ = Width;
        gridHeight_ = Height;
        grid_.assign(static_cast<size_t>(Width * Height), CellType::Empty);

        for (int32_t Y = 1; Y < Height - 1; ++Y)
        {
            for (int32_t X = 1; X < Width - 1; ++X)
            {
                if ((X < 3 && Y < 3) || (X >= Width - 3 && Y >= Height - 3))
                    continue;
                if ((Rng.Next() % 100) < 10)
                    grid_[static_cast<size_t>(Y * Width + X)] = CellType::Cover;
            }
        }

        for (int32_t X = 0; X < Width; ++X)
        {
            grid_[static_cast<size_t>(X)] = CellType::Wall;
            grid_[static_cast<size_t>((Height - 1) * Width + X)] = CellType::Wall;
        }
        for (int32_t Y = 0; Y < Height; ++Y)
        {
            grid_[static_cast<size_t>(Y * Width)] = CellType::Wall;
            grid_[static_cast<size_t>(Y * Width + Width - 1)] = CellType::Wall;
        }

        actionPointItemValues_.assign(grid_.size(), 0);
        std::vector<size_t> EmptyCells;
        for (int32_t Y = 1; Y < Height - 1; ++Y)
            for (int32_t X = 1; X < Width - 1; ++X)
            {
                const size_t CellIndex = static_cast<size_t>(Y * Width + X);
                const bool bRobotStart = (X == 1 && Y == 1) || (X == Width - 2 && Y == Height - 2);
                if (!bRobotStart && grid_[CellIndex] == CellType::Empty)
                    EmptyCells.push_back(CellIndex);
            }

        auto SpawnItems = [&](CellType Type, int32_t RequestedCount)
        {
            const int32_t ItemCount = std::min<int32_t>(RequestedCount, static_cast<int32_t>(EmptyCells.size()));
            for (int32_t ItemIndex = 0; ItemIndex < ItemCount; ++ItemIndex)
            {
                const size_t CandidateIndex = static_cast<size_t>(Rng.Next() % EmptyCells.size());
                const size_t CellIndex = EmptyCells[CandidateIndex];
                grid_[CellIndex] = Type;
                if (Type == CellType::ActionPointItem)
                    actionPointItemValues_[CellIndex] = ActionPointItemMinValue +
                                                        static_cast<int32_t>(Rng.Next() % (ActionPointItemMaxValue - ActionPointItemMinValue + 1));
                EmptyCells[CandidateIndex] = EmptyCells.back();
                EmptyCells.pop_back();
            }
        };
        SpawnItems(CellType::ActionPointItem, ActionPointItemCount);
        SpawnItems(CellType::ExtraAmmoItem, ExtraAmmoItemCount);
        SpawnItems(CellType::ShieldItem, ShieldItemCount);
        SpawnItems(CellType::AcceleratorItem, AcceleratorItemCount);

        initialGrid_ = grid_;
        obstacleHealth_.assign(grid_.size(), 0);
        for (size_t CellIndex = 0; CellIndex < grid_.size(); ++CellIndex)
            if (grid_[CellIndex] == CellType::Cover)
                obstacleHealth_[CellIndex] = ObstacleMaxHealth;
    }

    void Simulation::SpawnRobots(int32_t Width, int32_t Height, const SimConfig &Config)
    {
        robots_[0] = {};
        robots_[0].x = 1;
        robots_[0].y = 1;
        robots_[0].facing = Dir::South;
        robots_[0].actionPoints = Config.initialActionPoints[0];
        robots_[0].effects = Config.initialEffects[0];

        robots_[1] = {};
        robots_[1].x = Width - 2;
        robots_[1].y = Height - 2;
        robots_[1].facing = Dir::North;
        robots_[1].actionPoints = Config.initialActionPoints[1];
        robots_[1].effects = Config.initialEffects[1];
    }

    bool Simulation::InBounds(int32_t X, int32_t Y) const
    {
        return X >= 0 && X < gridWidth_ && Y >= 0 && Y < gridHeight_;
    }

    CellType Simulation::CellAt(int32_t X, int32_t Y) const
    {
        return InBounds(X, Y) ? grid_[static_cast<size_t>(Y * gridWidth_ + X)] : CellType::Wall;
    }

    void Simulation::Fire(int32_t RobotIndex, int32_t Step)
    {
        RobotState &Robot = robots_[RobotIndex];
        const int32_t TargetIndex = 1 - RobotIndex;
        RobotState &Target = robots_[TargetIndex];
        const int32_t Direction = static_cast<int32_t>(Robot.facing);
        const int32_t Damage = ProjectileDamage + (Robot.effects.ExtraAmmoRounds > 0 ? ExtraAmmoDamageBonus : 0);
        int32_t X = Robot.x;
        int32_t Y = Robot.y;

        events_.push_back({Step, RobotIndex, EventType::Fire, 0, 0});
        while (true)
        {
            X += DirDX[Direction];
            Y += DirDY[Direction];
            if (!InBounds(X, Y) || CellAt(X, Y) == CellType::Wall)
            {
                events_.push_back({Step, RobotIndex, EventType::ShotBlocked, X, Y});
                return;
            }

            const size_t CellIndex = static_cast<size_t>(Y * gridWidth_ + X);
            if (grid_[CellIndex] == CellType::Cover)
            {
                obstacleHealth_[CellIndex] = std::max(0, obstacleHealth_[CellIndex] - Damage);
                if (obstacleHealth_[CellIndex] == 0)
                    grid_[CellIndex] = CellType::Empty;
                events_.push_back({Step, RobotIndex, EventType::ShotBlocked, X, Y});
                return;
            }

            if (Target.x == X && Target.y == Y)
            {
                int32_t AppliedDamage = Damage;
                if (Target.effects.bShieldCharged || Target.effects.ShieldRounds > 0)
                {
                    AppliedDamage = (Damage + 1) / 2;
                    Target.effects.bShieldCharged = false;
                    events_.push_back({Step, TargetIndex, EventType::ShieldAbsorbed, Damage - AppliedDamage, RobotIndex});
                }
                Target.hp = std::max(0, Target.hp - AppliedDamage);
                events_.push_back({Step, TargetIndex, EventType::Hit, AppliedDamage, RobotIndex});
                return;
            }
        }
    }

    void Simulation::CollectItem(int32_t RobotIndex, size_t CellIndex, int32_t Step)
    {
        RobotState &Robot = robots_[RobotIndex];
        const CellType Item = grid_[CellIndex];
        switch (Item)
        {
        case CellType::ActionPointItem:
        {
            const int32_t Award = actionPointItemValues_[CellIndex];
            Robot.actionPoints += Award;
            actionPointItemValues_[CellIndex] = 0;
            events_.push_back({Step, RobotIndex, EventType::ActionPointsCollected,
                               static_cast<int32_t>(CellIndex), Award});
            break;
        }
        case CellType::ExtraAmmoItem:
            Robot.effects.ExtraAmmoRounds = PowerUpDurationRounds;
            events_.push_back({Step, RobotIndex, EventType::ExtraAmmoCollected, static_cast<int32_t>(CellIndex), PowerUpDurationRounds});
            break;
        case CellType::ShieldItem:
            Robot.effects.ShieldRounds = PowerUpDurationRounds;
            events_.push_back({Step, RobotIndex, EventType::ShieldCollected, static_cast<int32_t>(CellIndex), PowerUpDurationRounds});
            break;
        case CellType::AcceleratorItem:
            Robot.effects.AcceleratorRounds = PowerUpDurationRounds;
            events_.push_back({Step, RobotIndex, EventType::AcceleratorCollected, static_cast<int32_t>(CellIndex), PowerUpDurationRounds});
            break;
        default:
            return;
        }
        grid_[CellIndex] = CellType::Empty;
    }

    void Simulation::Move(int32_t RobotIndex, int32_t Step)
    {
        RobotState &Robot = robots_[RobotIndex];
        const int32_t Direction = static_cast<int32_t>(Robot.facing);
        const bool bAccelerated = Robot.effects.bAccelerateNextMove || Robot.effects.AcceleratorRounds > 0;
        Robot.effects.bAccelerateNextMove = false;

        for (int32_t Cell = 0; Cell < (bAccelerated ? 2 : 1); ++Cell)
        {
            const int32_t X = Robot.x + DirDX[Direction];
            const int32_t Y = Robot.y + DirDY[Direction];
            const CellType Destination = CellAt(X, Y);
            if (Destination == CellType::Wall)
            {
                events_.push_back({Step, RobotIndex, EventType::MoveBlockedWall, X, Y});
                return;
            }
            if (Destination == CellType::Cover)
            {
                events_.push_back({Step, RobotIndex, EventType::MoveBlockedCover, X, Y});
                return;
            }
            if (robots_[1 - RobotIndex].x == X && robots_[1 - RobotIndex].y == Y)
            {
                events_.push_back({Step, RobotIndex, EventType::MoveBlockedRobot, X, Y});
                return;
            }

            Robot.x = X;
            Robot.y = Y;
            events_.push_back({Step, RobotIndex, EventType::Move, X, Y});
            CollectItem(RobotIndex, static_cast<size_t>(Y * gridWidth_ + X), Step);
        }
    }

    void Simulation::ExecuteCommand(int32_t RobotIndex, EAWCommand Command, int32_t Step)
    {
        RobotState &Robot = robots_[RobotIndex];
        switch (Command)
        {
        case EAWCommand::Move:
            Move(RobotIndex, Step);
            break;
        case EAWCommand::Fire:
            Fire(RobotIndex, Step);
            break;
        case EAWCommand::TurnLeft:
            Robot.facing = static_cast<Dir>((static_cast<int32_t>(Robot.facing) + 3) % 4);
            events_.push_back({Step, RobotIndex, EventType::Turn, -1, 0});
            break;
        case EAWCommand::TurnRight:
            Robot.facing = static_cast<Dir>((static_cast<int32_t>(Robot.facing) + 1) % 4);
            events_.push_back({Step, RobotIndex, EventType::Turn, 1, 0});
            break;
        case EAWCommand::Wait:
            events_.push_back({Step, RobotIndex, EventType::Wait, 0, 0});
            break;
        case EAWCommand::ChargeShield:
            Robot.effects.bShieldCharged = true;
            events_.push_back({Step, RobotIndex, EventType::ShieldCharged, 0, 0});
            break;
        case EAWCommand::Accelerate:
            Robot.effects.bAccelerateNextMove = true;
            events_.push_back({Step, RobotIndex, EventType::AccelerationCharged, 0, 0});
            break;
        default:
            break;
        }
    }

    uint64_t Simulation::ComputeHash() const
    {
        uint64_t Hash = 14695981039346656037ULL;
        auto Mix = [&](int64_t Value)
        {
            for (int32_t Byte = 0; Byte < 8; ++Byte)
            {
                Hash ^= static_cast<uint8_t>(Value >> (Byte * 8));
                Hash *= 1099511628211ULL;
            }
        };

        Mix(gridWidth_);
        Mix(gridHeight_);
        for (size_t CellIndex = 0; CellIndex < grid_.size(); ++CellIndex)
        {
            Mix(static_cast<int64_t>(grid_[CellIndex]));
            Mix(obstacleHealth_[CellIndex]);
            Mix(actionPointItemValues_[CellIndex]);
        }
        for (const RobotState &Robot : robots_)
        {
            Mix(Robot.x);
            Mix(Robot.y);
            Mix(static_cast<int64_t>(Robot.facing));
            Mix(Robot.hp);
            Mix(Robot.actionPoints);
            Mix(Robot.currentCommand);
            Mix(Robot.nextCommand);
            Mix(Robot.effects.bShieldCharged);
            Mix(Robot.effects.bAccelerateNextMove);
            Mix(Robot.effects.ExtraAmmoRounds);
            Mix(Robot.effects.ShieldRounds);
            Mix(Robot.effects.AcceleratorRounds);
        }
        return Hash;
    }

    MatchResult Simulation::RunMatch(TConstArrayView<EAWCommand> CommandsA, TConstArrayView<EAWCommand> CommandsB, const SimConfig &Config)
    {
        Xorshift64 Rng{Config.seed == 0 ? 1 : Config.seed};
        InitGrid(Config.gridWidth, Config.gridHeight, Rng);
        SpawnRobots(Config.gridWidth, Config.gridHeight, Config);
        events_.clear();
        snapshots_.clear();

        const TConstArrayView<EAWCommand> Commands[2] = {CommandsA, CommandsB};
        const int32_t StepCount = CommandsA.Num() + CommandsB.Num();
        events_.reserve(static_cast<size_t>(StepCount * 4));
        snapshots_.reserve(static_cast<size_t>(StepCount));
        const int32_t StartingRobot = Config.startingRobot == 1 ? 1 : 0;
        int32_t Step = 0;

        for (int32_t Turn = 0; Turn < 2; ++Turn)
        {
            const int32_t RobotIndex = Turn == 0 ? StartingRobot : 1 - StartingRobot;
            RobotState &Robot = robots_[RobotIndex];
            if (Robot.hp <= 0)
                break;

            while (Robot.nextCommand < Commands[RobotIndex].Num())
            {
                robots_[0].currentCommand = -1;
                robots_[1].currentCommand = -1;
                Robot.currentCommand = Robot.nextCommand;
                ExecuteCommand(RobotIndex, Commands[RobotIndex][Robot.nextCommand], Step);
                ++Robot.nextCommand;

                StepSnapshot Snapshot;
                Snapshot.step = Step++;
                Snapshot.robots = robots_;
                Snapshot.obstacleHealth = obstacleHealth_;
                Snapshot.stateHash = ComputeHash();
                snapshots_.push_back(MoveTemp(Snapshot));

                if (robots_[0].hp <= 0 || robots_[1].hp <= 0)
                    break;
            }

            if (robots_[0].hp <= 0 || robots_[1].hp <= 0)
                break;
        }

        finalHash_ = ComputeHash();
        MatchResult Result;
        Result.stepsExecuted = static_cast<int32_t>(snapshots_.size());
        Result.finalHP = {robots_[0].hp, robots_[1].hp};
        Result.finalActionPoints = {robots_[0].actionPoints, robots_[1].actionPoints};
        for (int32_t RobotIndex = 0; RobotIndex < 2; ++RobotIndex)
        {
            Result.finalEffects[RobotIndex] = robots_[RobotIndex].effects;
            FAWRobotEffects &Effects = Result.finalEffects[RobotIndex];
            Effects.ExtraAmmoRounds = std::max(0, Effects.ExtraAmmoRounds - 1);
            Effects.ShieldRounds = std::max(0, Effects.ShieldRounds - 1);
            Effects.AcceleratorRounds = std::max(0, Effects.AcceleratorRounds - 1);
        }
        if (robots_[0].hp > robots_[1].hp)
            Result.outcome = MatchOutcome::Robot0Wins;
        else if (robots_[1].hp > robots_[0].hp)
            Result.outcome = MatchOutcome::Robot1Wins;
        return Result;
    }

} // namespace Automata
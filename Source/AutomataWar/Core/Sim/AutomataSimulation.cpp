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

        initialGrid_ = grid_;
        obstacleHealth_.assign(grid_.size(), 0);
        for (size_t CellIndex = 0; CellIndex < grid_.size(); ++CellIndex)
            if (grid_[CellIndex] == CellType::Cover)
                obstacleHealth_[CellIndex] = ObstacleMaxHealth;
    }

    void Simulation::SpawnRobots(int32_t Width, int32_t Height)
    {
        robots_[0] = {};
        robots_[0].x = 1;
        robots_[0].y = 1;
        robots_[0].facing = Dir::South;

        robots_[1] = {};
        robots_[1].x = Width - 2;
        robots_[1].y = Height - 2;
        robots_[1].facing = Dir::North;
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
        const int32_t Direction = static_cast<int32_t>(Robot.facing);
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
                obstacleHealth_[CellIndex] = std::max(0, obstacleHealth_[CellIndex] - ProjectileDamage);
                if (obstacleHealth_[CellIndex] == 0)
                    grid_[CellIndex] = CellType::Empty;
                events_.push_back({Step, RobotIndex, EventType::ShotBlocked, X, Y});
                return;
            }

            if (robots_[TargetIndex].x == X && robots_[TargetIndex].y == Y)
            {
                robots_[TargetIndex].hp = std::max(0, robots_[TargetIndex].hp - ProjectileDamage);
                events_.push_back({Step, TargetIndex, EventType::Hit, ProjectileDamage, RobotIndex});
                return;
            }
        }
    }

    void Simulation::ExecuteCommand(int32_t RobotIndex, EAWCommand Command, int32_t Step)
    {
        RobotState &Robot = robots_[RobotIndex];
        switch (Command)
        {
        case EAWCommand::Move:
        {
            const int32_t Direction = static_cast<int32_t>(Robot.facing);
            const int32_t X = Robot.x + DirDX[Direction];
            const int32_t Y = Robot.y + DirDY[Direction];
            const CellType Cell = CellAt(X, Y);
            if (Cell == CellType::Wall)
                events_.push_back({Step, RobotIndex, EventType::MoveBlockedWall, X, Y});
            else if (Cell == CellType::Cover)
                events_.push_back({Step, RobotIndex, EventType::MoveBlockedCover, X, Y});
            else if (robots_[1 - RobotIndex].x == X && robots_[1 - RobotIndex].y == Y)
                events_.push_back({Step, RobotIndex, EventType::MoveBlockedRobot, X, Y});
            else
            {
                Robot.x = X;
                Robot.y = Y;
                events_.push_back({Step, RobotIndex, EventType::Move, X, Y});
            }
            break;
        }
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
        }
        for (const RobotState &Robot : robots_)
        {
            Mix(Robot.x);
            Mix(Robot.y);
            Mix(static_cast<int64_t>(Robot.facing));
            Mix(Robot.hp);
            Mix(Robot.currentCommand);
            Mix(Robot.nextCommand);
        }
        return Hash;
    }

    MatchResult Simulation::RunMatch(TConstArrayView<EAWCommand> CommandsA, TConstArrayView<EAWCommand> CommandsB, const SimConfig &Config)
    {
        Xorshift64 Rng{Config.seed == 0 ? 1 : Config.seed};
        InitGrid(Config.gridWidth, Config.gridHeight, Rng);
        SpawnRobots(Config.gridWidth, Config.gridHeight);
        events_.clear();
        snapshots_.clear();

        const TConstArrayView<EAWCommand> Commands[2] = {CommandsA, CommandsB};
        const int32_t StepCount = FMath::Max(CommandsA.Num(), CommandsB.Num());
        events_.reserve(static_cast<size_t>(StepCount * 4));
        snapshots_.reserve(static_cast<size_t>(StepCount));

        for (int32_t Step = 0; Step < StepCount; ++Step)
        {
            robots_[0].currentCommand = -1;
            robots_[1].currentCommand = -1;
            const int32_t FirstRobot = Step % 2;

            for (int32_t Pass = 0; Pass < 2; ++Pass)
            {
                const int32_t RobotIndex = Pass == 0 ? FirstRobot : 1 - FirstRobot;
                RobotState &Robot = robots_[RobotIndex];
                if (Robot.hp <= 0 || Robot.nextCommand >= Commands[RobotIndex].Num())
                    continue;

                Robot.currentCommand = Robot.nextCommand;
                ExecuteCommand(RobotIndex, Commands[RobotIndex][Robot.nextCommand], Step);
                ++Robot.nextCommand;
            }

            StepSnapshot Snapshot;
            Snapshot.step = Step;
            Snapshot.robots = robots_;
            Snapshot.obstacleHealth = obstacleHealth_;
            Snapshot.stateHash = ComputeHash();
            snapshots_.push_back(MoveTemp(Snapshot));

            if (robots_[0].hp <= 0 || robots_[1].hp <= 0)
                break;
        }

        finalHash_ = ComputeHash();
        MatchResult Result;
        Result.stepsExecuted = static_cast<int32_t>(snapshots_.size());
        Result.finalHP = {robots_[0].hp, robots_[1].hp};
        if (robots_[0].hp > robots_[1].hp)
            Result.outcome = MatchOutcome::Robot0Wins;
        else if (robots_[1].hp > robots_[0].hp)
            Result.outcome = MatchOutcome::Robot1Wins;
        return Result;
    }

} // namespace Automata
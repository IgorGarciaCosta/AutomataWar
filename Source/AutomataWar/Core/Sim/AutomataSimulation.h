#pragma once

/**
 * @file AutomataSimulation.h
 * @brief Deterministic execution of finite player-selected command lists.
 */

#include "AutomataWar/Core/AutomataRules.h"
#include "AutomataWar/Core/AutomataDomainTypes.h"
#include <array>
#include <cstdint>
#include <vector>

namespace Automata
{

    /** Canonical grid occupancy used by movement and firing. */
    enum class CellType : uint8_t
    {
        Empty,
        Wall,
        Cover,
        ActionPointItem,
        ExtraAmmoItem,
        ShieldItem,
        AcceleratorItem
    };

    /** Complete canonical state for one tank. */
    struct RobotState
    {
        int32_t x = 0;
        int32_t y = 0;
        Dir facing = Dir::North;
        int32_t hp = MaxHP;
        int32_t actionPoints = InitialActionPoints;
        int32_t currentCommand = -1;
        int32_t nextCommand = 0;
        FAWRobotEffects effects;
    };

    /** Arena occupancy and tank placement carried from one round into the next. */
    struct RoundState
    {
        int32_t gridWidth = 0;
        int32_t gridHeight = 0;
        std::vector<CellType> grid;
        std::vector<int32_t> obstacleHealth;
        std::vector<int32_t> actionPointItemValues;
        std::array<int32_t, 2> robotX = {};
        std::array<int32_t, 2> robotY = {};
        std::array<Dir, 2> robotFacing = {Dir::South, Dir::North};
    };

    /** Encode round carryover for replication and replay storage. */
    std::vector<uint8_t> EncodeRoundState(const RoundState &State);
    /** Decode and validate round carryover received from storage or the network. */
    bool DecodeRoundState(const uint8_t *Data, size_t Size, RoundState &OutState);

    /** Replay-visible effects emitted while commands execute. */
    enum class EventType : uint8_t
    {
        Move,
        MoveBlockedWall,
        MoveBlockedCover,
        MoveBlockedRobot,
        Turn,
        Wait,
        ShieldCharged,
        AccelerationCharged,
        Fire,
        Hit,
        ShieldAbsorbed,
        ShotBlocked,
        ActionPointsCollected,
        ExtraAmmoCollected,
        ShieldCollected,
        AcceleratorCollected
    };

    /** Compact event record used by replay, VFX, and audio. */
    struct SimEvent
    {
        int32_t step = 0;
        int32_t robot = 0;
        EventType type = EventType::Move;
        int32_t paramA = 0;
        int32_t paramB = 0;
    };

    /** Presentation snapshot captured after exactly one tank command. */
    struct StepSnapshot
    {
        int32_t step = 0;
        std::array<RobotState, 2> robots;
        std::vector<int32_t> obstacleHealth;
        uint64_t stateHash = 0;
    };

    /** Final winner classification after both command lists finish. */
    enum class MatchOutcome : uint8_t
    {
        Robot0Wins,
        Robot1Wins,
        Draw
    };

    /** Final state required by gameplay UI and deterministic checks. */
    struct MatchResult
    {
        MatchOutcome outcome = MatchOutcome::Draw;
        /** True when HP reaches zero or end-of-round AP cannot buy a positive-cost command. */
        bool bMatchEnded = false;
        /** Rule that produced the terminal result. */
        EAWMatchEndReason endReason = EAWMatchEndReason::None;
        int32_t stepsExecuted = 0;
        std::array<int32_t, 2> finalHP = {};
        std::array<int32_t, 2> finalActionPoints = {};
        std::array<FAWRobotEffects, 2> finalEffects;
    };

    /** Explicit deterministic inputs controlling arena generation. */
    struct SimConfig
    {
        int32_t gridWidth = DefaultGridWidth;
        int32_t gridHeight = DefaultGridHeight;
        uint64_t seed = 12345;
        /** Tank slot whose complete command queue executes first. */
        int32_t startingRobot = 0;
        std::array<int32_t, 2> initialActionPoints = {InitialActionPoints, InitialActionPoints};
        std::array<FAWRobotEffects, 2> initialEffects;
        RoundState initialState;
    };

    /** Runs both queues until they finish or either tank reaches zero HP. */
    class Simulation
    {
    public:
        MatchResult RunMatch(TConstArrayView<EAWCommand> CommandsA, TConstArrayView<EAWCommand> CommandsB, const SimConfig &Config = {});

        const std::vector<SimEvent> &GetEvents() const { return events_; }
        const std::vector<StepSnapshot> &GetSnapshots() const { return snapshots_; }
        uint64_t GetFinalHash() const { return finalHash_; }
        const std::vector<CellType> &GetGrid() const { return initialGrid_; }
        const RoundState &GetFinalState() const { return finalState_; }

    private:
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

        void InitGrid(int32_t Width, int32_t Height, Xorshift64 &Rng);
        void SpawnRobots(int32_t Width, int32_t Height, const SimConfig &Config);
        bool RestoreState(const RoundState &State, const SimConfig &Config);
        void CaptureFinalState();
        void ExecuteCommand(int32_t RobotIndex, EAWCommand Command, int32_t Step);
        /** Move one or two cells, stopping at the first blocker and collecting each crossed item. */
        void Move(int32_t RobotIndex, int32_t Step);
        /** Apply the canonical pickup in a reached cell and emit its replay event. */
        void CollectItem(int32_t RobotIndex, size_t CellIndex, int32_t Step);
        void Fire(int32_t RobotIndex, int32_t Step);
        bool InBounds(int32_t X, int32_t Y) const;
        CellType CellAt(int32_t X, int32_t Y) const;
        uint64_t ComputeHash() const;

        int32_t gridWidth_ = 0;
        int32_t gridHeight_ = 0;
        std::vector<CellType> grid_;
        std::vector<CellType> initialGrid_;
        std::vector<int32_t> obstacleHealth_;
        std::vector<int32_t> actionPointItemValues_;
        std::array<RobotState, 2> robots_;
        std::vector<SimEvent> events_;
        std::vector<StepSnapshot> snapshots_;
        RoundState finalState_;
        uint64_t finalHash_ = 0;
    };

} // namespace Automata
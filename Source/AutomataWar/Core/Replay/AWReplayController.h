#pragma once

/**
 * @file AWReplayController.h
 * @brief Re-simulates command lists and provides step-based replay navigation.
 */

#include "AutomataWar/Core/Sim/AutomataSimulation.h"
#include <vector>

namespace Automata
{

    class FAWReplayController
    {
    public:
        /** Re-simulate a replay from its complete canonical starting state. */
        bool Initialize(const TArray<EAWCommand> &CommandsA, const TArray<EAWCommand> &CommandsB, uint64_t Seed,
                        int32_t InitialActionPointsA = InitialActionPoints,
                        int32_t InitialActionPointsB = InitialActionPoints,
                        const FAWRobotEffects &InitialEffectsA = {},
                        const FAWRobotEffects &InitialEffectsB = {});
        bool IsValid() const { return bValid_; }

        int32_t GetTotalSteps() const { return static_cast<int32_t>(snapshots_.size()); }
        int32_t GetCurrentStep() const { return currentStep_; }
        void SeekToStep(int32_t Step);
        bool StepForward();
        bool StepBackward();

        const StepSnapshot &GetCurrentSnapshot() const { return snapshots_[currentStep_]; }
        std::vector<SimEvent> GetEventsForStep(int32_t Step) const;
        std::vector<SimEvent> GetEventsInRange(int32_t FromStep, int32_t ToStep) const;
        const MatchResult &GetResult() const { return result_; }
        const TArray<EAWCommand> &GetCommandsA() const { return commandsA_; }
        const TArray<EAWCommand> &GetCommandsB() const { return commandsB_; }
        const std::vector<CellType> &GetGrid() const { return grid_; }
        const SimConfig &GetConfig() const { return config_; }

    private:
        bool bValid_ = false;
        int32_t currentStep_ = 0;
        TArray<EAWCommand> commandsA_;
        TArray<EAWCommand> commandsB_;
        SimConfig config_;
        MatchResult result_;
        std::vector<StepSnapshot> snapshots_;
        std::vector<SimEvent> events_;
        std::vector<CellType> grid_;
    };

} // namespace Automata
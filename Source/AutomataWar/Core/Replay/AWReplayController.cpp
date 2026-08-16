#include "AWReplayController.h"
#include <algorithm>

namespace Automata
{

    bool FAWReplayController::Initialize(const TArray<EAWCommand> &CommandsA, const TArray<EAWCommand> &CommandsB, uint64_t Seed,
                                         int32_t InitialActionPointsA, int32_t InitialActionPointsB,
                                         const FAWRobotEffects &InitialEffectsA, const FAWRobotEffects &InitialEffectsB,
                                         int32_t StartingRobot, TConstArrayView<uint8> InitialState)
    {
        bValid_ = false;
        commandsA_ = CommandsA;
        commandsB_ = CommandsB;
        config_.seed = Seed;
        config_.startingRobot = StartingRobot == 1 ? 1 : 0;
        config_.initialActionPoints = {InitialActionPointsA, InitialActionPointsB};
        config_.initialEffects = {InitialEffectsA, InitialEffectsB};
        if (!DecodeRoundState(InitialState.GetData(), static_cast<size_t>(InitialState.Num()), config_.initialState))
            return false;
        if (!config_.initialState.grid.empty())
        {
            config_.gridWidth = config_.initialState.gridWidth;
            config_.gridHeight = config_.initialState.gridHeight;
        }

        Simulation Sim;
        result_ = Sim.RunMatch(commandsA_, commandsB_, config_);
        snapshots_ = Sim.GetSnapshots();
        events_ = Sim.GetEvents();
        grid_ = Sim.GetGrid();

        currentStep_ = 0;
        bValid_ = !snapshots_.empty();
        return bValid_;
    }

    void FAWReplayController::SeekToStep(int32_t Step)
    {
        if (bValid_)
            currentStep_ = std::clamp(Step, 0, GetTotalSteps() - 1);
    }

    bool FAWReplayController::StepForward()
    {
        if (!bValid_ || currentStep_ >= GetTotalSteps() - 1)
            return false;
        ++currentStep_;
        return true;
    }

    bool FAWReplayController::StepBackward()
    {
        if (!bValid_ || currentStep_ <= 0)
            return false;
        --currentStep_;
        return true;
    }

    std::vector<SimEvent> FAWReplayController::GetEventsForStep(int32_t Step) const
    {
        std::vector<SimEvent> Result;
        for (const SimEvent &Event : events_)
            if (Event.step == Step)
                Result.push_back(Event);
        return Result;
    }

    std::vector<SimEvent> FAWReplayController::GetEventsInRange(int32_t FromStep, int32_t ToStep) const
    {
        std::vector<SimEvent> Result;
        for (const SimEvent &Event : events_)
            if (Event.step >= FromStep && Event.step <= ToStep)
                Result.push_back(Event);
        return Result;
    }

} // namespace Automata
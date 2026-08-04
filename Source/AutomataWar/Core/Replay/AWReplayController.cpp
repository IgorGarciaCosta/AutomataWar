#include "AWReplayController.h"
#include <algorithm>

namespace Automata
{

    bool FAWReplayController::Initialize(const std::string &SourceA, const std::string &SourceB, uint64_t Seed)
    {
        bValid_ = false;
        sourceA_ = SourceA;
        sourceB_ = SourceB;

        CompileResult cA = Compile(SourceA);
        if (!cA.Ok())
            return false;
        CompileResult cB = Compile(SourceB);
        if (!cB.Ok())
            return false;

        programA_ = cA.program;
        programB_ = cB.program;

        config_.seed = Seed;

        Simulation Sim;
        result_ = Sim.RunMatch(programA_, programB_, config_);
        snapshots_ = Sim.GetSnapshots();
        events_ = Sim.GetEvents();
        grid_ = Sim.GetGrid();

        currentTick_ = 0;
        bValid_ = !snapshots_.empty();
        return bValid_;
    }

    void FAWReplayController::SeekToTick(int32_t Tick)
    {
        if (!bValid_)
            return;
        currentTick_ = std::clamp(Tick, 0, GetTotalTicks() - 1);
    }

    bool FAWReplayController::StepForward()
    {
        if (!bValid_ || currentTick_ >= GetTotalTicks() - 1)
            return false;
        ++currentTick_;
        return true;
    }

    bool FAWReplayController::StepBackward()
    {
        if (!bValid_ || currentTick_ <= 0)
            return false;
        --currentTick_;
        return true;
    }

    bool FAWReplayController::StepInstruction(int32_t RobotIdx)
    {
        if (!bValid_ || RobotIdx < 0 || RobotIdx > 1)
            return false;
        if (currentTick_ >= GetTotalTicks() - 1)
            return false;

        int32_t startCount = snapshots_[currentTick_].robots[RobotIdx].vm.instrExecCount;

        while (currentTick_ < GetTotalTicks() - 1)
        {
            ++currentTick_;
            if (snapshots_[currentTick_].robots[RobotIdx].vm.instrExecCount > startCount)
                return true;
        }
        return true;
    }

    std::vector<SimEvent> FAWReplayController::GetEventsForTick(int32_t Tick) const
    {
        std::vector<SimEvent> result;
        for (const auto &e : events_)
        {
            if (e.tick == Tick)
                result.push_back(e);
        }
        return result;
    }

    std::vector<SimEvent> FAWReplayController::GetEventsInRange(int32_t FromTick, int32_t ToTick) const
    {
        std::vector<SimEvent> result;
        for (const auto &e : events_)
        {
            if (e.tick >= FromTick && e.tick <= ToTick)
                result.push_back(e);
        }
        return result;
    }

} // namespace Automata

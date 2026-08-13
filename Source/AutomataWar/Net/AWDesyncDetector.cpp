#include "AWDesyncDetector.h"
#include "AutomataWar/Core/Sim/AutomataSimulation.h"
#include "AutomataWar/Game/AWMatchTypes.h"

bool FAWDesyncDetector::VerifyMatch(const TArray<EAWCommand> &Commands0, const TArray<EAWCommand> &Commands1, uint64 Seed, uint64 AuthoritativeHash)
{
    Automata::SimConfig Config;
    Config.seed = Seed;

    Automata::Simulation Sim;
    Sim.RunMatch(Commands0, Commands1, Config);
    uint64 LocalHash = Sim.GetFinalHash();

    if (LocalHash != AuthoritativeHash)
    {
        UE_LOG(LogAutomataNet, Error, TEXT("DESYNC DETECTED: local hash 0x%016llX != authority 0x%016llX (seed=%llu)"),
               LocalHash, AuthoritativeHash, Seed);
        return false;
    }

    UE_LOG(LogAutomataNet, Verbose, TEXT("Desync check passed: hash 0x%016llX"), LocalHash);
    return true;
}

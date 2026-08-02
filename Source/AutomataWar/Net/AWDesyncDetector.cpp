#include "AWDesyncDetector.h"
#include "AutomataWar/Core/Lang/AutomataCompiler.h"
#include "AutomataWar/Core/Sim/AutomataSimulation.h"
#include "AutomataWar/Game/AWMatchTypes.h"

bool FAWDesyncDetector::VerifyMatch(const FString &Source0, const FString &Source1, uint64 Seed, uint64 AuthoritativeHash)
{
    std::string Src0 = TCHAR_TO_UTF8(*Source0);
    std::string Src1 = TCHAR_TO_UTF8(*Source1);

    Automata::CompileResult C0 = Automata::Compile(Src0);
    Automata::CompileResult C1 = Automata::Compile(Src1);

    if (!C0.Ok() || !C1.Ok())
    {
        UE_LOG(LogAutomataNet, Error, TEXT("Desync check: compilation failed on client (should never happen for revealed scripts)."));
        return false;
    }

    Automata::SimConfig Config;
    Config.seed = Seed;

    Automata::Simulation Sim;
    Sim.RunMatch(C0.program, C1.program, Config);
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

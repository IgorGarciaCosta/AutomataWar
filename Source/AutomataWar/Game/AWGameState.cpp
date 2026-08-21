#include "AWGameState.h"
#include "AutomataWar/Core/Replay/AutomataReplay.h"
#include "AutomataWar/Net/AWDesyncDetector.h"
#include "Net/UnrealNetwork.h"

AAWGameState::AAWGameState()
{
    bReplicates = true;
}

void AAWGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAWGameState, Phase);
    DOREPLIFETIME(AAWGameState, SubmissionTimeRemaining);
    DOREPLIFETIME(AAWGameState, ResolvedRound);
    DOREPLIFETIME(AAWGameState, ActionPoints0);
    DOREPLIFETIME(AAWGameState, ActionPoints1);
    DOREPLIFETIME(AAWGameState, Effects0);
    DOREPLIFETIME(AAWGameState, Effects1);
}

void AAWGameState::SetActionPoints(int32 Slot, int32 Value)
{
    if (Slot == 0)
        ActionPoints0 = FMath::Max(0, Value);
    else if (Slot == 1)
        ActionPoints1 = FMath::Max(0, Value);
}

void AAWGameState::SetEffects(int32 Slot, const FAWRobotEffects &Effects)
{
    if (Slot == 0)
        Effects0 = Effects;
    else if (Slot == 1)
        Effects1 = Effects;
}

void AAWGameState::SetResolvedRound(const FAWResolvedRound &NewRound)
{
    ResolvedRound = NewRound;
    OnResolvedRoundChanged.Broadcast();
    ForceNetUpdate();
}

void AAWGameState::OnRep_Phase()
{
    OnPhaseChanged.Broadcast(Phase);
}

void AAWGameState::OnRep_ResolvedRound()
{
    if (ResolvedRound.IsReadyForReplay() && ResolvedRound.AuthoritativeHash != 0)
    {
        const Automata::ReplayData Data = MakeReplayData(ResolvedRound);
        Automata::SimConfig Config;
        Config.seed = Data.seed;
        Config.startingRobot = Data.startingRobot;
        Config.initialActionPoints = {Data.initialActionPointsA, Data.initialActionPointsB};
        Config.initialEffects = {Data.initialEffectsA, Data.initialEffectsB};
        if (Automata::DecodeRoundState(Data.initialState.data(), Data.initialState.size(), Config.initialState) &&
            !Config.initialState.grid.empty())
        {
            Config.gridWidth = Config.initialState.gridWidth;
            Config.gridHeight = Config.initialState.gridHeight;
            FAWDesyncDetector::VerifyMatch(
                Data.commandsA, Data.commandsB, Config,
                static_cast<uint64>(ResolvedRound.AuthoritativeHash));
        }
    }
    OnResolvedRoundChanged.Broadcast();
}

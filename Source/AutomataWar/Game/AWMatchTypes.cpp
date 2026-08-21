#include "AWMatchTypes.h"
#include "AutomataWar/Core/Replay/AutomataReplay.h"

Automata::ReplayData MakeReplayData(const FAWResolvedRound &Round)
{
    Automata::ReplayData Data;
    Data.seed = static_cast<uint64>(Round.Seed);
    Data.startingRobot = Round.StartingSlot;
    Data.initialActionPointsA = Round.InitialActionPoints0;
    Data.initialActionPointsB = Round.InitialActionPoints1;
    Data.initialEffectsA = Round.InitialEffects0;
    Data.initialEffectsB = Round.InitialEffects1;
    if (!Round.InitialArenaState.IsEmpty())
        Data.initialState.assign(Round.InitialArenaState.GetData(),
                                 Round.InitialArenaState.GetData() + Round.InitialArenaState.Num());
    Data.commandsA = Round.Commands0;
    Data.commandsB = Round.Commands1;
    return Data;
}

FAWResolvedRound MakeResolvedRound(Automata::ReplayData Data)
{
    FAWResolvedRound Round;
    Round.StartingSlot = Data.startingRobot;
    Round.Seed = static_cast<int64>(Data.seed);
    Round.InitialActionPoints0 = Data.initialActionPointsA;
    Round.InitialActionPoints1 = Data.initialActionPointsB;
    Round.InitialEffects0 = Data.initialEffectsA;
    Round.InitialEffects1 = Data.initialEffectsB;
    if (!Data.initialState.empty())
        Round.InitialArenaState.Append(Data.initialState.data(), static_cast<int32>(Data.initialState.size()));
    Round.Commands0 = MoveTemp(Data.commandsA);
    Round.Commands1 = MoveTemp(Data.commandsB);
    Round.bResolved = true;
    return Round;
}
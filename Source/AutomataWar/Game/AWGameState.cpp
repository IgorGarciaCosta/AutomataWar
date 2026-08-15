#include "AWGameState.h"
#include "Net/UnrealNetwork.h"

AAWGameState::AAWGameState()
{
    bReplicates = true;
}

void AAWGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAWGameState, Phase);
    DOREPLIFETIME(AAWGameState, RoundNumber);
    DOREPLIFETIME(AAWGameState, RoundStartingSlot);
    DOREPLIFETIME(AAWGameState, SubmissionTimeRemaining);
    DOREPLIFETIME(AAWGameState, RevealedCommands0);
    DOREPLIFETIME(AAWGameState, RevealedCommands1);
    DOREPLIFETIME(AAWGameState, AuthoritativeHash);
    DOREPLIFETIME(AAWGameState, SimSeed);
    DOREPLIFETIME(AAWGameState, Outcome);
    DOREPLIFETIME(AAWGameState, ActionPoints0);
    DOREPLIFETIME(AAWGameState, ActionPoints1);
    DOREPLIFETIME(AAWGameState, ReplayStartActionPoints0);
    DOREPLIFETIME(AAWGameState, ReplayStartActionPoints1);
    DOREPLIFETIME(AAWGameState, Effects0);
    DOREPLIFETIME(AAWGameState, Effects1);
    DOREPLIFETIME(AAWGameState, ReplayStartEffects0);
    DOREPLIFETIME(AAWGameState, ReplayStartEffects1);
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

void AAWGameState::OnRep_Phase()
{
    OnPhaseChanged.Broadcast(Phase);
}

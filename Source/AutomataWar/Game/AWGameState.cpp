#include "AWGameState.h"
#include "Net/UnrealNetwork.h"

AAWGameState::AAWGameState()
{
	bReplicates = true;
}

void AAWGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAWGameState, Phase);
	DOREPLIFETIME(AAWGameState, RoundNumber);
	DOREPLIFETIME(AAWGameState, SubmissionTimeRemaining);
	DOREPLIFETIME(AAWGameState, RevealedSource0);
	DOREPLIFETIME(AAWGameState, RevealedSource1);
	DOREPLIFETIME(AAWGameState, AuthoritativeHash);
	DOREPLIFETIME(AAWGameState, SimSeed);
	DOREPLIFETIME(AAWGameState, Outcome);
}

void AAWGameState::OnRep_Phase()
{
	OnPhaseChanged.Broadcast(Phase);
}

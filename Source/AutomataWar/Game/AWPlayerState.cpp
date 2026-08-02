#include "AWPlayerState.h"
#include "Net/UnrealNetwork.h"

AAWPlayerState::AAWPlayerState()
{
}

void AAWPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAWPlayerState, ScriptSlot);
	DOREPLIFETIME(AAWPlayerState, bSubmitted);
}

#include "AWPlayerState.h"
#include "Net/UnrealNetwork.h"

void AAWPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAWPlayerState, CommandSlot);
    DOREPLIFETIME(AAWPlayerState, bSubmitted);
}

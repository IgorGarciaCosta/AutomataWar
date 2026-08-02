/**
 * @file AWSpectatorPawn.cpp
 * @brief Implementation of the minimal spectator pawn.
 */

#include "AWSpectatorPawn.h"
#include "Components/SphereComponent.h"

AAWSpectatorPawn::AAWSpectatorPawn()
{
	// Disable collision entirely
	if (UPrimitiveComponent* Collision = Cast<UPrimitiveComponent>(GetRootComponent()))
	{
		Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	bAddDefaultMovementBindings = false;
}

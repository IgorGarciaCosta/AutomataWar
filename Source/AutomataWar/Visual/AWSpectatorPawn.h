#pragma once

/**
 * @file AWSpectatorPawn.h
 * @brief Minimal spectator pawn used as default pawn class.
 *
 * No movement input; just a body for the player controller to possess.
 * Camera is handled separately by the isometric camera actor.
 */

#include "CoreMinimal.h"
#include "GameFramework/SpectatorPawn.h"
#include "AWSpectatorPawn.generated.h"

/**
 * @brief Default pawn: spectator with no movement, collision disabled.
 */
UCLASS()
class AUTOMATAWAR_API AAWSpectatorPawn : public ASpectatorPawn
{
    GENERATED_BODY()

public:
    AAWSpectatorPawn();
};

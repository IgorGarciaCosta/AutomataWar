#pragma once

/**
 * @file AWPlayerState.h
 * @brief Per-player replicated command-slot assignment.
 */

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AWMatchTypes.h"
#include "AWPlayerState.generated.h"

/**
 * @brief Tracks which command slot is owned by a network player.
 *
 * Command queues are revealed through GameState only after simulation.
 */
UCLASS()
class AUTOMATAWAR_API AAWPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    /** Register the authoritative command-slot assignment for replication. */
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    /** Which command slot this player controls (0 or 1). */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
    int32 CommandSlot = -1;
};

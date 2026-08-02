#pragma once

/**
 * @file AWPlayerState.h
 * @brief Per-player replicated state: submission status and revealed info.
 */

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AWMatchTypes.h"
#include "AWPlayerState.generated.h"

/**
 * @brief Tracks per-player submission readiness.
 *
 * The actual script source is NOT replicated here to preserve blind submission.
 */
UCLASS()
class AUTOMATAWAR_API AAWPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    AAWPlayerState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    /** Which script slot this player controls (0 or 1). */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
    int32 ScriptSlot = -1;

    /** Whether this player has submitted/locked their script this round. */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
    bool bSubmitted = false;

    /** Last validation error message (client-only, not replicated). */
    UPROPERTY(BlueprintReadOnly, Category = "Match")
    FString LastError;
};

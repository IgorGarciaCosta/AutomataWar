#pragma once

/**
 * @file AWAIController.h
 * @brief Deterministic command-queue planning for single-player opponents.
 */

#include "CoreMinimal.h"
#include "AIController.h"
#include "AutomataWar/Game/AWMatchTypes.h"
#include "AWAIController.generated.h"

/** Planning depth used by the single-player opponent. */
UENUM(BlueprintType)
enum class EAWAIDifficulty : uint8
{
    Easy UMETA(DisplayName = "Easy"),
    Normal UMETA(DisplayName = "Normal"),
    Hard UMETA(DisplayName = "Hard")
};

/**
 * Server-owned planner for the single-player command slot.
 *
 * The controller never mutates simulation state or bypasses match validation.
 * It builds a finite queue, then AAWGameMode submits that queue through the same
 * authoritative path used by local and network players. It does not possess a
 * pawn because the tanks are snapshot-driven presentation actors.
 */
UCLASS()
class AUTOMATAWAR_API AAWAIController : public AAIController
{
    GENERATED_BODY()

public:
    /** Disable ticking and player-state creation for this planning-only controller. */
    AAWAIController();

    /**
     * Build a deterministic, non-empty command queue within the supplied AP budget.
     * @param Difficulty Planning depth and access to advanced commands.
     * @param AvailableActionPoints Maximum total queue cost.
     * @param Seed Stable value used to vary left/right routing.
     */
    static TArray<EAWCommand> GenerateCommandQueue(EAWAIDifficulty Difficulty, int32 AvailableActionPoints, int32 Seed);
};
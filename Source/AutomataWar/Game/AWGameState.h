#pragma once

/**
 * @file AWGameState.h
 * @brief Replicated match state including phase, round, timer, and post-sim results.
 */

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "AWMatchTypes.h"
#include "AWGameState.generated.h"

/**
 * @brief Authoritative replicated match state.
 *
 * Manages the phase state machine and replicates revealed commands/outcomes
 * only after simulation completes.
 */
UCLASS()
class AUTOMATAWAR_API AAWGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    /** Initialize the replicated match state defaults. */
    AAWGameState();

    /** Register all authoritative match fields for Unreal replication. */
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    /** Current match phase. */
    UPROPERTY(ReplicatedUsing = OnRep_Phase, BlueprintReadOnly, Category = "Match")
    EAWMatchPhase Phase = EAWMatchPhase::Programming;

    /** Current round number (1-based). */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
    int32 RoundNumber = 1;

    /** Submission time remaining (seconds). -1 = no timer (standalone). */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
    float SubmissionTimeRemaining = -1.f;

    /** Revealed commands for slot 0 (only valid in ReplayAutopsy phase). */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
    TArray<EAWCommand> RevealedCommands0;

    /** Revealed commands for slot 1 (only valid in ReplayAutopsy phase). */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
    TArray<EAWCommand> RevealedCommands1;

    /** Authoritative final state hash for desync detection. */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
    int64 AuthoritativeHash = 0;

    /** Simulation seed used for this round. */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
    int64 SimSeed = 0;

    /** Match outcome (valid in ReplayAutopsy). */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
    FAWMatchOutcome Outcome;

    /** Delegate broadcast on phase change. */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhaseChanged, EAWMatchPhase, NewPhase);
    UPROPERTY(BlueprintAssignable, Category = "Match")
    FOnPhaseChanged OnPhaseChanged;

protected:
    UFUNCTION()
    void OnRep_Phase();
};

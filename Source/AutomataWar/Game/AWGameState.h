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

    /** Tank slot whose complete command queue executes first this round. */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
    int32 RoundStartingSlot = INDEX_NONE;

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

    /** AP currently available to each command slot. */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match|Action Points")
    int32 ActionPoints0 = Automata::InitialActionPoints;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match|Action Points")
    int32 ActionPoints1 = Automata::InitialActionPoints;

    /** AP at simulation start, retained so replay reconstruction is exact. */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match|Action Points")
    int32 ReplayStartActionPoints0 = Automata::InitialActionPoints;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match|Action Points")
    int32 ReplayStartActionPoints1 = Automata::InitialActionPoints;

    /** Persistent effects currently available to each command slot. */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match|Effects")
    FAWRobotEffects Effects0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match|Effects")
    FAWRobotEffects Effects1;

    /** Effects at simulation start, retained so replay reconstruction is exact. */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match|Effects")
    FAWRobotEffects ReplayStartEffects0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match|Effects")
    FAWRobotEffects ReplayStartEffects1;

    /** Return the non-negative AP balance for a command slot. */
    int32 GetActionPoints(int32 Slot) const { return Slot == 0 ? ActionPoints0 : ActionPoints1; }
    /** Replace a command slot's AP balance, clamped to zero. */
    void SetActionPoints(int32 Slot, int32 Value);
    /** Return the persistent effects owned by a command slot. */
    const FAWRobotEffects &GetEffects(int32 Slot) const { return Slot == 0 ? Effects0 : Effects1; }
    /** Replace the persistent effects owned by a valid command slot. */
    void SetEffects(int32 Slot, const FAWRobotEffects &Effects);

    /** Delegate broadcast on phase change. */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhaseChanged, EAWMatchPhase, NewPhase);
    UPROPERTY(BlueprintAssignable, Category = "Match")
    FOnPhaseChanged OnPhaseChanged;

protected:
    UFUNCTION()
    void OnRep_Phase();
};

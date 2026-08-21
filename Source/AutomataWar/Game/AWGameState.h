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
 * Manages the phase state machine and publishes each resolved round as one
 * replicated record after simulation completes.
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

    /** Submission time remaining (seconds). -1 = no timer (standalone). */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
    float SubmissionTimeRemaining = -1.f;

    /** Complete round record; RepNotify fires only when replay inputs change. */
    UPROPERTY(ReplicatedUsing = OnRep_ResolvedRound, BlueprintReadOnly, Category = "Match")
    FAWResolvedRound ResolvedRound;

    /** AP currently available to each command slot. */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match|Action Points")
    int32 ActionPoints0 = Automata::InitialActionPoints;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match|Action Points")
    int32 ActionPoints1 = Automata::InitialActionPoints;

    /** Persistent effects currently available to each command slot. */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match|Effects")
    FAWRobotEffects Effects0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match|Effects")
    FAWRobotEffects Effects1;

    /** Return the non-negative AP balance for a command slot. */
    int32 GetActionPoints(int32 Slot) const { return Slot == 0 ? ActionPoints0 : ActionPoints1; }
    /** Replace a command slot's AP balance, clamped to zero. */
    void SetActionPoints(int32 Slot, int32 Value);
    /** Return the persistent effects owned by a command slot. */
    const FAWRobotEffects &GetEffects(int32 Slot) const { return Slot == 0 ? Effects0 : Effects1; }
    /** Replace the persistent effects owned by a valid command slot. */
    void SetEffects(int32 Slot, const FAWRobotEffects &Effects);
    /** Publish one complete round record and notify local server-side observers. */
    void SetResolvedRound(const FAWResolvedRound &NewRound);

    /** Delegate broadcast on phase change. */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhaseChanged, EAWMatchPhase, NewPhase);
    UPROPERTY(BlueprintAssignable, Category = "Match")
    FOnPhaseChanged OnPhaseChanged;

    /** Delegate broadcast when one complete round record becomes available or resets. */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnResolvedRoundChanged);
    UPROPERTY(BlueprintAssignable, Category = "Match")
    FOnResolvedRoundChanged OnResolvedRoundChanged;

protected:
    /** Notify observers after Unreal applies a replicated phase change. */
    UFUNCTION()
    void OnRep_Phase();
    /** Verify and publish a complete replicated round after all nested fields arrive. */
    UFUNCTION()
    void OnRep_ResolvedRound();
};

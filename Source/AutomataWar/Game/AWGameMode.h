#pragma once

/**
 * @file AWGameMode.h
 * @brief Server-authoritative match orchestration: phase transitions, validation,
 *        simulation execution, and result distribution.
 *
 * Serves both standalone local (hot-seat) and listen-server networked play.
 */

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AWMatchTypes.h"
#include "AWGameMode.generated.h"

/**
 * @brief Central authority for Automata War matches.
 *
 * Manages the Programming -> Submission -> Simulation -> ReplayAutopsy cycle.
 * In standalone, two command slots are filled by direct calls.
 * In online, slots map to connected players via PlayerState.
 */
UCLASS(Blueprintable)
class AUTOMATAWAR_API AAWGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    /** Configure the authoritative state, controller, player state, and spectator classes. */
    AAWGameMode();

    /** Determine standalone versus network authority when the arena initializes. */
    virtual void InitGame(const FString &MapName, const FString &Options, FString &ErrorMessage) override;
    /** Assign the next stable command slot to a newly connected player. */
    virtual void PostLogin(APlayerController *NewPlayer) override;
    /** Convert an online disconnect into a deterministic forfeit and clean logout. */
    virtual void Logout(AController *Exiting) override;

    /** Reset the current standalone world for a fresh local hot-seat match. */
    void BeginLocalMatch();

    /**
     * @brief Handle a command-list submission for a given slot.
     * @param Slot 0 or 1.
     * @param Commands Ordered tank actions.
     * @return Validation result.
     *
     * This is the single authoritative entry point used by both local
     * direct calls and Server RPCs.
     */
    FAWValidationResult HandleSubmission(int32 Slot, const TArray<EAWCommand> &Commands);

    /** Withdraw a local command slot while the match is still in Programming. */
    FAWValidationResult WithdrawSubmission(int32 Slot);

    /** Force advance to next round (ReplayAutopsy -> Programming). */
    UFUNCTION(BlueprintCallable, Category = "AutomataWar")
    void AdvanceToNextRound();

    /** Submission timer duration in seconds. -1 disables. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AutomataWar")
    float SubmissionTimerSeconds = 180.f;

    /** Whether this is a local standalone match (no net). */
    UPROPERTY(BlueprintReadOnly, Category = "AutomataWar")
    bool bLocalMatch = true;

protected:
    /** Transition to a new phase with validation. */
    void SetPhase(EAWMatchPhase NewPhase);

    /** Called when both slots have submitted. */
    void OnBothSubmitted();

    /** Run the Core simulation on server. */
    void RunSimulation();

    /** Timer callback for submission deadline. */
    void OnSubmissionTimerExpired();

    /** Last accepted command list per slot. */
    TArray<EAWCommand> AcceptedCommands[2];

    /** Whether each slot has submitted this round. */
    bool bSlotSubmitted[2] = {false, false};

    /** AP already reserved by each slot's last accepted persistent program. */
    int32 CommittedProgramCosts[2] = {0, 0};

    /** Timer handle for submission deadline. */
    FTimerHandle SubmissionTimerHandle;

    /** Next player slot to assign. */
    int32 NextSlot = 0;
};

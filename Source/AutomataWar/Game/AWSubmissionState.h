#pragma once

/**
 * @file AWSubmissionState.h
 * @brief Pure command-submission bookkeeping shared by authoritative match flows.
 */

#include "CoreMinimal.h"
#include "AWMatchTypes.h"

/**
 * Owns the two accepted command queues, reservation costs, and submission flags.
 * AAWGameMode supplies phase and AP authority, then applies the returned AP delta.
 */
struct FAWSubmissionState
{
    /** Initialize a new match with a safe zero-cost timeout fallback. */
    FAWSubmissionState()
    {
        ResetForMatch();
    }

    /** Reset all bookkeeping for a newly configured match. */
    void ResetForMatch()
    {
        ResetForRound();
    }

    /** Reset submission state and install WAIT as each slot's affordable timeout fallback. */
    void ResetForRound()
    {
        for (int32 Slot = 0; Slot < 2; ++Slot)
        {
            AcceptedCommands[Slot] = {EAWCommand::Wait};
            bSubmitted[Slot] = false;
            CommittedProgramCosts[Slot] = 0;
        }
    }

    /**
     * Validate and retain a queue, returning the AP reservation delta to apply.
     * Replacing a queue charges or refunds only the difference from its prior cost.
     */
    FAWValidationResult TrySubmit(int32 Slot, TConstArrayView<EAWCommand> Commands,
                                  int32 AvailableActionPoints, int32 &OutCostDelta)
    {
        OutCostDelta = 0;
        FAWValidationResult Result;
        if (!IsValidSlot(Slot))
        {
            Result.ErrorMessage = TEXT("Invalid command slot.");
            return Result;
        }
        if (Commands.IsEmpty())
        {
            Result.ErrorMessage = TEXT("Add at least one action before submitting.");
            return Result;
        }
        if (Commands.Num() > Automata::MaxCommands)
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("A program can contain at most %d actions."), Automata::MaxCommands);
            return Result;
        }
        for (EAWCommand Command : Commands)
        {
            if (Command >= EAWCommand::Count)
            {
                Result.ErrorMessage = TEXT("Command list contains an invalid action.");
                return Result;
            }
        }

        const int32 NewProgramCost = GetProgramActionPointCost(Commands);
        OutCostDelta = NewProgramCost - CommittedProgramCosts[Slot];
        if (OutCostDelta > AvailableActionPoints)
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("Program needs %d more AP; only %d AP available."),
                OutCostDelta, AvailableActionPoints);
            OutCostDelta = 0;
            return Result;
        }

        AcceptedCommands[Slot] = Commands;
        CommittedProgramCosts[Slot] = NewProgramCost;
        bSubmitted[Slot] = true;
        Result.bSuccess = true;
        return Result;
    }

    /** Reopen a valid slot while retaining its accepted queue and reservation baseline. */
    FAWValidationResult Withdraw(int32 Slot)
    {
        FAWValidationResult Result;
        if (!IsValidSlot(Slot))
        {
            Result.ErrorMessage = TEXT("Invalid command slot.");
            return Result;
        }

        bSubmitted[Slot] = false;
        Result.bSuccess = true;
        return Result;
    }

    /**
     * Lock a non-submitted slot at timeout and return whether its AP delta is affordable.
     * The slot always becomes submitted so the authoritative match can continue.
     */
    bool ExpireSlot(int32 Slot, int32 AvailableActionPoints, int32 &OutCostDelta)
    {
        OutCostDelta = 0;
        if (!IsValidSlot(Slot) || bSubmitted[Slot])
            return false;

        const int32 NewProgramCost = GetProgramActionPointCost(AcceptedCommands[Slot]);
        const int32 CostDelta = NewProgramCost - CommittedProgramCosts[Slot];
        const bool bCanApplyCost = CostDelta <= AvailableActionPoints;
        if (bCanApplyCost)
        {
            OutCostDelta = CostDelta;
            CommittedProgramCosts[Slot] = NewProgramCost;
        }
        bSubmitted[Slot] = true;
        return bCanApplyCost;
    }

    /** Return the accepted queue for a valid command slot. */
    const TArray<EAWCommand> &GetCommands(int32 Slot) const
    {
        check(IsValidSlot(Slot));
        return AcceptedCommands[Slot];
    }

    /** Return whether a valid command slot is locked for this round. */
    bool IsSubmitted(int32 Slot) const
    {
        return IsValidSlot(Slot) && bSubmitted[Slot];
    }

    /** Return whether both command slots are ready for deterministic execution. */
    bool AreBothSubmitted() const
    {
        return bSubmitted[0] && bSubmitted[1];
    }

private:
    /** Return whether a caller supplied one of the two supported command slots. */
    static bool IsValidSlot(int32 Slot)
    {
        return Slot == 0 || Slot == 1;
    }

    TArray<EAWCommand> AcceptedCommands[2];
    bool bSubmitted[2] = {false, false};
    int32 CommittedProgramCosts[2] = {0, 0};
};
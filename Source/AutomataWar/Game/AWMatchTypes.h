#pragma once

/**
 * @file AWMatchTypes.h
 * @brief Shared enumerations and structs for the Automata War match orchestration layer.
 */

#include "CoreMinimal.h"
#include "AutomataWar/Core/AutomataDomainTypes.h"
#include "AWMatchTypes.generated.h"

namespace Automata
{
    struct ReplayData;
}

DECLARE_LOG_CATEGORY_EXTERN(LogAutomataGame, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAutomataNet, Log, All);

/** Match challenge preset controlling starting AP and single-player AI depth. */
UENUM(BlueprintType)
enum class EAWDifficulty : uint8
{
    Easy UMETA(DisplayName = "Easy"),
    Normal UMETA(DisplayName = "Normal"),
    Hard UMETA(DisplayName = "Hard")
};

/** Return the symmetric starting AP budget for a match difficulty preset. */
inline constexpr int32 GetStartingActionPoints(EAWDifficulty Difficulty)
{
    switch (Difficulty)
    {
    case EAWDifficulty::Easy:
        return Automata::InitialActionPoints * 3 / 2;
    case EAWDifficulty::Hard:
        return Automata::InitialActionPoints * 3 / 4;
    default:
        return Automata::InitialActionPoints;
    }
}

/** Selectable procedural arena dimensions used when a new match is created. */
UENUM(BlueprintType)
enum class EAWArenaSize : uint8
{
    Compact UMETA(DisplayName = "Compact 8 x 8"),
    Standard UMETA(DisplayName = "Standard 16 x 16"),
    Expanded UMETA(DisplayName = "Expanded 32 x 32")
};

/** Return the canonical grid dimensions for a selectable arena size. */
inline FIntPoint GetArenaGridSize(EAWArenaSize ArenaSize)
{
    switch (ArenaSize)
    {
    case EAWArenaSize::Compact:
        return {Automata::CompactGridWidth, Automata::CompactGridHeight};
    case EAWArenaSize::Expanded:
        return {Automata::ExpandedGridWidth, Automata::ExpandedGridHeight};
    default:
        return {Automata::DefaultGridWidth, Automata::DefaultGridHeight};
    }
}

/**
 * Choose initiative before programming begins.
 * Round one is random; later rounds favor the higher AP balance and use the
 * supplied random slot only when both balances are equal.
 */
inline constexpr int32 ChooseRoundStartingSlot(int32 RoundNumber, int32 ActionPoints0,
                                               int32 ActionPoints1, int32 RandomSlot)
{
    const int32 TieBreakSlot = RandomSlot == 1 ? 1 : 0;
    if (RoundNumber <= 1 || ActionPoints0 == ActionPoints1)
        return TieBreakSlot;
    return ActionPoints0 > ActionPoints1 ? 0 : 1;
}

/** Replicated match phase enum representing the state machine. */
UENUM(BlueprintType)
enum class EAWMatchPhase : uint8
{
    /** Players assemble command lists. */
    Programming UMETA(DisplayName = "Programming"),
    /** Command queues locked, awaiting both submissions. */
    Submission UMETA(DisplayName = "Submission"),
    /** Server executing deterministic simulation. */
    Simulation UMETA(DisplayName = "Simulation"),
    /** Simulation done, results revealed for replay inspection. */
    ReplayAutopsy UMETA(DisplayName = "ReplayAutopsy")
};

/** Result of a command-list validation attempt. */
USTRUCT(BlueprintType)
struct FAWValidationResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bSuccess = false;
    UPROPERTY(BlueprintReadOnly)
    FString ErrorMessage;
};

/** Result of a match as seen by UE gameplay code. */
USTRUCT(BlueprintType)
struct FAWMatchOutcome
{
    GENERATED_BODY()

    /** True when HP or AP depletion has ended the match. */
    UPROPERTY(BlueprintReadOnly)
    bool bMatchEnded = false;
    /** -1 = draw, 0 = player0 wins, 1 = player1 wins. */
    UPROPERTY(BlueprintReadOnly)
    int32 WinnerSlot = -1;
    /** Rule that ended the match. */
    UPROPERTY(BlueprintReadOnly)
    EAWMatchEndReason EndReason = EAWMatchEndReason::None;
    UPROPERTY(BlueprintReadOnly)
    int32 HP0 = 0;
    UPROPERTY(BlueprintReadOnly)
    int32 HP1 = 0;
};

/**
 * Complete replicated record required to reconstruct and present one resolved round.
 * The server publishes this as one property so consumers never assemble partial replay inputs.
 */
USTRUCT(BlueprintType)
struct FAWResolvedRound
{
    GENERATED_BODY()

    /** One-based match round identifier. */
    UPROPERTY(BlueprintReadOnly)
    int32 RoundNumber = 1;

    /** Tank slot whose complete command queue executes first. */
    UPROPERTY(BlueprintReadOnly)
    int32 StartingSlot = INDEX_NONE;

    /** Deterministic simulation seed for arena generation and replay. */
    UPROPERTY(BlueprintReadOnly)
    int64 Seed = 0;

    /** AP balances after programming costs and before simulation executes. */
    UPROPERTY(BlueprintReadOnly)
    int32 InitialActionPoints0 = Automata::InitialActionPoints;

    UPROPERTY(BlueprintReadOnly)
    int32 InitialActionPoints1 = Automata::InitialActionPoints;

    /** Persistent effects at simulation start. */
    UPROPERTY(BlueprintReadOnly)
    FAWRobotEffects InitialEffects0;

    UPROPERTY(BlueprintReadOnly)
    FAWRobotEffects InitialEffects1;

    /** Compact canonical arena placement at simulation start. */
    UPROPERTY(BlueprintReadOnly)
    TArray<uint8> InitialArenaState;

    /** Revealed command queues after authoritative execution. */
    UPROPERTY(BlueprintReadOnly)
    TArray<EAWCommand> Commands0;

    UPROPERTY(BlueprintReadOnly)
    TArray<EAWCommand> Commands1;

    /** Authoritative terminal or non-terminal result for this round. */
    UPROPERTY(BlueprintReadOnly)
    FAWMatchOutcome Outcome;

    /** Canonical final-state hash used for client desync verification. */
    UPROPERTY(BlueprintReadOnly)
    int64 AuthoritativeHash = 0;

    /** True only after commands, outcome, and hash have been committed together. */
    UPROPERTY(BlueprintReadOnly)
    bool bResolved = false;

    /** Return whether all inputs required for deterministic replay are available. */
    bool IsReadyForReplay() const
    {
        return bResolved && !Commands0.IsEmpty() && !Commands1.IsEmpty() &&
               StartingSlot >= 0 && StartingSlot <= 1;
    }
};

/** Convert a reflected round record to the compact Core replay payload. */
AUTOMATAWAR_API Automata::ReplayData MakeReplayData(const FAWResolvedRound &Round);

/** Convert a decoded Core replay payload to a reflected round record. */
AUTOMATAWAR_API FAWResolvedRound MakeResolvedRound(Automata::ReplayData Data);

/** Return whether a completed non-terminal round can enter its next programming phase. */
inline bool CanAdvanceToNextRound(EAWMatchPhase Phase, const FAWMatchOutcome &Outcome,
                                  TConstArrayView<EAWCommand> Commands0, TConstArrayView<EAWCommand> Commands1)
{
    return Phase == EAWMatchPhase::ReplayAutopsy && !Outcome.bMatchEnded &&
           !Commands0.IsEmpty() && !Commands1.IsEmpty();
}

/** Return whether a terminal result may be revealed after the visual replay completes. */
inline bool CanRevealMatchResult(EAWMatchPhase Phase, const FAWMatchOutcome &Outcome, bool bReplayCompleted)
{
    return Phase == EAWMatchPhase::ReplayAutopsy && Outcome.bMatchEnded && bReplayCompleted;
}

/** Info about a saved replay file. */
USTRUCT(BlueprintType)
struct FAWReplayInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString Filename;
    UPROPERTY(BlueprintReadOnly)
    FDateTime Timestamp;
    UPROPERTY(BlueprintReadOnly)
    int32 FileSizeBytes = 0;
};

/** Info about a discovered LAN session. */
USTRUCT(BlueprintType)
struct FAWSessionInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString SessionName;
    UPROPERTY(BlueprintReadOnly)
    FString HostName;
    UPROPERTY(BlueprintReadOnly)
    int32 CurrentPlayers = 0;
    UPROPERTY(BlueprintReadOnly)
    int32 MaxPlayers = 0;
    UPROPERTY(BlueprintReadOnly)
    int32 PingMs = 0;
};

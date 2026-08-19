#pragma once

/**
 * @file AWMatchTypes.h
 * @brief Shared enumerations and structs for the Automata War match orchestration layer.
 */

#include "CoreMinimal.h"
#include "AutomataWar/Core/AutomataRules.h"
#include "AWMatchTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAutomataGame, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAutomataNet, Log, All);

/** Canonical command and pickup effects that may survive into later rounds. */
USTRUCT(BlueprintType)
struct FAWRobotEffects
{
    GENERATED_BODY()

    /** The next incoming hit is reduced by half. */
    UPROPERTY(BlueprintReadOnly)
    bool bShieldCharged = false;

    /** The next move crosses up to two cells. */
    UPROPERTY(BlueprintReadOnly)
    bool bAccelerateNextMove = false;

    /** Remaining rounds with bonus projectile damage. */
    UPROPERTY(BlueprintReadOnly)
    int32 ExtraAmmoRounds = 0;

    /** Remaining rounds with all incoming damage reduced by half. */
    UPROPERTY(BlueprintReadOnly)
    int32 ShieldRounds = 0;

    /** Remaining rounds in which every move crosses up to two cells. */
    UPROPERTY(BlueprintReadOnly)
    int32 AcceleratorRounds = 0;
};

/** Return whether either canonical shield source is currently active. */
inline constexpr bool HasActiveShield(const FAWRobotEffects &Effects)
{
    return Effects.bShieldCharged || Effects.ShieldRounds > 0;
}

/** One player-selected action, executed relative to the tank's current facing. */
UENUM(BlueprintType)
enum class EAWCommand : uint8
{
    Move UMETA(DisplayName = "Move"),
    Fire UMETA(DisplayName = "Fire"),
    TurnLeft UMETA(DisplayName = "Turn Left"),
    TurnRight UMETA(DisplayName = "Turn Right"),
    Wait UMETA(DisplayName = "Wait"),
    ChargeShield UMETA(DisplayName = "Charge Shield"),
    Accelerate UMETA(DisplayName = "Accelerate"),
    Count UMETA(Hidden)
};

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

/** AP reserved while this command is present in a player's program. */
inline constexpr int32 GetActionPointCost(EAWCommand Command)
{
    switch (Command)
    {
    case EAWCommand::Move:
        return Automata::MoveActionPointCost;
    case EAWCommand::Fire:
        return Automata::FireActionPointCost;
    case EAWCommand::TurnLeft:
    case EAWCommand::TurnRight:
        return Automata::TurnActionPointCost;
    case EAWCommand::Wait:
        return Automata::WaitActionPointCost;
    case EAWCommand::ChargeShield:
        return Automata::ChargeShieldActionPointCost;
    case EAWCommand::Accelerate:
        return Automata::AccelerateActionPointCost;
    default:
        return 0;
    }
}

/** Total AP reserved by a complete command program. */
inline int32 GetProgramActionPointCost(TConstArrayView<EAWCommand> Commands)
{
    int32 Cost = 0;
    for (EAWCommand Command : Commands)
        Cost += GetActionPointCost(Command);
    return Cost;
}

/** Lowest AP balance that can still buy a command whose cost is greater than zero. */
inline constexpr int32 GetMinimumPositiveActionPointCost()
{
    int32 MinimumCost = Automata::InitialActionPoints;
    for (uint8 Value = 0; Value < static_cast<uint8>(EAWCommand::Count); ++Value)
    {
        const int32 Cost = GetActionPointCost(static_cast<EAWCommand>(Value));
        if (Cost > 0 && Cost < MinimumCost)
            MinimumCost = Cost;
    }
    return MinimumCost;
}

/** Human-readable label used by command lists and replay views. */
inline const TCHAR *LexToString(EAWCommand Command)
{
    switch (Command)
    {
    case EAWCommand::Move:
        return TEXT("MOVE");
    case EAWCommand::Fire:
        return TEXT("FIRE");
    case EAWCommand::TurnLeft:
        return TEXT("TURN LEFT");
    case EAWCommand::TurnRight:
        return TEXT("TURN RIGHT");
    case EAWCommand::Wait:
        return TEXT("WAIT");
    case EAWCommand::ChargeShield:
        return TEXT("CHARGE SHIELD");
    case EAWCommand::Accelerate:
        return TEXT("ACCELERATE");
    default:
        return TEXT("INVALID");
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

/** Rule that produced a terminal match result and selects its presentation message. */
UENUM(BlueprintType)
enum class EAWMatchEndReason : uint8
{
    None UMETA(DisplayName = "None"),
    Health UMETA(DisplayName = "Health"),
    ActionPoints UMETA(DisplayName = "Action Points")
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

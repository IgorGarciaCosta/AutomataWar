#pragma once

/**
 * @file AutomataDomainTypes.h
 * @brief Canonical reflected types shared by deterministic simulation and Unreal adapters.
 */

#include "CoreMinimal.h"
#include "AutomataRules.h"
#include "AutomataDomainTypes.generated.h"

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

/** Rule that produced a terminal match result and selects its presentation message. */
UENUM(BlueprintType)
enum class EAWMatchEndReason : uint8
{
    None UMETA(DisplayName = "None"),
    Health UMETA(DisplayName = "Health"),
    ActionPoints UMETA(DisplayName = "Action Points")
};

/** Return the AP reserved while a command is present in a player's program. */
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

/** Return the total AP reserved by a complete command program. */
inline int32 GetProgramActionPointCost(TConstArrayView<EAWCommand> Commands)
{
    int32 Cost = 0;
    for (EAWCommand Command : Commands)
        Cost += GetActionPointCost(Command);
    return Cost;
}

/** Return the lowest AP balance that can buy a command whose cost is greater than zero. */
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

/** Return the stable human-readable label used by command lists and replay views. */
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
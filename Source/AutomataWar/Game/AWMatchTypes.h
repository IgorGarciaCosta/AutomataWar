#pragma once

/**
 * @file AWMatchTypes.h
 * @brief Shared enumerations and structs for the Automata War match orchestration layer.
 */

#include "CoreMinimal.h"
#include "AWMatchTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAutomataGame, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAutomataNet, Log, All);

/** Replicated match phase enum representing the state machine. */
UENUM(BlueprintType)
enum class EAWMatchPhase : uint8
{
    /** Players write/edit scripts. */
    Programming UMETA(DisplayName = "Programming"),
    /** Scripts locked, awaiting both submissions. */
    Submission UMETA(DisplayName = "Submission"),
    /** Server executing deterministic simulation. */
    Simulation UMETA(DisplayName = "Simulation"),
    /** Simulation done, results revealed for replay inspection. */
    ReplayAutopsy UMETA(DisplayName = "ReplayAutopsy")
};

/** Result of a script validation attempt. */
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

    /** -1 = draw, 0 = player0 wins, 1 = player1 wins. */
    UPROPERTY(BlueprintReadOnly)
    int32 WinnerSlot = -1;
    UPROPERTY(BlueprintReadOnly)
    int32 FinalTick = 0;
    UPROPERTY(BlueprintReadOnly)
    int32 HP0 = 0;
    UPROPERTY(BlueprintReadOnly)
    int32 HP1 = 0;
};

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

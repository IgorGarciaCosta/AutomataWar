#pragma once

/**
 * @file AWGameSubsystem.h
 * @brief GameInstance subsystem providing the full public BlueprintCallable API.
 *
 * Handles: local match start, hosting, LAN discovery, joining,
 * replay service, and phase/status delegates for a later C++ UMG UI.
 */

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "AWMatchTypes.h"
#include "AWGameSubsystem.generated.h"

class AAWGameMode;

/**
 * @brief Top-level API subsystem for all Automata War game operations.
 *
 * Survives map travel. Manages online session lifecycle with proper
 * delegate cleanup. All session operations use OnlineSubsystem NULL.
 */
UCLASS()
class AUTOMATAWAR_API UAWGameSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /** Remove online delegates and release subsystem-owned session state. */
    virtual void Deinitialize() override;

    // ─── Local Match ─────────────────────────────────────────────────────────

    /** Start a local hot-seat match with the selected difficulty and arena. */
    UFUNCTION(BlueprintCallable, Category = "AutomataWar|Local")
    void StartLocalMatch(EAWDifficulty Difficulty, EAWArenaSize ArenaSize);

    /** Start a local match with the selected AI difficulty and procedural arena size. */
    UFUNCTION(BlueprintCallable, Category = "AutomataWar|Local")
    void StartSinglePlayerMatch(EAWDifficulty Difficulty, EAWArenaSize ArenaSize);

    /** Advance to next round (local or host). */
    UFUNCTION(BlueprintCallable, Category = "AutomataWar|Match")
    void NextRound();

    // ─── Networking ──────────────────────────────────────────────────────────

    /** Host a LAN listen-server session. */
    UFUNCTION(BlueprintCallable, Category = "AutomataWar|Network")
    void HostSession(const FString &SessionName);

    /** Refresh LAN session list. */
    UFUNCTION(BlueprintCallable, Category = "AutomataWar|Network")
    void RefreshSessions();

    /** Join a session by index in the last search results. */
    UFUNCTION(BlueprintCallable, Category = "AutomataWar|Network")
    void JoinSessionByIndex(int32 Index);

    /** Join a session by direct IP address. */
    UFUNCTION(BlueprintCallable, Category = "AutomataWar|Network")
    void JoinByIP(const FString &IPAddress);

    /** Destroy current session and return to menu. */
    UFUNCTION(BlueprintCallable, Category = "AutomataWar|Network")
    void DestroySession();

    /** Discovered sessions from last refresh. */
    UFUNCTION(BlueprintCallable, Category = "AutomataWar|Network")
    TArray<FAWSessionInfo> GetSessionList() const { return CachedSessions; }

    /** Delegate for session list update. */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSessionsRefreshed);
    UPROPERTY(BlueprintAssignable, Category = "AutomataWar|Network")
    FOnSessionsRefreshed OnSessionsRefreshed;

    /** Delegate for network errors. */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNetworkError, const FString &, ErrorMessage);
    UPROPERTY(BlueprintAssignable, Category = "AutomataWar|Network")
    FOnNetworkError OnNetworkError;

    // ─── Replay ──────────────────────────────────────────────────────────────

    /** List all saved replays. */
    UFUNCTION(BlueprintCallable, Category = "AutomataWar|Replay")
    TArray<FAWReplayInfo> GetReplayList();

    /** Save current match as replay. */
    UFUNCTION(BlueprintCallable, Category = "AutomataWar|Replay")
    bool SaveReplay(const FString &Filename);

    /** Load a replay by filename as one complete re-simulation record. */
    UFUNCTION(BlueprintCallable, Category = "AutomataWar|Replay")
    bool LoadReplay(const FString &Filename, FAWResolvedRound &OutRound, FString &OutError);

    /** Delete a replay. */
    UFUNCTION(BlueprintCallable, Category = "AutomataWar|Replay")
    bool DeleteReplay(const FString &Filename);

    /** Export replay to base64. */
    UFUNCTION(BlueprintCallable, Category = "AutomataWar|Replay")
    bool ExportReplayBase64(const FString &Filename, FString &OutBase64);

    /** Import replay from base64. */
    UFUNCTION(BlueprintCallable, Category = "AutomataWar|Replay")
    bool ImportReplayBase64(const FString &Base64Data, const FString &Filename, FString &OutError);

    /** Delegate broadcast on any error from this subsystem. */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnError, const FString &, Message);
    UPROPERTY(BlueprintAssignable, Category = "AutomataWar|Status")
    FOnError OnError;

protected:
    void OnCreateSessionComplete(FName SessionName, bool bSuccess);
    void OnFindSessionsComplete(bool bSuccess);
    void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
    void OnDestroySessionComplete(FName SessionName, bool bSuccess);

    void CleanupSessionDelegates();

    TArray<FAWSessionInfo> CachedSessions;
    TSharedPtr<FOnlineSessionSearch> SessionSearch;

    FDelegateHandle CreateHandle;
    FDelegateHandle FindHandle;
    FDelegateHandle JoinHandle;
    FDelegateHandle DestroyHandle;
};

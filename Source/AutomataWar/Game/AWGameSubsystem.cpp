#include "AWGameSubsystem.h"
#include "AWGameMode.h"
#include "AWGameState.h"
#include "AWReplayService.h"
#include "AutomataWar/Core/Replay/AutomataReplay.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

void UAWGameSubsystem::Deinitialize()
{
    CleanupSessionDelegates();
    Super::Deinitialize();
}

// --- Local Match ------------------------------------------------------------

void UAWGameSubsystem::StartLocalMatch(EAWDifficulty Difficulty, EAWArenaSize ArenaSize)
{
    UGameInstance *GI = GetGameInstance();
    if (!GI)
        return;

    UWorld *World = GI->GetWorld();
    if (!World)
        return;

    if (AAWGameMode *GM = World->GetAuthGameMode<AAWGameMode>())
    {
        GM->BeginLocalMatch(Difficulty, ArenaSize);
        return;
    }

    OnError.Broadcast(TEXT("Local match requires the Automata War arena."));
}

void UAWGameSubsystem::StartSinglePlayerMatch(EAWDifficulty Difficulty, EAWArenaSize ArenaSize)
{
    UWorld *World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
    if (AAWGameMode *GM = World ? World->GetAuthGameMode<AAWGameMode>() : nullptr)
    {
        GM->BeginSinglePlayerMatch(Difficulty, ArenaSize);
        return;
    }

    OnError.Broadcast(TEXT("Single-player match requires the Automata War arena."));
}

void UAWGameSubsystem::NextRound()
{
    UWorld *World = GetGameInstance()->GetWorld();
    if (!World)
        return;

    if (AAWGameMode *GM = World->GetAuthGameMode<AAWGameMode>())
    {
        GM->AdvanceToNextRound();
    }
}

// ─── Networking ──────────────────────────────────────────────────────────────

void UAWGameSubsystem::HostSession(const FString &SessionName)
{
    IOnlineSubsystem *OSS = IOnlineSubsystem::Get();
    if (!OSS)
    {
        OnNetworkError.Broadcast(TEXT("OnlineSubsystem not available."));
        return;
    }

    IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
    if (!Sessions.IsValid())
    {
        OnNetworkError.Broadcast(TEXT("Session interface not available."));
        return;
    }

    CleanupSessionDelegates();

    CreateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
        FOnCreateSessionCompleteDelegate::CreateUObject(this, &UAWGameSubsystem::OnCreateSessionComplete));

    FOnlineSessionSettings Settings;
    Settings.bIsLANMatch = true;
    Settings.NumPublicConnections = 2;
    Settings.bShouldAdvertise = true;
    Settings.bUsesPresence = false;
    Settings.Set(FName("SESSION_NAME"), SessionName, EOnlineDataAdvertisementType::ViaOnlineService);

    Sessions->CreateSession(0, NAME_GameSession, Settings);
}

void UAWGameSubsystem::OnCreateSessionComplete(FName SessionName, bool bSuccess)
{
    if (!bSuccess)
    {
        OnNetworkError.Broadcast(TEXT("Failed to create session."));
        return;
    }

    // Start listen server
    UWorld *World = GetGameInstance()->GetWorld();
    if (World)
    {
        World->ServerTravel(TEXT("/Game/Maps/L_AutomataArena?listen"), true);
    }
}

void UAWGameSubsystem::RefreshSessions()
{
    IOnlineSubsystem *OSS = IOnlineSubsystem::Get();
    if (!OSS)
        return;

    IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
    if (!Sessions.IsValid())
        return;

    CleanupSessionDelegates();

    FindHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
        FOnFindSessionsCompleteDelegate::CreateUObject(this, &UAWGameSubsystem::OnFindSessionsComplete));

    SessionSearch = MakeShareable(new FOnlineSessionSearch());
    SessionSearch->bIsLanQuery = true;
    SessionSearch->MaxSearchResults = 20;

    Sessions->FindSessions(0, SessionSearch.ToSharedRef());
}

void UAWGameSubsystem::OnFindSessionsComplete(bool bSuccess)
{
    CachedSessions.Reset();

    if (bSuccess && SessionSearch.IsValid())
    {
        for (const FOnlineSessionSearchResult &Result : SessionSearch->SearchResults)
        {
            FAWSessionInfo Info;
            Result.Session.SessionSettings.Get(FName("SESSION_NAME"), Info.SessionName);
            Info.HostName = Result.Session.OwningUserName;
            Info.CurrentPlayers = Result.Session.SessionSettings.NumPublicConnections - Result.Session.NumOpenPublicConnections;
            Info.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
            Info.PingMs = Result.PingInMs;
            CachedSessions.Add(MoveTemp(Info));
        }
    }

    OnSessionsRefreshed.Broadcast();
}

void UAWGameSubsystem::JoinSessionByIndex(int32 Index)
{
    if (!SessionSearch.IsValid() || !SessionSearch->SearchResults.IsValidIndex(Index))
    {
        OnNetworkError.Broadcast(TEXT("Invalid session index."));
        return;
    }

    IOnlineSubsystem *OSS = IOnlineSubsystem::Get();
    if (!OSS)
        return;

    IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
    if (!Sessions.IsValid())
        return;

    CleanupSessionDelegates();

    JoinHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
        FOnJoinSessionCompleteDelegate::CreateUObject(this, &UAWGameSubsystem::OnJoinSessionComplete));

    Sessions->JoinSession(0, NAME_GameSession, SessionSearch->SearchResults[Index]);
}

void UAWGameSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    if (Result != EOnJoinSessionCompleteResult::Success)
    {
        OnNetworkError.Broadcast(FString::Printf(TEXT("Join failed (code %d)."), static_cast<int32>(Result)));
        return;
    }

    IOnlineSubsystem *OSS = IOnlineSubsystem::Get();
    if (!OSS)
        return;

    IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
    FString ConnectInfo;
    if (Sessions->GetResolvedConnectString(NAME_GameSession, ConnectInfo))
    {
        APlayerController *PC = GetGameInstance()->GetFirstLocalPlayerController();
        if (PC)
        {
            PC->ClientTravel(ConnectInfo, TRAVEL_Absolute);
        }
    }
}

void UAWGameSubsystem::JoinByIP(const FString &IPAddress)
{
    APlayerController *PC = GetGameInstance()->GetFirstLocalPlayerController();
    if (PC)
    {
        PC->ClientTravel(IPAddress, TRAVEL_Absolute);
    }
}

void UAWGameSubsystem::DestroySession()
{
    IOnlineSubsystem *OSS = IOnlineSubsystem::Get();
    if (!OSS)
        return;

    IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
    if (!Sessions.IsValid())
        return;

    CleanupSessionDelegates();

    DestroyHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
        FOnDestroySessionCompleteDelegate::CreateUObject(this, &UAWGameSubsystem::OnDestroySessionComplete));

    Sessions->DestroySession(NAME_GameSession);
}

void UAWGameSubsystem::OnDestroySessionComplete(FName SessionName, bool bSuccess)
{
    UGameplayStatics::OpenLevel(GetGameInstance()->GetWorld(), TEXT("L_AutomataArena"));
}

void UAWGameSubsystem::CleanupSessionDelegates()
{
    IOnlineSubsystem *OSS = IOnlineSubsystem::Get();
    if (!OSS)
        return;

    IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
    if (!Sessions.IsValid())
        return;

    if (CreateHandle.IsValid())
    {
        Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
        CreateHandle.Reset();
    }
    if (FindHandle.IsValid())
    {
        Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
        FindHandle.Reset();
    }
    if (JoinHandle.IsValid())
    {
        Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
        JoinHandle.Reset();
    }
    if (DestroyHandle.IsValid())
    {
        Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
        DestroyHandle.Reset();
    }
}

// ─── Replay ──────────────────────────────────────────────────────────────────

TArray<FAWReplayInfo> UAWGameSubsystem::GetReplayList()
{
    TArray<FAWReplayInfo> Infos;
    FAWReplayService::List(Infos);
    return Infos;
}

bool UAWGameSubsystem::SaveReplay(const FString &Filename)
{
    UWorld *World = GetGameInstance()->GetWorld();
    if (!World)
        return false;

    AAWGameState *GS = World->GetGameState<AAWGameState>();
    if (!GS || GS->Phase != EAWMatchPhase::ReplayAutopsy)
        return false;

    return GS->ResolvedRound.IsReadyForReplay() &&
           FAWReplayService::Save(Filename, MakeReplayData(GS->ResolvedRound));
}

bool UAWGameSubsystem::LoadReplay(const FString &Filename, FAWResolvedRound &OutRound, FString &OutError)
{
    Automata::ReplayData Data;
    if (!FAWReplayService::Load(Filename, Data, OutError))
    {
        return false;
    }

    OutRound = MakeResolvedRound(MoveTemp(Data));
    return true;
}

bool UAWGameSubsystem::DeleteReplay(const FString &Filename)
{
    return FAWReplayService::Delete(Filename);
}

bool UAWGameSubsystem::ExportReplayBase64(const FString &Filename, FString &OutBase64)
{
    return FAWReplayService::ExportBase64(Filename, OutBase64);
}

bool UAWGameSubsystem::ImportReplayBase64(const FString &Base64Data, const FString &Filename, FString &OutError)
{
    return FAWReplayService::ImportBase64(Base64Data, Filename, OutError);
}

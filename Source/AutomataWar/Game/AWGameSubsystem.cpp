#include "AWGameSubsystem.h"
#include "AWGameMode.h"
#include "AWGameState.h"
#include "AWReplayService.h"
#include "AWExampleScripts.h"
#include "AutomataWar/Core/Lang/AutomataCompiler.h"
#include "AutomataWar/Core/Sim/AutomataSimulation.h"
#include "AutomataWar/Core/Replay/AutomataReplay.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

void UAWGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UAWGameSubsystem::Deinitialize()
{
	CleanupSessionDelegates();
	Super::Deinitialize();
}

// ─── Local Match ─────────────────────────────────────────────────────────────

void UAWGameSubsystem::StartLocalMatch()
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	UWorld* World = GI->GetWorld();
	if (!World) return;

	// Open the arena map in standalone
	UGameplayStatics::OpenLevel(World, TEXT("L_AutomataArena"), true, TEXT("?listen"));
}

FAWValidationResult UAWGameSubsystem::SubmitLocalScript(int32 Slot, const FString& Source)
{
	UWorld* World = GetGameInstance()->GetWorld();
	if (!World)
	{
		FAWValidationResult R;
		R.ErrorMessage = TEXT("No active world.");
		return R;
	}

	AAWGameMode* GM = World->GetAuthGameMode<AAWGameMode>();
	if (!GM)
	{
		FAWValidationResult R;
		R.ErrorMessage = TEXT("No GameMode (not server).");
		return R;
	}

	return GM->HandleSubmission(Slot, Source);
}

void UAWGameSubsystem::NextRound()
{
	UWorld* World = GetGameInstance()->GetWorld();
	if (!World) return;

	if (AAWGameMode* GM = World->GetAuthGameMode<AAWGameMode>())
	{
		GM->AdvanceToNextRound();
	}
}

// ─── Networking ──────────────────────────────────────────────────────────────

void UAWGameSubsystem::HostSession(const FString& SessionName)
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
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
	UWorld* World = GetGameInstance()->GetWorld();
	if (World)
	{
		World->ServerTravel(TEXT("/Game/Maps/L_AutomataArena?listen"), true);
	}
}

void UAWGameSubsystem::RefreshSessions()
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS) return;

	IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
	if (!Sessions.IsValid()) return;

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
		for (const FOnlineSessionSearchResult& Result : SessionSearch->SearchResults)
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

	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS) return;

	IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
	if (!Sessions.IsValid()) return;

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

	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS) return;

	IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
	FString ConnectInfo;
	if (Sessions->GetResolvedConnectString(NAME_GameSession, ConnectInfo))
	{
		APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController();
		if (PC)
		{
			PC->ClientTravel(ConnectInfo, TRAVEL_Absolute);
		}
	}
}

void UAWGameSubsystem::JoinByIP(const FString& IPAddress)
{
	APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController();
	if (PC)
	{
		PC->ClientTravel(IPAddress, TRAVEL_Absolute);
	}
}

void UAWGameSubsystem::DestroySession()
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS) return;

	IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
	if (!Sessions.IsValid()) return;

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
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS) return;

	IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
	if (!Sessions.IsValid()) return;

	if (CreateHandle.IsValid()) { Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle); CreateHandle.Reset(); }
	if (FindHandle.IsValid()) { Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle); FindHandle.Reset(); }
	if (JoinHandle.IsValid()) { Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle); JoinHandle.Reset(); }
	if (DestroyHandle.IsValid()) { Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle); DestroyHandle.Reset(); }
}

// ─── Training ────────────────────────────────────────────────────────────────

void UAWGameSubsystem::RunTraining(const FString& UserSource, int32 Iterations, int32& OutWins, int32& OutLosses, int32& OutDraws)
{
	OutWins = OutLosses = OutDraws = 0;

	std::string UserSrc = TCHAR_TO_UTF8(*UserSource);
	Automata::CompileResult UserCompile = Automata::Compile(UserSrc);
	if (!UserCompile.Ok()) return;

	std::string BotSrc = TCHAR_TO_UTF8(*FAWExampleScripts::DefaultBot());
	Automata::CompileResult BotCompile = Automata::Compile(BotSrc);
	if (!BotCompile.Ok()) return;

	Iterations = FMath::Clamp(Iterations, 1, 1000);

	for (int32 i = 0; i < Iterations; ++i)
	{
		Automata::SimConfig Config;
		Config.seed = static_cast<uint64>(i * 7919 + 42);

		Automata::Simulation Sim;
		Automata::MatchResult Result = Sim.RunMatch(UserCompile.program, BotCompile.program, Config);

		switch (Result.outcome)
		{
		case Automata::MatchOutcome::Robot0Wins: ++OutWins; break;
		case Automata::MatchOutcome::Robot1Wins: ++OutLosses; break;
		default: ++OutDraws; break;
		}
	}
}

// ─── Replay ──────────────────────────────────────────────────────────────────

TArray<FAWReplayInfo> UAWGameSubsystem::GetReplayList()
{
	TArray<FAWReplayInfo> Infos;
	FAWReplayService::List(Infos);
	return Infos;
}

bool UAWGameSubsystem::SaveReplay(const FString& Filename)
{
	UWorld* World = GetGameInstance()->GetWorld();
	if (!World) return false;

	AAWGameState* GS = World->GetGameState<AAWGameState>();
	if (!GS || GS->Phase != EAWMatchPhase::ReplayAutopsy) return false;

	Automata::ReplayData Data;
	Data.seed = static_cast<uint64>(GS->SimSeed);
	Data.sourceA = TCHAR_TO_UTF8(*GS->RevealedSource0);
	Data.sourceB = TCHAR_TO_UTF8(*GS->RevealedSource1);

	return FAWReplayService::Save(Filename, Data);
}

bool UAWGameSubsystem::LoadReplay(const FString& Filename, FString& OutSource0, FString& OutSource1, int64& OutSeed, FString& OutError)
{
	Automata::ReplayData Data;
	if (!FAWReplayService::Load(Filename, Data, OutError))
	{
		return false;
	}

	OutSource0 = UTF8_TO_TCHAR(Data.sourceA.c_str());
	OutSource1 = UTF8_TO_TCHAR(Data.sourceB.c_str());
	OutSeed = static_cast<int64>(Data.seed);
	return true;
}

bool UAWGameSubsystem::DeleteReplay(const FString& Filename)
{
	return FAWReplayService::Delete(Filename);
}

bool UAWGameSubsystem::ExportReplayBase64(const FString& Filename, FString& OutBase64)
{
	return FAWReplayService::ExportBase64(Filename, OutBase64);
}

bool UAWGameSubsystem::ImportReplayBase64(const FString& Base64Data, const FString& Filename, FString& OutError)
{
	return FAWReplayService::ImportBase64(Base64Data, Filename, OutError);
}

// ─── Status ──────────────────────────────────────────────────────────────────

EAWMatchPhase UAWGameSubsystem::GetCurrentPhase() const
{
	UWorld* World = GetGameInstance()->GetWorld();
	if (!World) return EAWMatchPhase::Programming;

	AAWGameState* GS = World->GetGameState<AAWGameState>();
	return GS ? GS->Phase : EAWMatchPhase::Programming;
}

FAWMatchOutcome UAWGameSubsystem::GetOutcome() const
{
	UWorld* World = GetGameInstance()->GetWorld();
	if (!World) return FAWMatchOutcome();

	AAWGameState* GS = World->GetGameState<AAWGameState>();
	return GS ? GS->Outcome : FAWMatchOutcome();
}

void UAWGameSubsystem::GetRevealedScripts(FString& OutSource0, FString& OutSource1) const
{
	UWorld* World = GetGameInstance()->GetWorld();
	if (!World) return;

	AAWGameState* GS = World->GetGameState<AAWGameState>();
	if (GS)
	{
		OutSource0 = GS->RevealedSource0;
		OutSource1 = GS->RevealedSource1;
	}
}

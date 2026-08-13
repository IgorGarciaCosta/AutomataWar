#include "AWHUDWidget.h"
#include "AWScreenWidget.h"
#include "AWUITypes.h"
#include "AutomataWar/Game/AWGameSubsystem.h"
#include "AutomataWar/Game/AWGameState.h"
#include "AutomataWar/Game/AWGameMode.h"
#include "AutomataWar/Game/AWReplayService.h"
#include "AutomataWar/Game/AWPlayerController.h"
#include "AutomataWar/Core/Replay/AutomataReplay.h"
#include "AutomataWar/Visual/AWArenaRenderer.h"
#include "AutomataWar/Visual/AWVisualTypes.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/GameStateBase.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Sound/SoundBase.h"

namespace
{
    const TCHAR *EventName(Automata::EventType Type)
    {
        switch (Type)
        {
        case Automata::EventType::Move:
            return TEXT("moved");
        case Automata::EventType::MoveBlockedWall:
            return TEXT("bumped wall");
        case Automata::EventType::MoveBlockedCover:
            return TEXT("blocked by cover");
        case Automata::EventType::MoveBlockedRobot:
            return TEXT("blocked by robot");
        case Automata::EventType::Turn:
            return TEXT("turned");
        case Automata::EventType::Fire:
            return TEXT("fired");
        case Automata::EventType::Hit:
            return TEXT("hit");
        case Automata::EventType::ShotBlocked:
            return TEXT("shot blocked");
        default:
            return TEXT("unknown");
        }
    }

    const TCHAR *FacingName(Automata::Dir Direction)
    {
        static const TCHAR *Names[] = {TEXT("NORTH"), TEXT("EAST"), TEXT("SOUTH"), TEXT("WEST")};
        return Names[static_cast<int32>(Direction)];
    }
}

void UAWHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    UAWScreenWidget *Screens[] = {
        MainMenuScreenWidget, ProgrammingScreenWidget, ReplayAutopsyScreenWidget,
        ReplayBrowserScreenWidget, LanguageReferenceScreenWidget};
    for (UAWScreenWidget *Screen : Screens)
    {
        if (Screen)
            Screen->OnAction.AddUObject(this, &UAWHUDWidget::OnScreenAction);
    }
    if (ReplayAutopsyScreenWidget)
        ReplayAutopsyScreenWidget->OnScrubbed.AddUObject(this, &UAWHUDWidget::OnReplayScrubChanged);

    PopulateLanguageReference();

    if (UWorld *World = GetWorld())
    {
        if (AAWGameState *GS = World->GetGameState<AAWGameState>())
        {
            GS->OnPhaseChanged.AddDynamic(this, &UAWHUDWidget::OnPhaseChanged);
        }
    }
    if (UAWGameSubsystem *Sub = GetSubsystem())
    {
        Sub->OnError.AddDynamic(this, &UAWHUDWidget::OnErrorReceived);
        Sub->OnNetworkError.AddDynamic(this, &UAWHUDWidget::OnErrorReceived);
        Sub->OnSessionsRefreshed.AddDynamic(this, &UAWHUDWidget::OnSessionsRefreshed);
    }
    ShowScreen(InitialScreen);

#if !UE_BUILD_SHIPPING
    FString CaptureMode;
    if (FParse::Value(FCommandLine::Get(), TEXT("AutomataCapture="), CaptureMode))
    {
        if (CaptureMode.Equals(TEXT("Programming"), ESearchCase::IgnoreCase))
        {
            OnLocalMatch();
        }
        else if (CaptureMode.Equals(TEXT("Replay"), ESearchCase::IgnoreCase))
        {
            OnLocalMatch();
            if (UAWGameSubsystem *Sub = GetSubsystem())
            {
                Sub->SubmitLocalCommands(0, {EAWCommand::Move, EAWCommand::Move, EAWCommand::Fire});
                Sub->SubmitLocalCommands(1, {EAWCommand::TurnRight, EAWCommand::Move, EAWCommand::Fire});
            }
        }
    }
#endif
}

void UAWHUDWidget::NativeDestruct()
{
    UAWScreenWidget *Screens[] = {
        MainMenuScreenWidget, ProgrammingScreenWidget, ReplayAutopsyScreenWidget,
        ReplayBrowserScreenWidget, LanguageReferenceScreenWidget};
    for (UAWScreenWidget *Screen : Screens)
    {
        if (Screen)
            Screen->OnAction.RemoveAll(this);
    }
    if (ReplayAutopsyScreenWidget)
        ReplayAutopsyScreenWidget->OnScrubbed.RemoveAll(this);

    if (UWorld *World = GetWorld())
    {
        if (AAWGameState *GS = World->GetGameState<AAWGameState>())
        {
            GS->OnPhaseChanged.RemoveDynamic(this, &UAWHUDWidget::OnPhaseChanged);
        }
    }
    if (UAWGameSubsystem *Sub = GetSubsystem())
    {
        Sub->OnError.RemoveDynamic(this, &UAWHUDWidget::OnErrorReceived);
        Sub->OnNetworkError.RemoveDynamic(this, &UAWHUDWidget::OnErrorReceived);
        Sub->OnSessionsRefreshed.RemoveDynamic(this, &UAWHUDWidget::OnSessionsRefreshed);
    }
    Super::NativeDestruct();
}

void UAWHUDWidget::NativeTick(const FGeometry &MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (bReplayPlaying && CurrentScreen == EAWScreen::ReplayAutopsy && ReplayController.IsValid() && ReplayController->IsValid())
    {
        ReplayAccumulator += InDeltaTime * ReplaySpeed;
        const double StepInterval = 1.0 / 10.0;
        while (ReplayAccumulator >= StepInterval)
        {
            ReplayAccumulator -= StepInterval;
            if (!ReplayController->StepForward())
            {
                bReplayPlaying = false;
                break;
            }
        }
        UpdateReplayUI();
        UpdateArenaFromReplay();
    }
}

int32 UAWHUDWidget::GetScreenCount() const
{
    return ScreenSwitcher ? ScreenSwitcher->GetNumWidgets() : 0;
}

int32 UAWHUDWidget::GetActiveScreenIndex() const
{
    return ScreenSwitcher ? ScreenSwitcher->GetActiveWidgetIndex() : -1;
}

UAWGameSubsystem *UAWHUDWidget::GetSubsystem() const
{
    if (UGameInstance *GI = GetGameInstance())
    {
        return GI->GetSubsystem<UAWGameSubsystem>();
    }
    return nullptr;
}

void UAWHUDWidget::ShowScreen(EAWScreen Screen)
{
    CurrentScreen = Screen;
    if (ScreenSwitcher)
    {
        ScreenSwitcher->SetActiveWidgetIndex(static_cast<int32>(Screen));
    }
}

void UAWHUDWidget::OnScreenAction(EAWUIAction Action)
{
    switch (Action)
    {
    case EAWUIAction::LocalMatch:
        OnLocalMatch();
        break;
    case EAWUIAction::HostLAN:
        OnHostLAN();
        break;
    case EAWUIAction::FindLAN:
        OnFindLAN();
        break;
    case EAWUIAction::JoinSession:
        OnJoinSelectedSession();
        break;
    case EAWUIAction::JoinIP:
        OnJoinIP();
        break;
    case EAWUIAction::OpenReplayBrowser:
        OnReplayBrowserNav();
        break;
    case EAWUIAction::OpenLanguageReference:
        OnLanguageRef();
        break;
    case EAWUIAction::Quit:
        OnQuit();
        break;
    case EAWUIAction::BackToMainMenu:
        OnBackToMainMenu();
        break;
    case EAWUIAction::SubmitP1:
        OnSubmitP1();
        break;
    case EAWUIAction::SubmitP2:
        OnSubmitP2();
        break;
    case EAWUIAction::ReplayStart:
        OnReplayScrubStart();
        break;
    case EAWUIAction::ReplayBack:
        OnReplayStepBack();
        break;
    case EAWUIAction::ReplayPause:
        OnReplayPause();
        break;
    case EAWUIAction::ReplayPlay:
        OnReplayPlay();
        break;
    case EAWUIAction::ReplayStep:
        OnReplayStep();
        break;
    case EAWUIAction::ReplaySpeedQuarter:
        OnReplaySpeedQuarter();
        break;
    case EAWUIAction::ReplaySpeedNormal:
        OnReplaySpeedNormal();
        break;
    case EAWUIAction::ReplaySpeedDouble:
        OnReplaySpeedDouble();
        break;
    case EAWUIAction::ReplaySpeedQuadruple:
        OnReplaySpeedQuadruple();
        break;
    case EAWUIAction::NextRound:
        OnNextRound();
        break;
    case EAWUIAction::ReplayRefresh:
        OnReplayRefresh();
        break;
    case EAWUIAction::ReplaySave:
        OnReplaySave();
        break;
    case EAWUIAction::ReplayLoad:
        OnReplayLoadSelected();
        break;
    case EAWUIAction::ReplayExport:
        OnReplayExportSelected();
        break;
    case EAWUIAction::ReplayImport:
        OnReplayImport();
        break;
    }
}

void UAWHUDWidget::OnPhaseChanged(EAWMatchPhase NewPhase)
{
    switch (NewPhase)
    {
    case EAWMatchPhase::Programming:
        ShowScreen(EAWScreen::Programming);
        break;
    case EAWMatchPhase::Simulation:
        if (SimulationScreenWidget && ProgrammingScreenWidget)
            SimulationScreenWidget->SetCommands(ProgrammingScreenWidget->GetCommands(0), ProgrammingScreenWidget->GetCommands(1));
        PlayUISound(AWVisualAssets::SFX_MatchStart);
        ShowScreen(EAWScreen::Simulation);
        break;
    case EAWMatchPhase::ReplayAutopsy:
        PlayUISound(AWVisualAssets::SFX_MatchEnd);
        InitializeReplayFromGameState();
        ShowScreen(EAWScreen::ReplayAutopsy);
        break;
    default:
        break;
    }
}

void UAWHUDWidget::OnErrorReceived(const FString &Message)
{
    SetStatus(Message, true);
}

void UAWHUDWidget::OnSessionsRefreshed()
{
    RefreshSessionList();
}

void UAWHUDWidget::SetStatus(const FString &Msg, bool bError)
{
    if (StatusText)
    {
        StatusText->SetText(FText::FromString(Msg));
        StatusText->SetColorAndOpacity(FSlateColor(bError ? AWUIColors::ErrorRed : AWUIColors::SuccessGreen));
    }
    if (bError)
        PlayUISound(AWUIAssets::SFX_UIError);
}

void UAWHUDWidget::PlayUISound(const TCHAR *AssetPath) const
{
    if (USoundBase *Sound = LoadObject<USoundBase>(nullptr, AssetPath))
    {
        UGameplayStatics::PlaySound2D(this, Sound);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Actions
// ═══════════════════════════════════════════════════════════════════════════════

void UAWHUDWidget::OnLocalMatch()
{
    PlayUISound(AWUIAssets::SFX_UIConfirm);
    if (UAWGameSubsystem *Sub = GetSubsystem())
    {
        Sub->StartLocalMatch();
    }
    ShowScreen(EAWScreen::Programming);
}

void UAWHUDWidget::OnHostLAN()
{
    PlayUISound(AWUIAssets::SFX_UIConfirm);
    if (UAWGameSubsystem *Sub = GetSubsystem())
    {
        Sub->HostSession(TEXT("AutomataWar"));
    }
    SetStatus(TEXT("Hosting LAN session..."));
}

void UAWHUDWidget::OnFindLAN()
{
    PlayUISound(AWUIAssets::SFX_UIConfirm);
    if (UAWGameSubsystem *Sub = GetSubsystem())
    {
        Sub->RefreshSessions();
        SetStatus(TEXT("Searching for LAN sessions..."));
    }
}

void UAWHUDWidget::RefreshSessionList()
{
    if (!MainMenuScreenWidget)
        return;

    MainMenuScreenWidget->ResetSessions();
    UAWGameSubsystem *Sub = GetSubsystem();
    if (!Sub)
        return;

    const TArray<FAWSessionInfo> Sessions = Sub->GetSessionList();
    for (const FAWSessionInfo &Session : Sessions)
    {
        const FString Label = FString::Printf(TEXT("%s | %s | %d/%d | %d ms"), *Session.SessionName,
                                              *Session.HostName, Session.CurrentPlayers, Session.MaxPlayers, Session.PingMs);
        MainMenuScreenWidget->AddSession(Label);
    }
    if (Sessions.IsEmpty())
        SetStatus(TEXT("No LAN sessions found."), true);
    else
        MainMenuScreenWidget->SelectFirstSession();
}

void UAWHUDWidget::OnJoinSelectedSession()
{
    const int32 SessionIndex = MainMenuScreenWidget ? MainMenuScreenWidget->GetSelectedSessionIndex() : INDEX_NONE;
    if (SessionIndex == INDEX_NONE)
    {
        SetStatus(TEXT("Select a LAN session first."), true);
        return;
    }

    if (UAWGameSubsystem *Sub = GetSubsystem())
    {
        PlayUISound(AWUIAssets::SFX_UIConfirm);
        Sub->JoinSessionByIndex(SessionIndex);
        SetStatus(TEXT("Joining LAN session..."));
    }
}

void UAWHUDWidget::OnJoinIP()
{
    const FString IP = MainMenuScreenWidget ? MainMenuScreenWidget->GetJoinIPAddress() : FString();
    if (IP.IsEmpty())
    {
        SetStatus(TEXT("Enter an IP address."), true);
        return;
    }
    PlayUISound(AWUIAssets::SFX_UIConfirm);
    if (UAWGameSubsystem *Sub = GetSubsystem())
    {
        Sub->JoinByIP(IP);
    }
    SetStatus(FString::Printf(TEXT("Joining %s..."), *IP));
}

void UAWHUDWidget::OnReplayBrowserNav()
{
    RefreshReplayList();
    ShowScreen(EAWScreen::ReplayBrowser);
}

void UAWHUDWidget::OnLanguageRef() { ShowScreen(EAWScreen::LanguageReference); }

void UAWHUDWidget::OnBackToMainMenu() { ShowScreen(EAWScreen::MainMenu); }

void UAWHUDWidget::OnQuit()
{
    if (UWorld *World = GetWorld())
    {
        if (APlayerController *PC = World->GetFirstPlayerController())
        {
            UKismetSystemLibrary::QuitGame(World, PC, EQuitPreference::Quit, false);
        }
    }
}

void UAWHUDWidget::OnSubmitSlot(int32 SlotIndex)
{
    if (UAWGameSubsystem *Sub = GetSubsystem())
    {
        const TArray<EAWCommand> Commands = ProgrammingScreenWidget ? ProgrammingScreenWidget->GetCommands(SlotIndex) : TArray<EAWCommand>();

        FAWValidationResult Result = Sub->SubmitLocalCommands(SlotIndex, Commands);
        if (!Result.bSuccess)
        {
            SetStatus(FString::Printf(TEXT("Slot %d: %s"), SlotIndex, *Result.ErrorMessage), true);
        }
        else
        {
            SetStatus(FString::Printf(TEXT("Slot %d submitted."), SlotIndex));
        }
    }
}

void UAWHUDWidget::OnSubmitP1() { OnSubmitSlot(0); }
void UAWHUDWidget::OnSubmitP2() { OnSubmitSlot(1); }

void UAWHUDWidget::OnNextRound()
{
    if (UAWGameSubsystem *Sub = GetSubsystem())
    {
        Sub->NextRound();
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Replay Debugger
// ═══════════════════════════════════════════════════════════════════════════════

void UAWHUDWidget::InitializeReplayFromGameState()
{
    UWorld *World = GetWorld();
    if (!World)
        return;
    AAWGameState *GS = World->GetGameState<AAWGameState>();
    if (!GS)
        return;

    if (!InitializeReplay(GS->RevealedCommands0, GS->RevealedCommands1, GS->SimSeed))
    {
        SetStatus(TEXT("Failed to reconstruct simulation for replay."), true);
    }
}

bool UAWHUDWidget::InitializeReplay(const TArray<EAWCommand> &CommandsA, const TArray<EAWCommand> &CommandsB, int64 Seed)
{
    bReplayPlaying = false;
    ReplayAccumulator = 0.0;
    ReplaySpeed = 1.f;

    ReplayController = MakeUnique<Automata::FAWReplayController>();
    if (!ReplayController->Initialize(CommandsA, CommandsB, static_cast<uint64_t>(Seed)))
        return false;

    if (AAWArenaRenderer *Renderer = FindOrSpawnRenderer())
    {
        TArray<Automata::CellType> Grid;
        Grid.Reserve(ReplayController->GetGrid().size());
        for (auto c : ReplayController->GetGrid())
            Grid.Add(c);
        Renderer->InitializeArena(ReplayController->GetConfig(), Grid);
    }

    UpdateReplayUI();
    UpdateArenaFromReplay();
    return true;
}

void UAWHUDWidget::UpdateReplayUI()
{
    if (!ReplayController.IsValid() || !ReplayController->IsValid())
        return;

    int32 Step = ReplayController->GetCurrentStep();
    int32 Total = ReplayController->GetTotalSteps();

    if (ReplayAutopsyScreenWidget)
        ReplayAutopsyScreenWidget->SetTimeline(ReplaySpeed,
                                               Total > 1 ? static_cast<float>(Step) / static_cast<float>(Total - 1) : 0.f);

    const auto &Snap = ReplayController->GetCurrentSnapshot();
    auto FormatDetails = [&](int32 Idx) -> FString
    {
        const auto &R = Snap.robots[Idx];
        return FString::Printf(TEXT("HP %d  |  FACING %s"), R.hp, FacingName(R.facing));
    };
    if (ReplayAutopsyScreenWidget)
    {
        ReplayAutopsyScreenWidget->SetCombatantData(
            0, ReplayController->GetCommandsA(), Snap.robots[0].currentCommand, FormatDetails(0));
        ReplayAutopsyScreenWidget->SetCombatantData(
            1, ReplayController->GetCommandsB(), Snap.robots[1].currentCommand, FormatDetails(1));

        auto Events = ReplayController->GetEventsInRange(FMath::Max(0, Step - 8), Step);
        FString Log;
        for (const auto &E : Events)
        {
            Log += FString::Printf(TEXT("[STEP %d P%d] %s (%d,%d)\n"), E.step + 1, E.robot + 1, EventName(E.type), E.paramA, E.paramB);
        }
        ReplayAutopsyScreenWidget->SetEventLog(Log);
    }
}

void UAWHUDWidget::UpdateArenaFromReplay()
{
    if (!ReplayController.IsValid() || !ReplayController->IsValid())
        return;

    AAWArenaRenderer *Renderer = FindOrSpawnRenderer();
    if (!Renderer)
        return;

    int32 Step = ReplayController->GetCurrentStep();
    Renderer->SetSnapshot(ReplayController->GetCurrentSnapshot());

    auto Events = ReplayController->GetEventsForStep(Step);
    TArray<Automata::SimEvent> UEEvents;
    for (const auto &E : Events)
        UEEvents.Add(E);
    Renderer->ProcessEvents(UEEvents, Step, Step);
}

AAWArenaRenderer *UAWHUDWidget::FindOrSpawnRenderer()
{
    UWorld *World = GetWorld();
    if (!World)
        return nullptr;

    for (TActorIterator<AAWArenaRenderer> It(World); It; ++It)
    {
        return *It;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    return World->SpawnActor<AAWArenaRenderer>(AAWArenaRenderer::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
}

void UAWHUDWidget::OnReplayPlay()
{
    bReplayPlaying = true;
    ReplayAccumulator = 0.0;
}
void UAWHUDWidget::OnReplayPause() { bReplayPlaying = false; }

void UAWHUDWidget::OnReplayStep()
{
    PlayUISound(AWUIAssets::SFX_UINavigate);
    if (ReplayController.IsValid() && ReplayController->IsValid())
    {
        ReplayController->StepForward();
        UpdateReplayUI();
        UpdateArenaFromReplay();
    }
}

void UAWHUDWidget::OnReplayStepBack()
{
    PlayUISound(AWUIAssets::SFX_UINavigate);
    if (ReplayController.IsValid() && ReplayController->IsValid())
    {
        ReplayController->StepBackward();
        UpdateReplayUI();
        UpdateArenaFromReplay();
    }
}

void UAWHUDWidget::OnReplaySetSpeed(float Speed)
{
    ReplaySpeed = Speed;
    if (ReplayAutopsyScreenWidget)
        ReplayAutopsyScreenWidget->SetSpeed(Speed);
}

void UAWHUDWidget::OnReplayScrub(int32 Step)
{
    if (ReplayController.IsValid() && ReplayController->IsValid())
    {
        ReplayController->SeekToStep(Step);
        UpdateReplayUI();
        UpdateArenaFromReplay();
    }
}

void UAWHUDWidget::OnReplayScrubStart() { OnReplayScrub(0); }
void UAWHUDWidget::OnReplaySpeedQuarter() { OnReplaySetSpeed(0.25f); }
void UAWHUDWidget::OnReplaySpeedNormal() { OnReplaySetSpeed(1.f); }
void UAWHUDWidget::OnReplaySpeedDouble() { OnReplaySetSpeed(2.f); }
void UAWHUDWidget::OnReplaySpeedQuadruple() { OnReplaySetSpeed(4.f); }

void UAWHUDWidget::OnReplayScrubChanged(float Value)
{
    if (!ReplayController.IsValid() || !ReplayController->IsValid())
        return;

    const int32 Step = FMath::RoundToInt32(Value * (ReplayController->GetTotalSteps() - 1));
    OnReplayScrub(Step);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Replay Browser Actions
// ═══════════════════════════════════════════════════════════════════════════════

void UAWHUDWidget::RefreshReplayList()
{
    if (!ReplayBrowserScreenWidget)
        return;

    ReplayBrowserScreenWidget->ResetReplays();
    ReplayFilenames.Reset();

    UAWGameSubsystem *Sub = GetSubsystem();
    if (!Sub)
        return;

    TArray<FAWReplayInfo> Replays = Sub->GetReplayList();
    for (const FAWReplayInfo &Info : Replays)
    {
        FString Label = FString::Printf(TEXT("%s (%d bytes)"), *Info.Filename, Info.FileSizeBytes);
        ReplayBrowserScreenWidget->AddReplay(Label);
        ReplayFilenames.Add(Info.Filename);
    }

    if (Replays.IsEmpty())
        ReplayBrowserScreenWidget->SetStatus(TEXT("No replays found in Saved/Replays/"));
    else
        ReplayBrowserScreenWidget->SelectFirstReplay();
}

void UAWHUDWidget::OnReplayLoad(const FString &Filename)
{
    UAWGameSubsystem *Sub = GetSubsystem();
    if (!Sub)
        return;

    TArray<EAWCommand> Commands0, Commands1;
    FString Error;
    int64 Seed;
    if (!Sub->LoadReplay(Filename, Commands0, Commands1, Seed, Error))
    {
        if (ReplayBrowserScreenWidget)
            ReplayBrowserScreenWidget->SetStatus(Error);
        return;
    }

    if (!InitializeReplay(Commands0, Commands1, Seed))
    {
        if (ReplayBrowserScreenWidget)
            ReplayBrowserScreenWidget->SetStatus(TEXT("Failed to resimulate replay."));
        return;
    }
    ShowScreen(EAWScreen::ReplayAutopsy);
}

void UAWHUDWidget::OnReplaySave()
{
    UAWGameSubsystem *Sub = GetSubsystem();
    if (!Sub)
        return;

    FString Filename = FString::Printf(TEXT("replay_%s"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
    if (Sub->SaveReplay(Filename))
    {
        if (ReplayBrowserScreenWidget)
            ReplayBrowserScreenWidget->SetStatus(FString::Printf(TEXT("Saved: %s"), *Filename));
        RefreshReplayList();
    }
    else
    {
        if (ReplayBrowserScreenWidget)
            ReplayBrowserScreenWidget->SetStatus(TEXT("Save failed (not in ReplayAutopsy phase?)."));
    }
}

void UAWHUDWidget::OnReplayExport(const FString &Filename)
{
    UAWGameSubsystem *Sub = GetSubsystem();
    if (!Sub)
        return;

    FString Base64;
    if (Sub->ExportReplayBase64(Filename, Base64))
    {
        if (ReplayBrowserScreenWidget)
        {
            ReplayBrowserScreenWidget->SetExportData(Base64);
            ReplayBrowserScreenWidget->SetStatus(TEXT("Exported to field above."));
        }
    }
    else
    {
        if (ReplayBrowserScreenWidget)
            ReplayBrowserScreenWidget->SetStatus(TEXT("Export failed."));
    }
}

void UAWHUDWidget::OnReplayImport()
{
    const FString Base64 = ReplayBrowserScreenWidget ? ReplayBrowserScreenWidget->GetImportData() : FString();
    if (Base64.IsEmpty())
    {
        if (ReplayBrowserScreenWidget)
            ReplayBrowserScreenWidget->SetStatus(TEXT("Paste base64 data first."));
        return;
    }

    UAWGameSubsystem *Sub = GetSubsystem();
    if (!Sub)
        return;

    FString Filename = FString::Printf(TEXT("imported_%s"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
    FString Error;
    if (Sub->ImportReplayBase64(Base64, Filename, Error))
    {
        if (ReplayBrowserScreenWidget)
            ReplayBrowserScreenWidget->SetStatus(FString::Printf(TEXT("Imported: %s"), *Filename));
        RefreshReplayList();
    }
    else
    {
        if (ReplayBrowserScreenWidget)
            ReplayBrowserScreenWidget->SetStatus(Error);
    }
}

void UAWHUDWidget::OnReplayLoadSelected()
{
    const int32 ReplayIndex = ReplayBrowserScreenWidget ? ReplayBrowserScreenWidget->GetSelectedReplayIndex() : INDEX_NONE;
    if (!ReplayFilenames.IsValidIndex(ReplayIndex))
    {
        if (ReplayBrowserScreenWidget)
            ReplayBrowserScreenWidget->SetStatus(TEXT("Select a replay first."));
        return;
    }
    OnReplayLoad(ReplayFilenames[ReplayIndex]);
}

void UAWHUDWidget::OnReplayExportSelected()
{
    const int32 ReplayIndex = ReplayBrowserScreenWidget ? ReplayBrowserScreenWidget->GetSelectedReplayIndex() : INDEX_NONE;
    if (!ReplayFilenames.IsValidIndex(ReplayIndex))
    {
        if (ReplayBrowserScreenWidget)
            ReplayBrowserScreenWidget->SetStatus(TEXT("Select a replay first."));
        return;
    }
    OnReplayExport(ReplayFilenames[ReplayIndex]);
}

void UAWHUDWidget::OnReplayRefresh() { RefreshReplayList(); }

void UAWHUDWidget::PopulateLanguageReference()
{
    if (!LanguageReferenceScreenWidget)
        return;

    const FString Reference = TEXT(
        "AVAILABLE COMMANDS\n\n"
        "MOVE        Move one cell in the direction the tank is facing.\n\n"
        "FIRE        Fire in a straight line from the tank's cannon.\n\n"
        "TURN LEFT   Rotate 90 degrees left from the tank's point of view.\n\n"
        "TURN RIGHT  Rotate 90 degrees right from the tank's point of view.\n");
    LanguageReferenceScreenWidget->SetReference(Reference);
}

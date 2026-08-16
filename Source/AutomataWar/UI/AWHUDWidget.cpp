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
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/GameStateBase.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UnrealClient.h"

namespace
{
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
        MainMenuScreenWidget, DifficultyScreenWidget, ProgrammingScreenWidget, ReplayAutopsyScreenWidget,
        ReplayBrowserScreenWidget, LanguageReferenceScreenWidget};
    for (UAWScreenWidget *Screen : Screens)
    {
        if (Screen)
            Screen->OnAction.AddUObject(this, &UAWHUDWidget::OnScreenAction);
    }

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
        float ScreenshotDelay = 1.f;
        FParse::Value(FCommandLine::Get(), TEXT("AutomataCaptureDelay="), ScreenshotDelay);
        if (CaptureMode.Equals(TEXT("Programming"), ESearchCase::IgnoreCase))
        {
            OnLocalMatch();
        }
        else if (CaptureMode.Equals(TEXT("Difficulty"), ESearchCase::IgnoreCase))
        {
            OnSinglePlayerNav();
        }
        else if (CaptureMode.Equals(TEXT("SinglePlayerReplay"), ESearchCase::IgnoreCase))
        {
            OnStartSinglePlayer(EAWAIDifficulty::Hard);
            if (UAWGameSubsystem *Sub = GetSubsystem())
                Sub->SubmitLocalCommands(0, {EAWCommand::Wait});
        }
        else if (CaptureMode.Equals(TEXT("ProgrammingSubmitted"), ESearchCase::IgnoreCase) ||
                 CaptureMode.Equals(TEXT("ProgrammingReturned"), ESearchCase::IgnoreCase))
        {
            OnLocalMatch();
            if (UUserWidget *Panel = Cast<UUserWidget>(ProgrammingScreenWidget->GetWidgetFromName(TEXT("ProgrammingP1PanelWidget"))))
            {
                if (UButton *MoveButton = Cast<UButton>(Panel->GetWidgetFromName(TEXT("ProgrammingMoveButton"))))
                    MoveButton->OnClicked.Broadcast();
                if (UButton *SubmitButton = Cast<UButton>(Panel->GetWidgetFromName(TEXT("ProgrammingSubmitButton"))))
                    SubmitButton->OnClicked.Broadcast();

                if (CaptureMode.Equals(TEXT("ProgrammingReturned"), ESearchCase::IgnoreCase))
                {
                    ScreenshotDelay = 1.4f;
                    TWeakObjectPtr<UUserWidget> WeakPanel = Panel;
                    FTimerHandle ReturnTimer;
                    GetWorld()->GetTimerManager().SetTimer(
                        ReturnTimer,
                        FTimerDelegate::CreateWeakLambda(this, [WeakPanel]()
                                                         {
                                                             if (UUserWidget *ProgrammingPanel = WeakPanel.Get())
                                                                 if (UButton *ReturnButton = Cast<UButton>(ProgrammingPanel->GetWidgetFromName(TEXT("ProgrammingReturnToPlanningButton"))))
                                                                     ReturnButton->OnClicked.Broadcast(); }),
                        0.7f, false);
                }
            }
        }
        else if (CaptureMode.Equals(TEXT("Replay"), ESearchCase::IgnoreCase))
        {
            OnLocalMatch();
            if (UAWGameSubsystem *Sub = GetSubsystem())
            {
                Sub->SubmitLocalCommands(0, {EAWCommand::Fire, EAWCommand::Move, EAWCommand::Fire});
                Sub->SubmitLocalCommands(1, {EAWCommand::TurnRight, EAWCommand::Move, EAWCommand::Fire});
            }
        }
        else if (CaptureMode.Equals(TEXT("MuzzleVFX"), ESearchCase::IgnoreCase) ||
                 CaptureMode.Equals(TEXT("ImpactVFX"), ESearchCase::IgnoreCase))
        {
            OnLocalMatch();
            if (UAWGameSubsystem *Sub = GetSubsystem())
            {
                Sub->SubmitLocalCommands(0, {EAWCommand::Fire});
                Sub->SubmitLocalCommands(1, {EAWCommand::Fire});
            }
        }

        if (FParse::Param(FCommandLine::Get(), TEXT("AutomataCaptureScreenshot")))
        {
            const FString ScreenshotPath = FPaths::ProjectSavedDir() / TEXT("Screenshots") /
                                           FString::Printf(TEXT("HUD_%s.png"), *CaptureMode);
            FTimerHandle ScreenshotTimer;
            GetWorld()->GetTimerManager().SetTimer(
                ScreenshotTimer,
                FTimerDelegate::CreateLambda([ScreenshotPath]()
                                             { FScreenshotRequest::RequestScreenshot(ScreenshotPath, true, false); }),
                ScreenshotDelay, false);
            if (FParse::Param(FCommandLine::Get(), TEXT("AutomataCaptureExit")))
            {
                FTimerHandle ExitTimer;
                GetWorld()->GetTimerManager().SetTimer(
                    ExitTimer,
                    FTimerDelegate::CreateLambda([]()
                                                 { FPlatformMisc::RequestExit(false); }),
                    ScreenshotDelay + 0.75f, false);
            }
        }
    }
#endif
}

void UAWHUDWidget::NativeDestruct()
{
    UAWScreenWidget *Screens[] = {
        MainMenuScreenWidget, DifficultyScreenWidget, ProgrammingScreenWidget, ReplayAutopsyScreenWidget,
        ReplayBrowserScreenWidget, LanguageReferenceScreenWidget};
    for (UAWScreenWidget *Screen : Screens)
    {
        if (Screen)
            Screen->OnAction.RemoveAll(this);
    }

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

void UAWHUDWidget::SetArenaRenderTarget(UTextureRenderTarget2D *RenderTarget)
{
    if (SimulationScreenWidget)
        SimulationScreenWidget->SetArenaRenderTarget(RenderTarget);
    if (ReplayAutopsyScreenWidget)
        ReplayAutopsyScreenWidget->SetArenaRenderTarget(RenderTarget);
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
    case EAWUIAction::SinglePlayer:
        OnSinglePlayerNav();
        break;
    case EAWUIAction::DifficultyEasy:
        OnStartSinglePlayer(EAWAIDifficulty::Easy);
        break;
    case EAWUIAction::DifficultyNormal:
        OnStartSinglePlayer(EAWAIDifficulty::Normal);
        break;
    case EAWUIAction::DifficultyHard:
        OnStartSinglePlayer(EAWAIDifficulty::Hard);
        break;
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
    case EAWUIAction::ReturnToPlanningP1:
        OnReturnToPlanningSlot(0);
        break;
    case EAWUIAction::ReturnToPlanningP2:
        OnReturnToPlanningSlot(1);
        break;
    case EAWUIAction::ReplayStart:
        OnReplayStart();
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
        if (ProgrammingScreenWidget)
        {
            if (AAWGameState *GS = GetWorld()->GetGameState<AAWGameState>())
            {
                ProgrammingScreenWidget->ResetForNewRound(GS->GetActionPoints(0), GS->GetActionPoints(1));
                ProgrammingScreenWidget->SetSinglePlayerMode(bSinglePlayer);
            }
            else
                ProgrammingScreenWidget->ResetSubmissionState();
        }
        ShowScreen(EAWScreen::Programming);
        break;
    case EAWMatchPhase::Simulation:
        if (SimulationScreenWidget && ProgrammingScreenWidget)
        {
            SimulationScreenWidget->SetCommands(ProgrammingScreenWidget->GetCommands(0), ProgrammingScreenWidget->GetCommands(1));
            if (AAWGameState *GS = GetWorld()->GetGameState<AAWGameState>())
            {
                SimulationScreenWidget->SetPlayerDetails(0, FString::Printf(
                                                                TEXT("HP %d  |  AP %d  |  FACING SOUTH"), Automata::MaxHP, GS->GetActionPoints(0)));
                SimulationScreenWidget->SetPlayerDetails(1, FString::Printf(
                                                                TEXT("HP %d  |  AP %d  |  FACING NORTH"), Automata::MaxHP, GS->GetActionPoints(1)));
            }
        }
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
    bSinglePlayer = false;
    if (UAWGameSubsystem *Sub = GetSubsystem())
    {
        Sub->StartLocalMatch();
    }
    if (ProgrammingScreenWidget)
    {
        ProgrammingScreenWidget->ResetForNewRound(Automata::InitialActionPoints, Automata::InitialActionPoints);
        ProgrammingScreenWidget->SetSinglePlayerMode(false);
    }
    ShowScreen(EAWScreen::Programming);
}

void UAWHUDWidget::OnSinglePlayerNav()
{
    ShowScreen(EAWScreen::Difficulty);
}

void UAWHUDWidget::OnStartSinglePlayer(EAWAIDifficulty Difficulty)
{
    bSinglePlayer = true;
    if (UAWGameSubsystem *Sub = GetSubsystem())
        Sub->StartSinglePlayerMatch(Difficulty);
    if (ProgrammingScreenWidget)
    {
        ProgrammingScreenWidget->ResetForNewRound(Automata::InitialActionPoints, Automata::InitialActionPoints);
        ProgrammingScreenWidget->SetSinglePlayerMode(true);
    }
    ShowScreen(EAWScreen::Programming);
}

void UAWHUDWidget::OnHostLAN()
{
    if (UAWGameSubsystem *Sub = GetSubsystem())
    {
        Sub->HostSession(TEXT("AutomataWar"));
    }
    SetStatus(TEXT("Hosting LAN session..."));
}

void UAWHUDWidget::OnFindLAN()
{
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
    FAWValidationResult Result;
    if (UAWGameSubsystem *Sub = GetSubsystem())
    {
        const TArray<EAWCommand> Commands = ProgrammingScreenWidget ? ProgrammingScreenWidget->GetCommands(SlotIndex) : TArray<EAWCommand>();
        Result = Sub->SubmitLocalCommands(SlotIndex, Commands);
    }
    else
    {
        Result.ErrorMessage = TEXT("Game subsystem unavailable.");
    }

    if (ProgrammingScreenWidget)
        ProgrammingScreenWidget->ResolveSubmission(SlotIndex, Result.bSuccess);
    if (!Result.bSuccess)
        SetStatus(FString::Printf(TEXT("Slot %d: %s"), SlotIndex, *Result.ErrorMessage), true);
    else
        SetStatus(FString::Printf(TEXT("Slot %d submitted."), SlotIndex));
}

void UAWHUDWidget::OnReturnToPlanningSlot(int32 SlotIndex)
{
    if (UAWGameSubsystem *Sub = GetSubsystem())
    {
        const FAWValidationResult Result = Sub->WithdrawLocalCommands(SlotIndex);
        if (!Result.bSuccess)
            SetStatus(FString::Printf(TEXT("Slot %d: %s"), SlotIndex, *Result.ErrorMessage), true);
        else
            SetStatus(FString::Printf(TEXT("Slot %d returned to planning."), SlotIndex));
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

    if (!InitializeReplay(GS->RevealedCommands0, GS->RevealedCommands1, GS->SimSeed,
                          GS->ReplayStartActionPoints0, GS->ReplayStartActionPoints1,
                          GS->ReplayStartEffects0, GS->ReplayStartEffects1, GS->RoundStartingSlot,
                          GS->ReplayStartArenaState))
    {
        SetStatus(TEXT("Failed to reconstruct simulation for replay."), true);
    }
}

bool UAWHUDWidget::InitializeReplay(const TArray<EAWCommand> &CommandsA, const TArray<EAWCommand> &CommandsB, int64 Seed,
                                    int32 ActionPointsA, int32 ActionPointsB,
                                    const FAWRobotEffects &EffectsA, const FAWRobotEffects &EffectsB,
                                    int32 StartingSlot, const TArray<uint8> &InitialState)
{
    bReplayPlaying = false;
    ReplayAccumulator = 0.0;
    LastProcessedReplayEventStep = INDEX_NONE;
    ReplaySpeed = .1f;

    ReplayController = MakeUnique<Automata::FAWReplayController>();
    if (!ReplayController->Initialize(CommandsA, CommandsB, static_cast<uint64_t>(Seed), ActionPointsA, ActionPointsB,
                                      EffectsA, EffectsB, StartingSlot, InitialState))
        return false;

    if (AAWArenaRenderer *Renderer = FindOrSpawnRenderer())
    {
        TArray<Automata::CellType> Grid;
        Grid.Reserve(ReplayController->GetGrid().size());
        for (auto c : ReplayController->GetGrid())
            Grid.Add(c);
        TArray<Automata::SimEvent> Events;
        for (const Automata::SimEvent &Event : ReplayController->GetEventsInRange(0, ReplayController->GetTotalSteps()))
            Events.Add(Event);
        Renderer->InitializeArena(ReplayController->GetConfig(), Grid, Events);
    }

    UpdateReplayUI();
    UpdateArenaFromReplay();
    return true;
}

void UAWHUDWidget::UpdateReplayUI()
{
    if (!ReplayController.IsValid() || !ReplayController->IsValid())
        return;

    if (ReplayAutopsyScreenWidget)
        ReplayAutopsyScreenWidget->SetSpeed(ReplaySpeed);

    const auto &Snap = ReplayController->GetCurrentSnapshot();
    auto FormatDetails = [&](int32 Idx) -> FString
    {
        const auto &R = Snap.robots[Idx];
        return FString::Printf(TEXT("HP %d  |  AP %d  |  FACING %s"), R.hp, R.actionPoints, FacingName(R.facing));
    };
    if (ReplayAutopsyScreenWidget)
    {
        ReplayAutopsyScreenWidget->SetCombatantData(
            0, ReplayController->GetCommandsA(), Snap.robots[0].currentCommand, FormatDetails(0));
        ReplayAutopsyScreenWidget->SetCombatantData(
            1, ReplayController->GetCommandsB(), Snap.robots[1].currentCommand, FormatDetails(1));
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

    if (Step == LastProcessedReplayEventStep)
        return;
    LastProcessedReplayEventStep = Step;

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
    if (ReplayController.IsValid() && ReplayController->IsValid())
    {
        ReplayController->StepForward();
        UpdateReplayUI();
        UpdateArenaFromReplay();
    }
}

void UAWHUDWidget::OnReplayStepBack()
{
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

void UAWHUDWidget::OnReplayStart()
{
    if (ReplayController.IsValid() && ReplayController->IsValid())
    {
        ReplayController->SeekToStep(0);
        UpdateReplayUI();
        UpdateArenaFromReplay();
    }
}
void UAWHUDWidget::OnReplaySpeedQuarter() { OnReplaySetSpeed(0.25f); }
void UAWHUDWidget::OnReplaySpeedNormal() { OnReplaySetSpeed(1.f); }
void UAWHUDWidget::OnReplaySpeedDouble() { OnReplaySetSpeed(2.f); }
void UAWHUDWidget::OnReplaySpeedQuadruple() { OnReplaySetSpeed(4.f); }

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
    int32 StartingSlot, ActionPoints0, ActionPoints1;
    FAWRobotEffects Effects0, Effects1;
    TArray<uint8> InitialState;
    if (!Sub->LoadReplay(Filename, Commands0, Commands1, Seed, StartingSlot,
                         ActionPoints0, ActionPoints1, Effects0, Effects1, InitialState, Error))
    {
        if (ReplayBrowserScreenWidget)
            ReplayBrowserScreenWidget->SetStatus(Error);
        return;
    }

    if (!InitializeReplay(Commands0, Commands1, Seed, ActionPoints0, ActionPoints1,
                          Effects0, Effects1, StartingSlot, InitialState))
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
        "MOVE        10 AP  |  Move one cell in the direction the tank is facing.\n\n"
        "FIRE        20 AP  |  Fire in a straight line from the tank's cannon.\n\n"
        "TURN LEFT    5 AP  |  Rotate 90 degrees left from the tank's point of view.\n\n"
        "TURN RIGHT   5 AP  |  Rotate 90 degrees right from the tank's point of view.\n\n"
        "WAIT         0 AP  |  Hold position for one command step.\n\n"
        "CHARGE SHIELD 20 AP  |  Reduce the next incoming hit by 50%.\n\n"
        "ACCELERATE  30 AP  |  Make the next move cross up to two cells.\n");
    LanguageReferenceScreenWidget->SetReference(Reference);
}

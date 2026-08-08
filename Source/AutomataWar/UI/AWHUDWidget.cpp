#include "AWHUDWidget.h"
#include "AWScreenWidget.h"
#include "AWUITypes.h"
#include "AutomataWar/Game/AWGameSubsystem.h"
#include "AutomataWar/Game/AWGameState.h"
#include "AutomataWar/Game/AWGameMode.h"
#include "AutomataWar/Game/AWExampleScripts.h"
#include "AutomataWar/Game/AWReplayService.h"
#include "AutomataWar/Game/AWPlayerController.h"
#include "AutomataWar/Core/AutomataRules.h"
#include "AutomataWar/Core/Lang/AutomataCompiler.h"
#include "AutomataWar/Core/Sim/AutomataSimulation.h"
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
    FString FormatReplaySource(const std::string &Source, const Automata::Program &Program, int32 CurrentInstruction)
    {
        int32 HighlightLine = -1;
        if (Program.sourceMap.size() > static_cast<size_t>(CurrentInstruction) && CurrentInstruction >= 0)
        {
            HighlightLine = Program.sourceMap[static_cast<size_t>(CurrentInstruction)].line;
        }

        TArray<FString> Lines;
        FString(UTF8_TO_TCHAR(Source.c_str())).ParseIntoArrayLines(Lines, false);
        FString Result;
        for (int32 Index = 0; Index < Lines.Num(); ++Index)
        {
            const bool bHighlighted = Index + 1 == HighlightLine;
            Result += FString::Printf(TEXT("%s %3d| %s\n"), bHighlighted ? TEXT(">>") : TEXT("  "), Index + 1, *Lines[Index]);
        }
        return Result;
    }

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
        case Automata::EventType::Scan:
            return TEXT("scanned");
        case Automata::EventType::Fire:
            return TEXT("fired");
        case Automata::EventType::ShieldActivate:
            return TEXT("shield activated");
        case Automata::EventType::ShieldAbsorb:
            return TEXT("shield absorbed hit");
        case Automata::EventType::Hit:
            return TEXT("hit");
        case Automata::EventType::ProjectileBlocked:
            return TEXT("projectile blocked");
        case Automata::EventType::Wait:
            return TEXT("waited");
        case Automata::EventType::Halt:
            return TEXT("halted");
        case Automata::EventType::EnergyDepleted:
            return TEXT("out of energy");
        default:
            return TEXT("unknown");
        }
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

    if (ProgrammingScreenWidget && ProgrammingScreenWidget->GetSource(0).IsEmpty())
        ProgrammingScreenWidget->SetSource(0, FAWExampleScripts::DefaultBot());
    if (ProgrammingScreenWidget && ProgrammingScreenWidget->GetSource(1).IsEmpty())
        ProgrammingScreenWidget->SetSource(1, FAWExampleScripts::DefaultBot());
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
                Sub->SubmitLocalScript(0, FAWExampleScripts::Aggressor());
                Sub->SubmitLocalScript(1, FAWExampleScripts::Camper());
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
        const double TickInterval = 1.0 / 10.0;
        while (ReplayAccumulator >= TickInterval)
        {
            ReplayAccumulator -= TickInterval;
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
    case EAWUIAction::AggressorP1:
        OnAggressorP1();
        break;
    case EAWUIAction::AggressorP2:
        OnAggressorP2();
        break;
    case EAWUIAction::CamperP1:
        OnCamperP1();
        break;
    case EAWUIAction::CamperP2:
        OnCamperP2();
        break;
    case EAWUIAction::KiterP1:
        OnKiterP1();
        break;
    case EAWUIAction::KiterP2:
        OnKiterP2();
        break;
    case EAWUIAction::TrainingP1:
        OnTrainingP1();
        break;
    case EAWUIAction::TrainingP2:
        OnTrainingP2();
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
        OnReplayStepTick();
        break;
    case EAWUIAction::ReplayStepP1:
        OnReplayStepP1();
        break;
    case EAWUIAction::ReplayStepP2:
        OnReplayStepP2();
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
            SimulationScreenWidget->SetSources(ProgrammingScreenWidget->GetSource(0), ProgrammingScreenWidget->GetSource(1));
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
        const FString Source = ProgrammingScreenWidget ? ProgrammingScreenWidget->GetSource(SlotIndex) : FString();

        FAWValidationResult Result = Sub->SubmitLocalScript(SlotIndex, Source);
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

void UAWHUDWidget::OnLoadExample(int32 SlotIndex, const FString &ScriptName)
{
    FString Source;
    if (ScriptName == TEXT("Aggressor"))
        Source = FAWExampleScripts::Aggressor();
    else if (ScriptName == TEXT("Camper"))
        Source = FAWExampleScripts::Camper();
    else if (ScriptName == TEXT("Kiter"))
        Source = FAWExampleScripts::Kiter();
    else
        Source = FAWExampleScripts::DefaultBot();

    if (ProgrammingScreenWidget)
        ProgrammingScreenWidget->SetSource(SlotIndex, Source);
}

void UAWHUDWidget::OnTrainingBot(int32 SlotIndex)
{
    if (UAWGameSubsystem *Sub = GetSubsystem())
    {
        const FString Source = ProgrammingScreenWidget ? ProgrammingScreenWidget->GetSource(SlotIndex) : FString();

        int32 Wins, Losses, Draws;
        Sub->RunTraining(Source, 10, Wins, Losses, Draws);
        SetStatus(FString::Printf(TEXT("Training: W%d L%d D%d"), Wins, Losses, Draws));
    }
}

void UAWHUDWidget::OnSubmitP1() { OnSubmitSlot(0); }
void UAWHUDWidget::OnSubmitP2() { OnSubmitSlot(1); }
void UAWHUDWidget::OnAggressorP1() { OnLoadExample(0, TEXT("Aggressor")); }
void UAWHUDWidget::OnAggressorP2() { OnLoadExample(1, TEXT("Aggressor")); }
void UAWHUDWidget::OnCamperP1() { OnLoadExample(0, TEXT("Camper")); }
void UAWHUDWidget::OnCamperP2() { OnLoadExample(1, TEXT("Camper")); }
void UAWHUDWidget::OnKiterP1() { OnLoadExample(0, TEXT("Kiter")); }
void UAWHUDWidget::OnKiterP2() { OnLoadExample(1, TEXT("Kiter")); }
void UAWHUDWidget::OnTrainingP1() { OnTrainingBot(0); }
void UAWHUDWidget::OnTrainingP2() { OnTrainingBot(1); }

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

    if (!InitializeReplay(GS->RevealedSource0, GS->RevealedSource1, GS->SimSeed))
    {
        SetStatus(TEXT("Failed to reconstruct simulation for replay."), true);
    }
}

bool UAWHUDWidget::InitializeReplay(const FString &SourceA, const FString &SourceB, int64 Seed)
{
    bReplayPlaying = false;
    ReplayAccumulator = 0.0;
    ReplaySpeed = 1.f;

    ReplayController = MakeUnique<Automata::FAWReplayController>();
    if (!ReplayController->Initialize(TCHAR_TO_UTF8(*SourceA), TCHAR_TO_UTF8(*SourceB), static_cast<uint64_t>(Seed)))
        return false;

    if (AAWArenaRenderer *Renderer = FindOrSpawnRenderer())
    {
        TArray<Automata::CellType> Grid;
        Grid.Reserve(ReplayController->GetGrid().size());
        for (auto c : ReplayController->GetGrid())
            Grid.Add(c);
        Renderer->InitializeArena(ReplayController->GetConfig(), Grid);
    }

    if (ReplayAutopsyScreenWidget)
    {
        const auto &R = ReplayController->GetResult();
        FString Outcome;
        switch (R.outcome)
        {
        case Automata::MatchOutcome::Robot0Wins:
            Outcome = TEXT("P1 WINS");
            break;
        case Automata::MatchOutcome::Robot1Wins:
            Outcome = TEXT("P2 WINS");
            break;
        default:
            Outcome = TEXT("DRAW");
            break;
        }
        Outcome += FString::Printf(TEXT(" | Tick %d | HP: %d/%d"), R.finalTick, R.finalHP[0], R.finalHP[1]);
        ReplayAutopsyScreenWidget->SetOutcome(Outcome);
    }

    UpdateReplayUI();
    UpdateArenaFromReplay();
    return true;
}

void UAWHUDWidget::UpdateReplayUI()
{
    if (!ReplayController.IsValid() || !ReplayController->IsValid())
        return;

    int32 Tick = ReplayController->GetCurrentTick();
    int32 Total = ReplayController->GetTotalTicks();

    if (ReplayAutopsyScreenWidget)
        ReplayAutopsyScreenWidget->SetTimeline(Tick, Total, ReplaySpeed,
                                               Total > 1 ? static_cast<float>(Tick) / static_cast<float>(Total - 1) : 0.f);

    const auto &Snap = ReplayController->GetCurrentSnapshot();
    auto FormatRegs = [&](int32 Idx) -> FString
    {
        const auto &R = Snap.robots[Idx];
        return FString::Printf(TEXT("PC:%d  LINE:%d  BUSY:%d  SHIELD:%s  EXEC:%d\nR0:%d  R1:%d  R2:%d  R3:%d\nR_HP:%d  R_ENEMY_DIST:%d  R_ENEMY_DIR:%d  R_ENERGY:%d  R_TICK:%d"),
                               R.vm.pc, R.vm.currentInstruction, R.vm.busyLeft, R.shielded ? TEXT("Y") : TEXT("N"), R.vm.instrExecCount,
                               R.vm.regs[0], R.vm.regs[1], R.vm.regs[2], R.vm.regs[3],
                               R.vm.regs[static_cast<int32>(Automata::Reg::R_HP)], R.vm.regs[static_cast<int32>(Automata::Reg::R_ENEMY_DIST)],
                               R.vm.regs[static_cast<int32>(Automata::Reg::R_ENEMY_DIR)], R.vm.regs[static_cast<int32>(Automata::Reg::R_ENERGY)],
                               R.vm.regs[static_cast<int32>(Automata::Reg::R_TICK)]);
    };
    if (ReplayAutopsyScreenWidget)
    {
        ReplayAutopsyScreenWidget->SetCombatantData(
            0,
            FormatReplaySource(ReplayController->GetSourceA(), ReplayController->GetProgramA(), Snap.robots[0].vm.currentInstruction),
            FormatRegs(0));
        ReplayAutopsyScreenWidget->SetCombatantData(
            1,
            FormatReplaySource(ReplayController->GetSourceB(), ReplayController->GetProgramB(), Snap.robots[1].vm.currentInstruction),
            FormatRegs(1));

        auto Events = ReplayController->GetEventsInRange(FMath::Max(0, Tick - 8), Tick);
        FString Log;
        for (const auto &E : Events)
        {
            Log += FString::Printf(TEXT("[T%d P%d] %s (%d,%d)\n"), E.tick, E.robot + 1, EventName(E.type), E.paramA, E.paramB);
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

    int32 Tick = ReplayController->GetCurrentTick();
    Renderer->SetSnapshot(ReplayController->GetCurrentSnapshot());

    auto Events = ReplayController->GetEventsForTick(Tick);
    TArray<Automata::SimEvent> UEEvents;
    for (const auto &E : Events)
        UEEvents.Add(E);
    Renderer->ProcessEvents(UEEvents, Tick, Tick);
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

void UAWHUDWidget::OnReplayStepTick()
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

void UAWHUDWidget::OnReplayStepInstruction(int32 RobotIndex)
{
    PlayUISound(AWUIAssets::SFX_UINavigate);
    if (ReplayController.IsValid() && ReplayController->IsValid())
    {
        ReplayController->StepInstruction(RobotIndex);
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

void UAWHUDWidget::OnReplayScrub(int32 Tick)
{
    if (ReplayController.IsValid() && ReplayController->IsValid())
    {
        ReplayController->SeekToTick(Tick);
        UpdateReplayUI();
        UpdateArenaFromReplay();
    }
}

void UAWHUDWidget::OnReplayScrubStart() { OnReplayScrub(0); }
void UAWHUDWidget::OnReplayStepP1() { OnReplayStepInstruction(0); }
void UAWHUDWidget::OnReplayStepP2() { OnReplayStepInstruction(1); }
void UAWHUDWidget::OnReplaySpeedQuarter() { OnReplaySetSpeed(0.25f); }
void UAWHUDWidget::OnReplaySpeedNormal() { OnReplaySetSpeed(1.f); }
void UAWHUDWidget::OnReplaySpeedDouble() { OnReplaySetSpeed(2.f); }
void UAWHUDWidget::OnReplaySpeedQuadruple() { OnReplaySetSpeed(4.f); }

void UAWHUDWidget::OnReplayScrubChanged(float Value)
{
    if (!ReplayController.IsValid() || !ReplayController->IsValid())
        return;

    const int32 Tick = FMath::RoundToInt32(Value * (ReplayController->GetTotalTicks() - 1));
    OnReplayScrub(Tick);
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

    FString Src0, Src1, Error;
    int64 Seed;
    if (!Sub->LoadReplay(Filename, Src0, Src1, Seed, Error))
    {
        if (ReplayBrowserScreenWidget)
            ReplayBrowserScreenWidget->SetStatus(Error);
        return;
    }

    if (!InitializeReplay(Src0, Src1, Seed))
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

    FString Reference = FString::Printf(TEXT("INSTRUCTIONS (%d)\n\n"), Automata::OpcodeCount);
    for (const Automata::InstructionDefinition &Definition : Automata::InstructionDefs)
    {
        Reference += FString::Printf(TEXT("%-28s  ENERGY %2d  TICKS %2d\n%s\n\n"),
                                     UTF8_TO_TCHAR(Definition.syntax), Definition.energyCost, Definition.tickCost,
                                     UTF8_TO_TCHAR(Definition.description));
    }

    Reference += FString::Printf(TEXT("\nREGISTERS (%d)\n\n"), Automata::TotalRegisterCount);
    for (const Automata::RegisterDefinition &Definition : Automata::RegisterDefs)
    {
        Reference += FString::Printf(TEXT("%-14s  %s%s\n"), UTF8_TO_TCHAR(Definition.name),
                                     UTF8_TO_TCHAR(Definition.description), Definition.readOnly ? TEXT("  [READ ONLY]") : TEXT(""));
    }
    LanguageReferenceScreenWidget->SetReference(Reference);
}

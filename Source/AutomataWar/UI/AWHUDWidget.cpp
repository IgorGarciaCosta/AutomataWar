#include "AWHUDWidget.h"
#include "AWCodeEditorWidget.h"
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
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/Slider.h"
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

#define BIND_BUTTON(Name, Handler)                                        \
    if (UButton *Button = Cast<UButton>(GetWidgetFromName(TEXT(Name))))   \
    {                                                                     \
        Button->OnClicked.AddUniqueDynamic(this, &UAWHUDWidget::Handler); \
    }

    BIND_BUTTON("LocalMatchButton", OnLocalMatch);
    BIND_BUTTON("HostLanButton", OnHostLAN);
    BIND_BUTTON("FindLanButton", OnFindLAN);
    BIND_BUTTON("JoinSessionButton", OnJoinSelectedSession);
    BIND_BUTTON("JoinIpButton", OnJoinIP);
    BIND_BUTTON("ReplayBrowserButton", OnReplayBrowserNav);
    BIND_BUTTON("LanguageReferenceButton", OnLanguageRef);
    BIND_BUTTON("QuitButton", OnQuit);
    BIND_BUTTON("SubmitP1Button", OnSubmitP1);
    BIND_BUTTON("SubmitP2Button", OnSubmitP2);
    BIND_BUTTON("AggressorP1Button", OnAggressorP1);
    BIND_BUTTON("AggressorP2Button", OnAggressorP2);
    BIND_BUTTON("CamperP1Button", OnCamperP1);
    BIND_BUTTON("CamperP2Button", OnCamperP2);
    BIND_BUTTON("KiterP1Button", OnKiterP1);
    BIND_BUTTON("KiterP2Button", OnKiterP2);
    BIND_BUTTON("TrainingP1Button", OnTrainingP1);
    BIND_BUTTON("TrainingP2Button", OnTrainingP2);
    BIND_BUTTON("ProgrammingBackButton", OnBackToMainMenu);
    BIND_BUTTON("ReplayStartButton", OnReplayScrubStart);
    BIND_BUTTON("ReplayBackButton", OnReplayStepBack);
    BIND_BUTTON("ReplayPauseButton", OnReplayPause);
    BIND_BUTTON("ReplayPlayButton", OnReplayPlay);
    BIND_BUTTON("ReplayStepButton", OnReplayStepTick);
    BIND_BUTTON("ReplayStepP1Button", OnReplayStepP1);
    BIND_BUTTON("ReplayStepP2Button", OnReplayStepP2);
    BIND_BUTTON("ReplayQuarterButton", OnReplaySpeedQuarter);
    BIND_BUTTON("ReplayNormalButton", OnReplaySpeedNormal);
    BIND_BUTTON("ReplayDoubleButton", OnReplaySpeedDouble);
    BIND_BUTTON("ReplayQuadButton", OnReplaySpeedQuadruple);
    BIND_BUTTON("NextRoundButton", OnNextRound);
    BIND_BUTTON("ReplayBackToMenuButton", OnBackToMainMenu);
    BIND_BUTTON("ReplayBrowserBackButton", OnBackToMainMenu);
    BIND_BUTTON("ReplayRefreshButton", OnReplayRefresh);
    BIND_BUTTON("ReplaySaveButton", OnReplaySave);
    BIND_BUTTON("ReplayLoadButton", OnReplayLoadSelected);
    BIND_BUTTON("ReplayExportButton", OnReplayExportSelected);
    BIND_BUTTON("ReplayImportButton", OnReplayImport);
    BIND_BUTTON("LanguageBackButton", OnBackToMainMenu);

#undef BIND_BUTTON

    if (ReplayScrubSlider)
        ReplayScrubSlider->OnValueChanged.AddUniqueDynamic(this, &UAWHUDWidget::OnReplayScrubChanged);

    if (EditorP1 && EditorP1->GetSourceText().IsEmpty())
        EditorP1->SetSourceText(FAWExampleScripts::DefaultBot());
    if (EditorP2 && EditorP2->GetSourceText().IsEmpty())
        EditorP2->SetSourceText(FAWExampleScripts::DefaultBot());
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

void UAWHUDWidget::OnPhaseChanged(EAWMatchPhase NewPhase)
{
    switch (NewPhase)
    {
    case EAWMatchPhase::Programming:
        ShowScreen(EAWScreen::Programming);
        break;
    case EAWMatchPhase::Simulation:
        if (UTextBlock *SourceP1 = Cast<UTextBlock>(GetWidgetFromName(TEXT("SimulationSourceP1Text"))))
            SourceP1->SetText(FText::FromString(EditorP1 ? EditorP1->GetSourceText() : FString()));
        if (UTextBlock *SourceP2 = Cast<UTextBlock>(GetWidgetFromName(TEXT("SimulationSourceP2Text"))))
            SourceP2->SetText(FText::FromString(EditorP2 ? EditorP2->GetSourceText() : FString()));
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
    SessionComboBox->ClearOptions();
    UAWGameSubsystem *Sub = GetSubsystem();
    if (!Sub)
        return;

    const TArray<FAWSessionInfo> Sessions = Sub->GetSessionList();
    for (const FAWSessionInfo &Session : Sessions)
    {
        const FString Label = FString::Printf(TEXT("%s | %s | %d/%d | %d ms"), *Session.SessionName,
                                              *Session.HostName, Session.CurrentPlayers, Session.MaxPlayers, Session.PingMs);
        SessionComboBox->AddOption(Label);
    }
    if (Sessions.IsEmpty())
        SetStatus(TEXT("No LAN sessions found."), true);
    else
        SessionComboBox->SetSelectedIndex(0);
}

void UAWHUDWidget::OnJoinSelectedSession()
{
    const int32 SessionIndex = SessionComboBox->GetSelectedIndex();
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
    FString IP = JoinIPField->GetText().ToString().TrimStartAndEnd();
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
        FString Source;
        if (SlotIndex == 0 && EditorP1)
            Source = EditorP1->GetSourceText();
        else if (SlotIndex == 1 && EditorP2)
            Source = EditorP2->GetSourceText();

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

    if (SlotIndex == 0 && EditorP1)
        EditorP1->SetSourceText(Source);
    else if (SlotIndex == 1 && EditorP2)
        EditorP2->SetSourceText(Source);
}

void UAWHUDWidget::OnTrainingBot(int32 SlotIndex)
{
    if (UAWGameSubsystem *Sub = GetSubsystem())
    {
        FString Source;
        if (SlotIndex == 0 && EditorP1)
            Source = EditorP1->GetSourceText();
        else if (SlotIndex == 1 && EditorP2)
            Source = EditorP2->GetSourceText();

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

    if (ReplayOutcomeText)
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
        ReplayOutcomeText->SetText(FText::FromString(Outcome));
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

    if (ReplayTickText)
    {
        ReplayTickText->SetText(FText::FromString(FString::Printf(TEXT("Tick: %d/%d"), Tick, Total - 1)));
    }
    if (ReplaySpeedText)
    {
        ReplaySpeedText->SetText(FText::FromString(FString::Printf(TEXT("Speed: %.2fx"), ReplaySpeed)));
    }
    if (ReplayScrubSlider && Total > 1)
    {
        bUpdatingReplaySlider = true;
        ReplayScrubSlider->SetValue(static_cast<float>(Tick) / static_cast<float>(Total - 1));
        bUpdatingReplaySlider = false;
    }

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
    if (ReplayRegistersP1)
        ReplayRegistersP1->SetText(FText::FromString(FormatRegs(0)));
    if (ReplayRegistersP2)
        ReplayRegistersP2->SetText(FText::FromString(FormatRegs(1)));

    if (ReplaySourceAText)
        ReplaySourceAText->SetText(FText::FromString(FormatReplaySource(ReplayController->GetSourceA(), ReplayController->GetProgramA(), Snap.robots[0].vm.currentInstruction)));
    if (ReplaySourceBText)
        ReplaySourceBText->SetText(FText::FromString(FormatReplaySource(ReplayController->GetSourceB(), ReplayController->GetProgramB(), Snap.robots[1].vm.currentInstruction)));

    if (ReplayEventLog)
    {
        auto Events = ReplayController->GetEventsInRange(FMath::Max(0, Tick - 8), Tick);
        FString Log;
        for (const auto &E : Events)
        {
            Log += FString::Printf(TEXT("[T%d P%d] %s (%d,%d)\n"), E.tick, E.robot + 1, EventName(E.type), E.paramA, E.paramB);
        }
        ReplayEventLog->SetText(FText::FromString(Log));
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
    if (ReplaySpeedText)
    {
        ReplaySpeedText->SetText(FText::FromString(FString::Printf(TEXT("Speed: %.2fx"), Speed)));
    }
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
    if (bUpdatingReplaySlider || !ReplayController.IsValid() || !ReplayController->IsValid())
        return;

    const int32 Tick = FMath::RoundToInt32(Value * (ReplayController->GetTotalTicks() - 1));
    OnReplayScrub(Tick);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Replay Browser Actions
// ═══════════════════════════════════════════════════════════════════════════════

void UAWHUDWidget::RefreshReplayList()
{
    ReplayComboBox->ClearOptions();
    ReplayFilenames.Reset();

    UAWGameSubsystem *Sub = GetSubsystem();
    if (!Sub)
        return;

    TArray<FAWReplayInfo> Replays = Sub->GetReplayList();
    for (const FAWReplayInfo &Info : Replays)
    {
        FString Label = FString::Printf(TEXT("%s (%d bytes)"), *Info.Filename, Info.FileSizeBytes);
        ReplayComboBox->AddOption(Label);
        ReplayFilenames.Add(Info.Filename);
    }

    if (Replays.IsEmpty())
        ReplayBrowserStatus->SetText(FText::FromString(TEXT("No replays found in Saved/Replays/")));
    else
        ReplayComboBox->SetSelectedIndex(0);
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
        if (ReplayBrowserStatus)
            ReplayBrowserStatus->SetText(FText::FromString(Error));
        return;
    }

    if (!InitializeReplay(Src0, Src1, Seed))
    {
        if (ReplayBrowserStatus)
            ReplayBrowserStatus->SetText(FText::FromString(TEXT("Failed to resimulate replay.")));
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
        if (ReplayBrowserStatus)
            ReplayBrowserStatus->SetText(FText::FromString(FString::Printf(TEXT("Saved: %s"), *Filename)));
        RefreshReplayList();
    }
    else
    {
        if (ReplayBrowserStatus)
            ReplayBrowserStatus->SetText(FText::FromString(TEXT("Save failed (not in ReplayAutopsy phase?).")));
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
        if (ExportField)
            ExportField->SetText(FText::FromString(Base64));
        if (ReplayBrowserStatus)
            ReplayBrowserStatus->SetText(FText::FromString(TEXT("Exported to field above.")));
    }
    else
    {
        if (ReplayBrowserStatus)
            ReplayBrowserStatus->SetText(FText::FromString(TEXT("Export failed.")));
    }
}

void UAWHUDWidget::OnReplayImport()
{
    FString Base64 = ImportField->GetText().ToString().TrimStartAndEnd();
    if (Base64.IsEmpty())
    {
        if (ReplayBrowserStatus)
            ReplayBrowserStatus->SetText(FText::FromString(TEXT("Paste base64 data first.")));
        return;
    }

    UAWGameSubsystem *Sub = GetSubsystem();
    if (!Sub)
        return;

    FString Filename = FString::Printf(TEXT("imported_%s"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
    FString Error;
    if (Sub->ImportReplayBase64(Base64, Filename, Error))
    {
        if (ReplayBrowserStatus)
            ReplayBrowserStatus->SetText(FText::FromString(FString::Printf(TEXT("Imported: %s"), *Filename)));
        RefreshReplayList();
    }
    else
    {
        if (ReplayBrowserStatus)
            ReplayBrowserStatus->SetText(FText::FromString(Error));
    }
}

void UAWHUDWidget::OnReplayLoadSelected()
{
    const int32 ReplayIndex = ReplayComboBox->GetSelectedIndex();
    if (!ReplayFilenames.IsValidIndex(ReplayIndex))
    {
        ReplayBrowserStatus->SetText(FText::FromString(TEXT("Select a replay first.")));
        return;
    }
    OnReplayLoad(ReplayFilenames[ReplayIndex]);
}

void UAWHUDWidget::OnReplayExportSelected()
{
    const int32 ReplayIndex = ReplayComboBox->GetSelectedIndex();
    if (!ReplayFilenames.IsValidIndex(ReplayIndex))
    {
        ReplayBrowserStatus->SetText(FText::FromString(TEXT("Select a replay first.")));
        return;
    }
    OnReplayExport(ReplayFilenames[ReplayIndex]);
}

void UAWHUDWidget::OnReplayRefresh() { RefreshReplayList(); }

void UAWHUDWidget::PopulateLanguageReference()
{
    if (!LanguageReferenceText)
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
    LanguageReferenceText->SetText(FText::FromString(Reference));
}

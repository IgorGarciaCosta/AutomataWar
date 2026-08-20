#include "AWScreenWidget.h"
#include "AWTypewriterTextBlock.h"
#include "AutomataWar/Core/AutomataRules.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UObject/Class.h"

namespace
{
    /** Stable entry order in /Game/UI/Data/E_MatchResultMessage. */
    enum class EResultMessageIndex : int32
    {
        WinByHealth,
        WinByActionPoints,
        LoseByHealth,
        LoseByActionPoints,
        Draw
    };
}

#define BIND_SCREEN_BUTTON(Name, Class, Handler)                        \
    if (UButton *Button = Cast<UButton>(GetWidgetFromName(TEXT(Name)))) \
    {                                                                   \
        Button->OnClicked.AddUniqueDynamic(this, &Class::Handler);      \
    }

void UAWMainMenuScreen::NativeConstruct()
{
    Super::NativeConstruct();
    BIND_SCREEN_BUTTON("SinglePlayerButton", UAWMainMenuScreen, OnSinglePlayer);
    BIND_SCREEN_BUTTON("LocalMatchButton", UAWMainMenuScreen, OnLocalMatch);
    BIND_SCREEN_BUTTON("HostLanButton", UAWMainMenuScreen, OnHostLAN);
    BIND_SCREEN_BUTTON("FindLanButton", UAWMainMenuScreen, OnFindLAN);
    BIND_SCREEN_BUTTON("JoinSessionButton", UAWMainMenuScreen, OnJoinSession);
    BIND_SCREEN_BUTTON("JoinIpButton", UAWMainMenuScreen, OnJoinIP);
    BIND_SCREEN_BUTTON("ReplayBrowserButton", UAWMainMenuScreen, OnOpenReplayBrowser);
    BIND_SCREEN_BUTTON("LanguageReferenceButton", UAWMainMenuScreen, OnOpenLanguageReference);
    BIND_SCREEN_BUTTON("QuitButton", UAWMainMenuScreen, OnQuit);
}

void UAWMainMenuScreen::OnSinglePlayer() { BroadcastAction(EAWUIAction::SinglePlayer); }

void UAWMainMenuScreen::PlayTitleAnimation()
{
    if (MainTypewriterTitle)
        MainTypewriterTitle->PlayTypewriter();
}

FString UAWMainMenuScreen::GetJoinIPAddress() const
{
    return JoinIPField ? JoinIPField->GetText().ToString().TrimStartAndEnd() : FString();
}

int32 UAWMainMenuScreen::GetSelectedSessionIndex() const
{
    return SessionComboBox ? SessionComboBox->GetSelectedIndex() : INDEX_NONE;
}

void UAWMainMenuScreen::ResetSessions()
{
    if (SessionComboBox)
        SessionComboBox->ClearOptions();
}

void UAWMainMenuScreen::AddSession(const FString &Label)
{
    if (SessionComboBox)
        SessionComboBox->AddOption(Label);
}

void UAWMainMenuScreen::SelectFirstSession()
{
    if (SessionComboBox)
        SessionComboBox->SetSelectedIndex(0);
}

void UAWMainMenuScreen::OnLocalMatch() { BroadcastAction(EAWUIAction::LocalMatch); }
void UAWMainMenuScreen::OnHostLAN() { BroadcastAction(EAWUIAction::HostLAN); }
void UAWMainMenuScreen::OnFindLAN() { BroadcastAction(EAWUIAction::FindLAN); }
void UAWMainMenuScreen::OnJoinSession() { BroadcastAction(EAWUIAction::JoinSession); }
void UAWMainMenuScreen::OnJoinIP() { BroadcastAction(EAWUIAction::JoinIP); }
void UAWMainMenuScreen::OnOpenReplayBrowser() { BroadcastAction(EAWUIAction::OpenReplayBrowser); }
void UAWMainMenuScreen::OnOpenLanguageReference() { BroadcastAction(EAWUIAction::OpenLanguageReference); }
void UAWMainMenuScreen::OnQuit() { BroadcastAction(EAWUIAction::Quit); }

void UAWDifficultyScreen::NativeConstruct()
{
    Super::NativeConstruct();
    if (UTextBlock *Eyebrow = Cast<UTextBlock>(GetWidgetFromName(TEXT("SelectDifficultyEyebrow"))))
        Eyebrow->SetText(FText::FromString(TEXT("MATCH SETUP")));
    if (UTextBlock *Subtitle = Cast<UTextBlock>(GetWidgetFromName(TEXT("SelectDifficultySubtitle"))))
        Subtitle->SetText(FText::FromString(TEXT("CHOOSE STARTING AP; SOLO ALSO SETS AI DEPTH.")));
    if (UTextBlock *EasyDetail = Cast<UTextBlock>(GetWidgetFromName(TEXT("DifficultyEasyDetail"))))
        EasyDetail->SetText(FText::FromString(TEXT("150 AP // BASIC AI IN SOLO")));
    if (UTextBlock *NormalDetail = Cast<UTextBlock>(GetWidgetFromName(TEXT("DifficultyNormalDetail"))))
        NormalDetail->SetText(FText::FromString(TEXT("100 AP // BALANCED AI IN SOLO")));
    if (UTextBlock *HardDetail = Cast<UTextBlock>(GetWidgetFromName(TEXT("DifficultyHardDetail"))))
        HardDetail->SetText(FText::FromString(TEXT("75 AP // DEEP AI IN SOLO")));
    BIND_SCREEN_BUTTON("DifficultyEasyButton", UAWDifficultyScreen, OnEasy);
    BIND_SCREEN_BUTTON("DifficultyNormalButton", UAWDifficultyScreen, OnNormal);
    BIND_SCREEN_BUTTON("DifficultyHardButton", UAWDifficultyScreen, OnHard);
    BIND_SCREEN_BUTTON("DifficultyBackButton", UAWDifficultyScreen, OnBack);
}

void UAWDifficultyScreen::OnEasy() { BroadcastAction(EAWUIAction::DifficultyEasy); }
void UAWDifficultyScreen::OnNormal() { BroadcastAction(EAWUIAction::DifficultyNormal); }
void UAWDifficultyScreen::OnHard() { BroadcastAction(EAWUIAction::DifficultyHard); }
void UAWDifficultyScreen::OnBack() { BroadcastAction(EAWUIAction::BackToMainMenu); }

void UAWArenaSelectionScreen::NativeConstruct()
{
    Super::NativeConstruct();
    BIND_SCREEN_BUTTON("ArenaCompactButton", UAWArenaSelectionScreen, OnCompact);
    BIND_SCREEN_BUTTON("ArenaStandardButton", UAWArenaSelectionScreen, OnStandard);
    BIND_SCREEN_BUTTON("ArenaExpandedButton", UAWArenaSelectionScreen, OnExpanded);
    BIND_SCREEN_BUTTON("ArenaBackButton", UAWArenaSelectionScreen, OnBack);
}

void UAWArenaSelectionScreen::OnCompact() { BroadcastAction(EAWUIAction::ArenaCompact); }
void UAWArenaSelectionScreen::OnStandard() { BroadcastAction(EAWUIAction::ArenaStandard); }
void UAWArenaSelectionScreen::OnExpanded() { BroadcastAction(EAWUIAction::ArenaExpanded); }
void UAWArenaSelectionScreen::OnBack() { BroadcastAction(EAWUIAction::BackToDifficulty); }

void UAWProgrammingPanelWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BIND_SCREEN_BUTTON("ProgrammingMoveButton", UAWProgrammingPanelWidget, OnMove);
    BIND_SCREEN_BUTTON("ProgrammingFireButton", UAWProgrammingPanelWidget, OnFire);
    BIND_SCREEN_BUTTON("ProgrammingTurnLeftButton", UAWProgrammingPanelWidget, OnTurnLeft);
    BIND_SCREEN_BUTTON("ProgrammingTurnRightButton", UAWProgrammingPanelWidget, OnTurnRight);
    BIND_SCREEN_BUTTON("ProgrammingWaitButton", UAWProgrammingPanelWidget, OnWait);
    BIND_SCREEN_BUTTON("ProgrammingChargeShieldButton", UAWProgrammingPanelWidget, OnChargeShield);
    BIND_SCREEN_BUTTON("ProgrammingAccelerateButton", UAWProgrammingPanelWidget, OnAccelerate);
    BIND_SCREEN_BUTTON("ProgrammingRemoveActionButton", UAWProgrammingPanelWidget, OnRemove);
    BIND_SCREEN_BUTTON("ProgrammingSubmitButton", UAWProgrammingPanelWidget, OnSubmit);
    BIND_SCREEN_BUTTON("ProgrammingReturnToPlanningButton", UAWProgrammingPanelWidget, OnReturnToPlanning);
    ResetSubmissionState();
    RefreshCommands();
}

void UAWProgrammingPanelWidget::SynchronizeProperties()
{
    Super::SynchronizeProperties();
    if (ProgrammingPlayerTitle)
    {
        ProgrammingPlayerTitle->SetText(PlayerLabel);
        ProgrammingPlayerTitle->SetColorAndOpacity(FSlateColor(AccentColor));
    }
    if (ProgrammingPlayerSlot)
        ProgrammingPlayerSlot->SetText(FText::FromString(FString::Printf(TEXT("SLOT %d"), PlayerIndex)));
    if (ProgrammingCommandsTitle)
        ProgrammingCommandsTitle->SetColorAndOpacity(FSlateColor(AccentColor));
    if (ProgrammingSubmitButton)
        ProgrammingSubmitButton->SetBackgroundColor(AccentColor);
    if (ProgrammingReturnToPlanningButton)
        ProgrammingReturnToPlanningButton->SetBackgroundColor(AccentColor);
}

void UAWProgrammingPanelWidget::NativeTick(const FGeometry &MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (PowerTransitionDirection == 0)
        return;

    PowerTransitionAlpha = FMath::Clamp(
        PowerTransitionAlpha + PowerTransitionDirection * InDeltaTime / PowerTransitionDuration,
        0.f, 1.f);
    ApplyPowerTransition();

    if (PowerTransitionDirection > 0 && PowerTransitionAlpha >= 1.f)
    {
        PowerTransitionDirection = 0;
        ProgrammingPanelContent->SetVisibility(ESlateVisibility::Hidden);
        ProgrammingShutdownLine->SetVisibility(ESlateVisibility::Collapsed);
        ProgrammingReturnLayer->SetVisibility(ESlateVisibility::Visible);
        OnSubmitted.Broadcast(PlayerIndex);
    }
    else if (PowerTransitionDirection < 0 && PowerTransitionAlpha <= 0.f)
    {
        PowerTransitionDirection = 0;
        ProgrammingPanelContent->SetIsEnabled(true);
        ProgrammingShutdownLine->SetVisibility(ESlateVisibility::Collapsed);
    }
}

FString UAWProgrammingPanelWidget::GetCommandText() const
{
    FString Text;
    for (EAWCommand Command : Commands)
        Text += FString::Printf(TEXT("%s\n"), LexToString(Command));
    return Text;
}

void UAWProgrammingPanelWidget::ResolveSubmission(bool bAccepted)
{
    bAwaitingSubmissionResult = false;
    bSubmitted = bAccepted;
    if (!bAccepted)
        StartPowerTransition(false);
}

void UAWProgrammingPanelWidget::ResetSubmissionState()
{
    bAwaitingSubmissionResult = false;
    bSubmitted = false;
    PowerTransitionAlpha = 0.f;
    PowerTransitionDirection = 0;
    ProgrammingPanelContent->SetVisibility(ESlateVisibility::Visible);
    ProgrammingPanelContent->SetIsEnabled(!bAIControlled);
    ProgrammingReturnLayer->SetVisibility(ESlateVisibility::Collapsed);
    ProgrammingShutdownLine->SetVisibility(ESlateVisibility::Collapsed);
    ApplyPowerTransition();
    RefreshCommands();
}

void UAWProgrammingPanelWidget::SetPlayerStats(int32 Health, int32 ActionPoints)
{
    PlayerHealth = FMath::Max(0, Health);
    AvailableActionPoints = FMath::Max(0, ActionPoints);
    RefreshCommands();
}

void UAWProgrammingPanelWidget::SetAIControlled(bool bInAIControlled)
{
    bAIControlled = bInAIControlled;
    Commands.Reset();
    if (ProgrammingPanelContent)
        ProgrammingPanelContent->SetIsEnabled(!bAIControlled);
    if (ProgrammingPlayerTitle)
        ProgrammingPlayerTitle->SetText(bAIControlled ? INVTEXT("AI OPPONENT") : PlayerLabel);
    if (ProgrammingSubmitButton)
        ProgrammingSubmitButton->SetVisibility(bAIControlled ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    if (ProgrammingReturnToPlanningButton)
        ProgrammingReturnToPlanningButton->SetVisibility(bAIControlled ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    RefreshCommands();
    OnCommandsChanged.Broadcast(PlayerIndex);
}

void UAWProgrammingPanelWidget::ResetForNewRound(int32 Health, int32 ActionPoints)
{
    Commands.Reset();
    SetPlayerStats(Health, ActionPoints);
    ResetSubmissionState();
    OnCommandsChanged.Broadcast(PlayerIndex);
}

void UAWProgrammingPanelWidget::AddCommand(EAWCommand Command)
{
    const int32 Cost = GetActionPointCost(Command);
    if (!bAIControlled && !bSubmitted && !bAwaitingSubmissionResult && Commands.Num() < Automata::MaxCommands &&
        Cost <= AvailableActionPoints)
    {
        Commands.Add(Command);
        AvailableActionPoints -= Cost;
        RefreshCommands();
        OnCommandsChanged.Broadcast(PlayerIndex);
    }
}

void UAWProgrammingPanelWidget::RefreshCommands()
{
    FString Text;
    for (int32 Index = 0; Index < Commands.Num(); ++Index)
        Text += FString::Printf(TEXT("%3d | %s\n"), Index + 1, LexToString(Commands[Index]));
    if (bAIControlled)
        Text = TEXT("AI QUEUE GENERATED AFTER YOUR SUBMISSION");
    else if (Text.IsEmpty())
        Text = TEXT("NO ACTIONS SELECTED");

    if (ProgrammingProgramText)
        ProgrammingProgramText->SetText(FText::FromString(Text));
    if (ProgrammingRemoveActionButton)
        ProgrammingRemoveActionButton->SetVisibility(bAIControlled || Commands.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    if (UTextBlock *Stats = Cast<UTextBlock>(GetWidgetFromName(TEXT("ProgrammingPlayerStats"))))
        Stats->SetText(FText::FromString(FString::Printf(TEXT("HP %d  |  AP %d"), PlayerHealth, AvailableActionPoints)));

    const bool bHasQueueCapacity = Commands.Num() < Automata::MaxCommands;
    const TPair<const TCHAR *, EAWCommand> CommandButtons[] = {
        {TEXT("ProgrammingMoveButton"), EAWCommand::Move},
        {TEXT("ProgrammingFireButton"), EAWCommand::Fire},
        {TEXT("ProgrammingTurnLeftButton"), EAWCommand::TurnLeft},
        {TEXT("ProgrammingTurnRightButton"), EAWCommand::TurnRight},
        {TEXT("ProgrammingWaitButton"), EAWCommand::Wait},
        {TEXT("ProgrammingChargeShieldButton"), EAWCommand::ChargeShield},
        {TEXT("ProgrammingAccelerateButton"), EAWCommand::Accelerate}};
    for (const TPair<const TCHAR *, EAWCommand> &Entry : CommandButtons)
        if (UButton *Button = Cast<UButton>(GetWidgetFromName(Entry.Key)))
            Button->SetIsEnabled(!bAIControlled && bHasQueueCapacity && GetActionPointCost(Entry.Value) <= AvailableActionPoints);
}

void UAWProgrammingPanelWidget::StartPowerTransition(bool bTurningOff)
{
    ProgrammingReturnLayer->SetVisibility(ESlateVisibility::Collapsed);
    ProgrammingPanelContent->SetVisibility(ESlateVisibility::Visible);
    ProgrammingPanelContent->SetIsEnabled(false);
    ProgrammingShutdownLine->SetVisibility(ESlateVisibility::HitTestInvisible);
    PowerTransitionDirection = bTurningOff ? 1 : -1;
}

void UAWProgrammingPanelWidget::ApplyPowerTransition()
{
    const float EasedAlpha = FMath::InterpEaseInOut(0.f, 1.f, PowerTransitionAlpha, 2.f);
    const float VerticalCollapse = FMath::Clamp(EasedAlpha / 0.68f, 0.f, 1.f);
    const float HorizontalCollapse = FMath::Clamp((EasedAlpha - 0.68f) / 0.32f, 0.f, 1.f);
    const FVector2D Scale(
        FMath::Lerp(1.f, 0.015f, HorizontalCollapse),
        FMath::Lerp(1.f, 0.025f, VerticalCollapse));

    ProgrammingPanelContent->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
    ProgrammingPanelContent->SetRenderScale(Scale);
    ProgrammingPanelContent->SetRenderOpacity(FMath::Lerp(1.f, 0.16f, HorizontalCollapse));
    ProgrammingShutdownLine->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
    ProgrammingShutdownLine->SetRenderScale(FVector2D(Scale.X, 1.f));
    ProgrammingShutdownLine->SetRenderOpacity(FMath::Sin(PI * PowerTransitionAlpha));
}

void UAWProgrammingPanelWidget::OnMove() { AddCommand(EAWCommand::Move); }
void UAWProgrammingPanelWidget::OnFire() { AddCommand(EAWCommand::Fire); }
void UAWProgrammingPanelWidget::OnTurnLeft() { AddCommand(EAWCommand::TurnLeft); }
void UAWProgrammingPanelWidget::OnTurnRight() { AddCommand(EAWCommand::TurnRight); }
void UAWProgrammingPanelWidget::OnWait() { AddCommand(EAWCommand::Wait); }
void UAWProgrammingPanelWidget::OnChargeShield() { AddCommand(EAWCommand::ChargeShield); }
void UAWProgrammingPanelWidget::OnAccelerate() { AddCommand(EAWCommand::Accelerate); }

void UAWProgrammingPanelWidget::OnRemove()
{
    if (!bAIControlled && !bSubmitted && !bAwaitingSubmissionResult && !Commands.IsEmpty())
    {
        AvailableActionPoints += GetActionPointCost(Commands.Last());
        Commands.Pop();
        RefreshCommands();
        OnCommandsChanged.Broadcast(PlayerIndex);
    }
}

void UAWProgrammingPanelWidget::OnSubmit()
{
    if (bAIControlled || bSubmitted || bAwaitingSubmissionResult || PowerTransitionDirection != 0)
        return;

    bAwaitingSubmissionResult = true;
    StartPowerTransition(true);
}

void UAWProgrammingPanelWidget::OnReturnToPlanning()
{
    if (bAIControlled || !bSubmitted || PowerTransitionDirection != 0)
        return;

    bSubmitted = false;
    OnPlanningReturned.Broadcast(PlayerIndex);
    StartPowerTransition(false);
}

void UAWProgrammingScreen::NativeConstruct()
{
    Super::NativeConstruct();
    BIND_SCREEN_BUTTON("ProgrammingBackButton", UAWProgrammingScreen, OnBack);
    for (UAWProgrammingPanelWidget *Panel : {ProgrammingP1PanelWidget.Get(), ProgrammingP2PanelWidget.Get()})
    {
        if (!Panel)
            continue;
        Panel->OnSubmitted.RemoveAll(this);
        Panel->OnSubmitted.AddUObject(this, &UAWProgrammingScreen::OnPanelSubmitted);
        Panel->OnPlanningReturned.RemoveAll(this);
        Panel->OnPlanningReturned.AddUObject(this, &UAWProgrammingScreen::OnPanelReturned);
    }
}

UAWProgrammingPanelWidget *UAWProgrammingScreen::GetPanel(int32 PlayerIndex) const
{
    return PlayerIndex == 0 ? ProgrammingP1PanelWidget.Get() : ProgrammingP2PanelWidget.Get();
}

TArray<EAWCommand> UAWProgrammingScreen::GetCommands(int32 PlayerIndex) const
{
    if (const UAWProgrammingPanelWidget *Panel = GetPanel(FMath::Clamp(PlayerIndex, 0, 1)))
        return Panel->GetCommands();
    return {};
}

FString UAWProgrammingScreen::GetCommandText(int32 PlayerIndex) const
{
    if (const UAWProgrammingPanelWidget *Panel = GetPanel(FMath::Clamp(PlayerIndex, 0, 1)))
        return Panel->GetCommandText();
    return {};
}

void UAWProgrammingScreen::ResolveSubmission(int32 PlayerIndex, bool bAccepted)
{
    if (UAWProgrammingPanelWidget *Panel = GetPanel(FMath::Clamp(PlayerIndex, 0, 1)))
        Panel->ResolveSubmission(bAccepted);
}

void UAWProgrammingScreen::ResetSubmissionState()
{
    if (ProgrammingP1PanelWidget)
        ProgrammingP1PanelWidget->ResetSubmissionState();
    if (ProgrammingP2PanelWidget)
        ProgrammingP2PanelWidget->ResetSubmissionState();
}

void UAWProgrammingScreen::ResetForNewRound(int32 ActionPoints0, int32 ActionPoints1)
{
    if (ProgrammingP1PanelWidget)
        ProgrammingP1PanelWidget->ResetForNewRound(Automata::MaxHP, ActionPoints0);
    if (ProgrammingP2PanelWidget)
        ProgrammingP2PanelWidget->ResetForNewRound(Automata::MaxHP, ActionPoints1);
}

void UAWProgrammingScreen::SetPlayerStats(int32 PlayerIndex, int32 Health, int32 ActionPoints)
{
    if (UAWProgrammingPanelWidget *Panel = GetPanel(FMath::Clamp(PlayerIndex, 0, 1)))
        Panel->SetPlayerStats(Health, ActionPoints);
}

void UAWProgrammingScreen::SetSinglePlayerMode(bool bSinglePlayer)
{
    if (ProgrammingP2PanelWidget)
        ProgrammingP2PanelWidget->SetAIControlled(bSinglePlayer);
}

void UAWProgrammingScreen::OnBack() { BroadcastAction(EAWUIAction::BackToMainMenu); }

void UAWProgrammingScreen::OnPanelSubmitted(int32 PlayerIndex)
{
    BroadcastAction(PlayerIndex == 0 ? EAWUIAction::SubmitP1 : EAWUIAction::SubmitP2);
}

void UAWProgrammingScreen::OnPanelReturned(int32 PlayerIndex)
{
    BroadcastAction(PlayerIndex == 0 ? EAWUIAction::ReturnToPlanningP1 : EAWUIAction::ReturnToPlanningP2);
}

void UAWSimulationDockWidget::SynchronizeProperties()
{
    Super::SynchronizeProperties();
    if (SimulationDockTitle)
    {
        SimulationDockTitle->SetText(PlayerLabel);
        SimulationDockTitle->SetColorAndOpacity(FSlateColor(AccentColor));
    }
    if (SimulationDockDetails)
        SimulationDockDetails->SetColorAndOpacity(FSlateColor(AccentColor));
}

void UAWSimulationDockWidget::SetCommands(const TArray<EAWCommand> &Commands, int32 CurrentCommand)
{
    FString Text;
    for (int32 Index = 0; Index < Commands.Num(); ++Index)
        Text += FString::Printf(TEXT("%s %3d | %s\n"), Index == CurrentCommand ? TEXT(">>") : TEXT("  "), Index + 1, LexToString(Commands[Index]));
    if (SimulationDockCommandsText)
        SimulationDockCommandsText->SetText(FText::FromString(Text));
}

void UAWSimulationDockWidget::SetDetails(const FString &Details)
{
    if (SimulationDockDetails)
    {
        SimulationDockDetails->SetText(FText::FromString(Details));
        SimulationDockDetails->SetVisibility(Details.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    }
}

void UAWSimulationScreen::SetCommands(const TArray<EAWCommand> &PlayerOneCommands, const TArray<EAWCommand> &PlayerTwoCommands)
{
    if (SimulationP1DockWidget)
        SimulationP1DockWidget->SetCommands(PlayerOneCommands);
    if (SimulationP2DockWidget)
        SimulationP2DockWidget->SetCommands(PlayerTwoCommands);
}

void UAWSimulationScreen::SetArenaRenderTarget(UTextureRenderTarget2D *RenderTarget)
{
    if (SimulationArenaFeed)
        SimulationArenaFeed->SetBrushResourceObject(RenderTarget);
}

void UAWSimulationScreen::SetPlayerDetails(int32 PlayerIndex, const FString &Details)
{
    UAWSimulationDockWidget *Dock = PlayerIndex == 0 ? SimulationP1DockWidget.Get() : SimulationP2DockWidget.Get();
    if (Dock)
        Dock->SetDetails(Details);
}

void UAWReplayAutopsyScreen::NativeConstruct()
{
    Super::NativeConstruct();
    BIND_SCREEN_BUTTON("ReplayStartButton", UAWReplayAutopsyScreen, OnStart);
    BIND_SCREEN_BUTTON("ReplayBackButton", UAWReplayAutopsyScreen, OnBack);
    BIND_SCREEN_BUTTON("ReplayPauseButton", UAWReplayAutopsyScreen, OnPause);
    BIND_SCREEN_BUTTON("ReplayPlayButton", UAWReplayAutopsyScreen, OnPlay);
    BIND_SCREEN_BUTTON("ReplayStepButton", UAWReplayAutopsyScreen, OnStep);
    BIND_SCREEN_BUTTON("ReplayQuarterButton", UAWReplayAutopsyScreen, OnSpeedQuarter);
    BIND_SCREEN_BUTTON("ReplayNormalButton", UAWReplayAutopsyScreen, OnSpeedNormal);
    BIND_SCREEN_BUTTON("ReplayDoubleButton", UAWReplayAutopsyScreen, OnSpeedDouble);
    BIND_SCREEN_BUTTON("ReplayQuadButton", UAWReplayAutopsyScreen, OnSpeedQuadruple);
    BIND_SCREEN_BUTTON("ReplayBackToMenuButton", UAWReplayAutopsyScreen, OnBackToMenu);
    BIND_SCREEN_BUTTON("NextRoundButton", UAWReplayAutopsyScreen, OnNextRound);
    for (UAWProgrammingPanelWidget *Panel : {ProgrammingP1PanelWidget.Get(), ProgrammingP2PanelWidget.Get()})
    {
        if (!Panel)
            continue;
        Panel->OnSubmitted.RemoveAll(this);
        Panel->OnSubmitted.AddUObject(this, &UAWReplayAutopsyScreen::OnPanelSubmitted);
        Panel->OnPlanningReturned.RemoveAll(this);
        Panel->OnPlanningReturned.AddUObject(this, &UAWReplayAutopsyScreen::OnPanelReturned);
        Panel->OnCommandsChanged.RemoveAll(this);
        Panel->OnCommandsChanged.AddUObject(this, &UAWReplayAutopsyScreen::OnPanelCommandsChanged);
    }
    SetProgrammingMode(false, false);
}

UAWProgrammingPanelWidget *UAWReplayAutopsyScreen::GetProgrammingPanel(int32 PlayerIndex) const
{
    return PlayerIndex == 0 ? ProgrammingP1PanelWidget.Get() : ProgrammingP2PanelWidget.Get();
}

TArray<EAWCommand> UAWReplayAutopsyScreen::GetProgrammingCommands(int32 PlayerIndex) const
{
    if (const UAWProgrammingPanelWidget *Panel = GetProgrammingPanel(FMath::Clamp(PlayerIndex, 0, 1)))
        return Panel->GetCommands();
    return {};
}

void UAWReplayAutopsyScreen::ResolveProgrammingSubmission(int32 PlayerIndex, bool bAccepted)
{
    if (UAWProgrammingPanelWidget *Panel = GetProgrammingPanel(FMath::Clamp(PlayerIndex, 0, 1)))
        Panel->ResolveSubmission(bAccepted);
}

void UAWReplayAutopsyScreen::ResetProgrammingForNewRound(int32 ActionPoints0, int32 ActionPoints1)
{
    if (ProgrammingP1PanelWidget)
        ProgrammingP1PanelWidget->ResetForNewRound(Automata::MaxHP, ActionPoints0);
    if (ProgrammingP2PanelWidget)
        ProgrammingP2PanelWidget->ResetForNewRound(Automata::MaxHP, ActionPoints1);
}

void UAWReplayAutopsyScreen::SetProgrammingPlayerStats(int32 PlayerIndex, int32 Health, int32 ActionPoints)
{
    if (UAWProgrammingPanelWidget *Panel = GetProgrammingPanel(FMath::Clamp(PlayerIndex, 0, 1)))
        Panel->SetPlayerStats(Health, ActionPoints);
}

void UAWReplayAutopsyScreen::SetSinglePlayerMode(bool bSinglePlayer)
{
    if (ProgrammingP2PanelWidget)
        ProgrammingP2PanelWidget->SetAIControlled(bSinglePlayer);
}

void UAWReplayAutopsyScreen::SetProgrammingMode(bool bProgramming, bool bCanAdvanceRound)
{
    for (UAWProgrammingPanelWidget *Panel : {ProgrammingP1PanelWidget.Get(), ProgrammingP2PanelWidget.Get()})
        if (Panel)
            Panel->SetVisibility(bProgramming ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    for (UAWSimulationDockWidget *Dock : {ReplayP1DockWidget.Get(), ReplayP2DockWidget.Get()})
        if (Dock)
            Dock->SetVisibility(bProgramming ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    if (ReplayPlaybackControls)
        ReplayPlaybackControls->SetIsEnabled(!bProgramming);
    if (NextRoundButton)
        NextRoundButton->SetIsEnabled(!bProgramming && bCanAdvanceRound);
    if (ReplayEyebrow)
        ReplayEyebrow->SetText(bProgramming ? INVTEXT("MATCH PROGRAMMING") : INVTEXT("POST-MATCH ANALYSIS"));
    if (ReplayTitle)
        ReplayTitle->SetText(bProgramming ? INVTEXT("TACTICAL CONSOLE") : INVTEXT("REPLAY AUTOPSY"));
    if (bProgramming && ReplaySpeedText)
        ReplaySpeedText->SetText(INVTEXT("PROGRAM BOTH COMBATANTS TO BEGIN"));
}

void UAWReplayAutopsyScreen::SetSpeed(float Speed)
{
    if (ReplaySpeedText)
        ReplaySpeedText->SetText(FText::FromString(FString::Printf(TEXT("Speed: %.2fx"), Speed)));
}

void UAWReplayAutopsyScreen::SetArenaRenderTarget(UTextureRenderTarget2D *RenderTarget)
{
    if (ReplayArenaFeed)
        ReplayArenaFeed->SetBrushResourceObject(RenderTarget);
}

void UAWReplayAutopsyScreen::SetCombatantData(int32 PlayerIndex, const TArray<EAWCommand> &Commands, int32 CurrentCommand, const FString &Details)
{
    UAWSimulationDockWidget *Dock = PlayerIndex == 0 ? ReplayP1DockWidget.Get() : ReplayP2DockWidget.Get();
    if (Dock)
    {
        Dock->SetCommands(Commands, CurrentCommand);
        Dock->SetDetails(Details);
    }
}

void UAWReplayAutopsyScreen::OnStart() { BroadcastAction(EAWUIAction::ReplayStart); }
void UAWReplayAutopsyScreen::OnBack() { BroadcastAction(EAWUIAction::ReplayBack); }
void UAWReplayAutopsyScreen::OnPause() { BroadcastAction(EAWUIAction::ReplayPause); }
void UAWReplayAutopsyScreen::OnPlay() { BroadcastAction(EAWUIAction::ReplayPlay); }
void UAWReplayAutopsyScreen::OnStep() { BroadcastAction(EAWUIAction::ReplayStep); }
void UAWReplayAutopsyScreen::OnSpeedQuarter() { BroadcastAction(EAWUIAction::ReplaySpeedQuarter); }
void UAWReplayAutopsyScreen::OnSpeedNormal() { BroadcastAction(EAWUIAction::ReplaySpeedNormal); }
void UAWReplayAutopsyScreen::OnSpeedDouble() { BroadcastAction(EAWUIAction::ReplaySpeedDouble); }
void UAWReplayAutopsyScreen::OnSpeedQuadruple() { BroadcastAction(EAWUIAction::ReplaySpeedQuadruple); }
void UAWReplayAutopsyScreen::OnBackToMenu() { BroadcastAction(EAWUIAction::BackToMainMenu); }
void UAWReplayAutopsyScreen::OnNextRound() { BroadcastAction(EAWUIAction::NextRound); }

void UAWReplayAutopsyScreen::OnPanelSubmitted(int32 PlayerIndex)
{
    BroadcastAction(PlayerIndex == 0 ? EAWUIAction::SubmitP1 : EAWUIAction::SubmitP2);
}

void UAWReplayAutopsyScreen::OnPanelReturned(int32 PlayerIndex)
{
    BroadcastAction(PlayerIndex == 0 ? EAWUIAction::ReturnToPlanningP1 : EAWUIAction::ReturnToPlanningP2);
}

void UAWReplayAutopsyScreen::OnPanelCommandsChanged(int32 PlayerIndex)
{
    OnProgrammingCommandsChanged.Broadcast(PlayerIndex);
}

void UAWMatchResultPopupWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BIND_SCREEN_BUTTON("MatchResultReturnButton", UAWMatchResultPopupWidget, OnReturnToMainMenu);
    SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
    SetVisibility(ESlateVisibility::Collapsed);
}

void UAWMatchResultPopupWidget::NativeTick(const FGeometry &MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (TransitionDirection == 0)
        return;

    TransitionAlpha = FMath::Clamp(
        TransitionAlpha + TransitionDirection * InDeltaTime / TransitionDuration,
        0.f, 1.f);
    ApplyTransition();

    if (TransitionDirection > 0 && TransitionAlpha >= 1.f)
    {
        TransitionDirection = 0;
    }
    else if (TransitionDirection < 0 && TransitionAlpha <= 0.f)
    {
        TransitionDirection = 0;
        SetVisibility(ESlateVisibility::Collapsed);
        BroadcastAction(EAWUIAction::BackToMainMenu);
    }
}

void UAWMatchResultPopupWidget::ShowResult(int32 WinnerSlot, int32 ViewerSlot,
                                           EAWMatchEndReason EndReason, bool bUsePlayerLabels)
{
    if (MatchResultText)
        MatchResultText->SetText(FormatResultText(
            WinnerSlot, ViewerSlot, EndReason, bUsePlayerLabels, MatchResultMessageEnum));
    if (MatchResultReturnButton)
        MatchResultReturnButton->SetIsEnabled(true);

    TransitionAlpha = 0.f;
    TransitionDirection = 1;
    SetVisibility(ESlateVisibility::Visible);
    ApplyTransition();
}

FText UAWMatchResultPopupWidget::FormatResultText(int32 WinnerSlot, int32 ViewerSlot,
                                                  EAWMatchEndReason EndReason, bool bUsePlayerLabels,
                                                  const UEnum *MessageEnum)
{
    const bool bDraw = WinnerSlot != 0 && WinnerSlot != 1;
    const bool bViewerWon = WinnerSlot == ViewerSlot;
    const EResultMessageIndex MessageIndex = bDraw
                                                 ? EResultMessageIndex::Draw
                                             : EndReason == EAWMatchEndReason::ActionPoints
                                                 ? (bViewerWon ? EResultMessageIndex::WinByActionPoints
                                                               : EResultMessageIndex::LoseByActionPoints)
                                                 : (bViewerWon ? EResultMessageIndex::WinByHealth
                                                               : EResultMessageIndex::LoseByHealth);
    if (!MessageEnum)
        MessageEnum = LoadObject<UEnum>(nullptr,
                                        TEXT("/Game/UI/Data/E_MatchResultMessage.E_MatchResultMessage"));
    const FText Message = MessageEnum && static_cast<int32>(MessageIndex) < MessageEnum->NumEnums() - 1
                              ? MessageEnum->GetDisplayNameTextByIndex(static_cast<int32>(MessageIndex))
                              : INVTEXT("MATCH RESULT UNAVAILABLE");
    return bUsePlayerLabels
               ? FText::Format(INVTEXT("P{0}: {1}"), FText::AsNumber(ViewerSlot + 1), Message)
               : Message;
}

void UAWMatchResultPopupWidget::OnReturnToMainMenu()
{
    if (TransitionDirection != 0 || TransitionAlpha <= 0.f)
        return;
    if (MatchResultReturnButton)
        MatchResultReturnButton->SetIsEnabled(false);
    TransitionDirection = -1;
}

void UAWMatchResultPopupWidget::ApplyTransition()
{
    const float EasedAlpha = FMath::InterpEaseOut(0.f, 1.f, TransitionAlpha, 3.f);
    SetRenderTranslation(FVector2D(0.f, FMath::Lerp(900.f, 0.f, EasedAlpha)));
    SetRenderOpacity(EasedAlpha);
}

void UAWReplayBrowserScreen::NativeConstruct()
{
    Super::NativeConstruct();
    BIND_SCREEN_BUTTON("ReplayBrowserBackButton", UAWReplayBrowserScreen, OnBack);
    BIND_SCREEN_BUTTON("ReplayRefreshButton", UAWReplayBrowserScreen, OnRefresh);
    BIND_SCREEN_BUTTON("ReplaySaveButton", UAWReplayBrowserScreen, OnSave);
    BIND_SCREEN_BUTTON("ReplayLoadButton", UAWReplayBrowserScreen, OnLoad);
    BIND_SCREEN_BUTTON("ReplayExportButton", UAWReplayBrowserScreen, OnExport);
    BIND_SCREEN_BUTTON("ReplayImportButton", UAWReplayBrowserScreen, OnImport);
}

int32 UAWReplayBrowserScreen::GetSelectedReplayIndex() const
{
    return ReplayComboBox ? ReplayComboBox->GetSelectedIndex() : INDEX_NONE;
}

FString UAWReplayBrowserScreen::GetImportData() const
{
    return ImportField ? ImportField->GetText().ToString().TrimStartAndEnd() : FString();
}

void UAWReplayBrowserScreen::ResetReplays()
{
    if (ReplayComboBox)
        ReplayComboBox->ClearOptions();
}

void UAWReplayBrowserScreen::AddReplay(const FString &Label)
{
    if (ReplayComboBox)
        ReplayComboBox->AddOption(Label);
}

void UAWReplayBrowserScreen::SelectFirstReplay()
{
    if (ReplayComboBox)
        ReplayComboBox->SetSelectedIndex(0);
}

void UAWReplayBrowserScreen::SetExportData(const FString &Data)
{
    if (ExportField)
        ExportField->SetText(FText::FromString(Data));
}

void UAWReplayBrowserScreen::SetStatus(const FString &Status)
{
    if (ReplayBrowserStatus)
        ReplayBrowserStatus->SetText(FText::FromString(Status));
}

void UAWReplayBrowserScreen::OnBack() { BroadcastAction(EAWUIAction::BackToMainMenu); }
void UAWReplayBrowserScreen::OnRefresh() { BroadcastAction(EAWUIAction::ReplayRefresh); }
void UAWReplayBrowserScreen::OnSave() { BroadcastAction(EAWUIAction::ReplaySave); }
void UAWReplayBrowserScreen::OnLoad() { BroadcastAction(EAWUIAction::ReplayLoad); }
void UAWReplayBrowserScreen::OnExport() { BroadcastAction(EAWUIAction::ReplayExport); }
void UAWReplayBrowserScreen::OnImport() { BroadcastAction(EAWUIAction::ReplayImport); }

void UAWLanguageReferenceScreen::NativeConstruct()
{
    Super::NativeConstruct();
    BIND_SCREEN_BUTTON("LanguageBackButton", UAWLanguageReferenceScreen, OnBack);
}

void UAWLanguageReferenceScreen::SetReference(const FString &Reference)
{
    if (LanguageReferenceText)
        LanguageReferenceText->SetText(FText::FromString(Reference));
}

void UAWLanguageReferenceScreen::OnBack() { BroadcastAction(EAWUIAction::BackToMainMenu); }

#undef BIND_SCREEN_BUTTON
#include "AWScreenWidget.h"
#include "AutomataWar/Core/AutomataRules.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"

#define BIND_SCREEN_BUTTON(Name, Class, Handler)                        \
    if (UButton *Button = Cast<UButton>(GetWidgetFromName(TEXT(Name)))) \
    {                                                                   \
        Button->OnClicked.AddUniqueDynamic(this, &Class::Handler);      \
    }

void UAWMainMenuScreen::NativeConstruct()
{
    Super::NativeConstruct();
    BIND_SCREEN_BUTTON("LocalMatchButton", UAWMainMenuScreen, OnLocalMatch);
    BIND_SCREEN_BUTTON("HostLanButton", UAWMainMenuScreen, OnHostLAN);
    BIND_SCREEN_BUTTON("FindLanButton", UAWMainMenuScreen, OnFindLAN);
    BIND_SCREEN_BUTTON("JoinSessionButton", UAWMainMenuScreen, OnJoinSession);
    BIND_SCREEN_BUTTON("JoinIpButton", UAWMainMenuScreen, OnJoinIP);
    BIND_SCREEN_BUTTON("ReplayBrowserButton", UAWMainMenuScreen, OnOpenReplayBrowser);
    BIND_SCREEN_BUTTON("LanguageReferenceButton", UAWMainMenuScreen, OnOpenLanguageReference);
    BIND_SCREEN_BUTTON("QuitButton", UAWMainMenuScreen, OnQuit);
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

void UAWProgrammingScreen::NativeConstruct()
{
    Super::NativeConstruct();
    BIND_SCREEN_BUTTON("ProgrammingBackButton", UAWProgrammingScreen, OnBack);
    BIND_SCREEN_BUTTON("SubmitP1Button", UAWProgrammingScreen, OnSubmitP1);
    BIND_SCREEN_BUTTON("SubmitP2Button", UAWProgrammingScreen, OnSubmitP2);
    BIND_SCREEN_BUTTON("MoveP1Button", UAWProgrammingScreen, OnMoveP1);
    BIND_SCREEN_BUTTON("FireP1Button", UAWProgrammingScreen, OnFireP1);
    BIND_SCREEN_BUTTON("TurnLeftP1Button", UAWProgrammingScreen, OnTurnLeftP1);
    BIND_SCREEN_BUTTON("TurnRightP1Button", UAWProgrammingScreen, OnTurnRightP1);
    BIND_SCREEN_BUTTON("RemoveActionP1Button", UAWProgrammingScreen, OnRemoveP1);
    BIND_SCREEN_BUTTON("MoveP2Button", UAWProgrammingScreen, OnMoveP2);
    BIND_SCREEN_BUTTON("FireP2Button", UAWProgrammingScreen, OnFireP2);
    BIND_SCREEN_BUTTON("TurnLeftP2Button", UAWProgrammingScreen, OnTurnLeftP2);
    BIND_SCREEN_BUTTON("TurnRightP2Button", UAWProgrammingScreen, OnTurnRightP2);
    BIND_SCREEN_BUTTON("RemoveActionP2Button", UAWProgrammingScreen, OnRemoveP2);
    RefreshCommands(0);
    RefreshCommands(1);
}

TArray<EAWCommand> UAWProgrammingScreen::GetCommands(int32 PlayerIndex) const
{
    return Commands[FMath::Clamp(PlayerIndex, 0, 1)];
}

FString UAWProgrammingScreen::GetCommandText(int32 PlayerIndex) const
{
    FString Text;
    for (EAWCommand Command : Commands[FMath::Clamp(PlayerIndex, 0, 1)])
        Text += FString::Printf(TEXT("%s\n"), LexToString(Command));
    return Text;
}

void UAWProgrammingScreen::AddCommand(int32 PlayerIndex, EAWCommand Command)
{
    if (Commands[PlayerIndex].Num() < Automata::MaxCommands)
    {
        Commands[PlayerIndex].Add(Command);
        RefreshCommands(PlayerIndex);
    }
}

void UAWProgrammingScreen::RemoveLastCommand(int32 PlayerIndex)
{
    if (!Commands[PlayerIndex].IsEmpty())
    {
        Commands[PlayerIndex].Pop();
        RefreshCommands(PlayerIndex);
    }
}

void UAWProgrammingScreen::RefreshCommands(int32 PlayerIndex)
{
    FString Text;
    for (int32 Index = 0; Index < Commands[PlayerIndex].Num(); ++Index)
        Text += FString::Printf(TEXT("%3d | %s\n"), Index + 1, LexToString(Commands[PlayerIndex][Index]));
    if (Text.IsEmpty())
        Text = TEXT("NO ACTIONS SELECTED");

    UTextBlock *List = PlayerIndex == 0 ? ProgramP1Text.Get() : ProgramP2Text.Get();
    UButton *Remove = PlayerIndex == 0 ? RemoveActionP1Button.Get() : RemoveActionP2Button.Get();
    if (List)
        List->SetText(FText::FromString(Text));
    if (Remove)
        Remove->SetVisibility(Commands[PlayerIndex].IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

void UAWProgrammingScreen::OnBack() { BroadcastAction(EAWUIAction::BackToMainMenu); }
void UAWProgrammingScreen::OnSubmitP1() { BroadcastAction(EAWUIAction::SubmitP1); }
void UAWProgrammingScreen::OnSubmitP2() { BroadcastAction(EAWUIAction::SubmitP2); }
void UAWProgrammingScreen::OnMoveP1() { AddCommand(0, EAWCommand::Move); }
void UAWProgrammingScreen::OnFireP1() { AddCommand(0, EAWCommand::Fire); }
void UAWProgrammingScreen::OnTurnLeftP1() { AddCommand(0, EAWCommand::TurnLeft); }
void UAWProgrammingScreen::OnTurnRightP1() { AddCommand(0, EAWCommand::TurnRight); }
void UAWProgrammingScreen::OnRemoveP1() { RemoveLastCommand(0); }
void UAWProgrammingScreen::OnMoveP2() { AddCommand(1, EAWCommand::Move); }
void UAWProgrammingScreen::OnFireP2() { AddCommand(1, EAWCommand::Fire); }
void UAWProgrammingScreen::OnTurnLeftP2() { AddCommand(1, EAWCommand::TurnLeft); }
void UAWProgrammingScreen::OnTurnRightP2() { AddCommand(1, EAWCommand::TurnRight); }
void UAWProgrammingScreen::OnRemoveP2() { RemoveLastCommand(1); }

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
    if (ReplayScrubSlider)
        ReplayScrubSlider->OnValueChanged.AddUniqueDynamic(this, &UAWReplayAutopsyScreen::OnScrubChanged);
}

void UAWReplayAutopsyScreen::SetTimeline(float Speed, float NormalizedPosition)
{
    SetSpeed(Speed);
    if (ReplayScrubSlider)
    {
        TGuardValue<bool> Guard(bUpdatingSlider, true);
        ReplayScrubSlider->SetValue(NormalizedPosition);
    }
}

void UAWReplayAutopsyScreen::SetSpeed(float Speed)
{
    if (ReplaySpeedText)
        ReplaySpeedText->SetText(FText::FromString(FString::Printf(TEXT("Speed: %.2fx"), Speed)));
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

void UAWReplayAutopsyScreen::SetEventLog(const FString &EventLog)
{
    if (ReplayEventLog)
        ReplayEventLog->SetText(FText::FromString(EventLog));
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

void UAWReplayAutopsyScreen::OnScrubChanged(float Value)
{
    if (!bUpdatingSlider)
        OnScrubbed.Broadcast(Value);
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
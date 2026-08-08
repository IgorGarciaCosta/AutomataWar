#include "AWScreenWidget.h"
#include "AWCodeEditorWidget.h"
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
    BIND_SCREEN_BUTTON("AggressorP1Button", UAWProgrammingScreen, OnAggressorP1);
    BIND_SCREEN_BUTTON("AggressorP2Button", UAWProgrammingScreen, OnAggressorP2);
    BIND_SCREEN_BUTTON("CamperP1Button", UAWProgrammingScreen, OnCamperP1);
    BIND_SCREEN_BUTTON("CamperP2Button", UAWProgrammingScreen, OnCamperP2);
    BIND_SCREEN_BUTTON("KiterP1Button", UAWProgrammingScreen, OnKiterP1);
    BIND_SCREEN_BUTTON("KiterP2Button", UAWProgrammingScreen, OnKiterP2);
    BIND_SCREEN_BUTTON("TrainingP1Button", UAWProgrammingScreen, OnTrainingP1);
    BIND_SCREEN_BUTTON("TrainingP2Button", UAWProgrammingScreen, OnTrainingP2);
}

FString UAWProgrammingScreen::GetSource(int32 PlayerIndex) const
{
    const UAWCodeEditorWidget *Editor = PlayerIndex == 0 ? EditorP1.Get() : EditorP2.Get();
    return Editor ? Editor->GetSourceText() : FString();
}

void UAWProgrammingScreen::SetSource(int32 PlayerIndex, const FString &Source)
{
    UAWCodeEditorWidget *Editor = PlayerIndex == 0 ? EditorP1.Get() : EditorP2.Get();
    if (Editor)
        Editor->SetSourceText(Source);
}

void UAWProgrammingScreen::OnBack() { BroadcastAction(EAWUIAction::BackToMainMenu); }
void UAWProgrammingScreen::OnSubmitP1() { BroadcastAction(EAWUIAction::SubmitP1); }
void UAWProgrammingScreen::OnSubmitP2() { BroadcastAction(EAWUIAction::SubmitP2); }
void UAWProgrammingScreen::OnAggressorP1() { BroadcastAction(EAWUIAction::AggressorP1); }
void UAWProgrammingScreen::OnAggressorP2() { BroadcastAction(EAWUIAction::AggressorP2); }
void UAWProgrammingScreen::OnCamperP1() { BroadcastAction(EAWUIAction::CamperP1); }
void UAWProgrammingScreen::OnCamperP2() { BroadcastAction(EAWUIAction::CamperP2); }
void UAWProgrammingScreen::OnKiterP1() { BroadcastAction(EAWUIAction::KiterP1); }
void UAWProgrammingScreen::OnKiterP2() { BroadcastAction(EAWUIAction::KiterP2); }
void UAWProgrammingScreen::OnTrainingP1() { BroadcastAction(EAWUIAction::TrainingP1); }
void UAWProgrammingScreen::OnTrainingP2() { BroadcastAction(EAWUIAction::TrainingP2); }

void UAWSimulationScreen::SetSources(const FString &PlayerOneSource, const FString &PlayerTwoSource)
{
    if (SimulationSourceP1Text)
        SimulationSourceP1Text->SetText(FText::FromString(PlayerOneSource));
    if (SimulationSourceP2Text)
        SimulationSourceP2Text->SetText(FText::FromString(PlayerTwoSource));
}

void UAWReplayAutopsyScreen::NativeConstruct()
{
    Super::NativeConstruct();
    BIND_SCREEN_BUTTON("ReplayStartButton", UAWReplayAutopsyScreen, OnStart);
    BIND_SCREEN_BUTTON("ReplayBackButton", UAWReplayAutopsyScreen, OnBack);
    BIND_SCREEN_BUTTON("ReplayPauseButton", UAWReplayAutopsyScreen, OnPause);
    BIND_SCREEN_BUTTON("ReplayPlayButton", UAWReplayAutopsyScreen, OnPlay);
    BIND_SCREEN_BUTTON("ReplayStepButton", UAWReplayAutopsyScreen, OnStep);
    BIND_SCREEN_BUTTON("ReplayStepP1Button", UAWReplayAutopsyScreen, OnStepP1);
    BIND_SCREEN_BUTTON("ReplayStepP2Button", UAWReplayAutopsyScreen, OnStepP2);
    BIND_SCREEN_BUTTON("ReplayQuarterButton", UAWReplayAutopsyScreen, OnSpeedQuarter);
    BIND_SCREEN_BUTTON("ReplayNormalButton", UAWReplayAutopsyScreen, OnSpeedNormal);
    BIND_SCREEN_BUTTON("ReplayDoubleButton", UAWReplayAutopsyScreen, OnSpeedDouble);
    BIND_SCREEN_BUTTON("ReplayQuadButton", UAWReplayAutopsyScreen, OnSpeedQuadruple);
    BIND_SCREEN_BUTTON("ReplayBackToMenuButton", UAWReplayAutopsyScreen, OnBackToMenu);
    BIND_SCREEN_BUTTON("NextRoundButton", UAWReplayAutopsyScreen, OnNextRound);
    if (ReplayScrubSlider)
        ReplayScrubSlider->OnValueChanged.AddUniqueDynamic(this, &UAWReplayAutopsyScreen::OnScrubChanged);
}

void UAWReplayAutopsyScreen::SetOutcome(const FString &Outcome)
{
    if (ReplayOutcomeText)
        ReplayOutcomeText->SetText(FText::FromString(Outcome));
}

void UAWReplayAutopsyScreen::SetTimeline(int32 CurrentTick, int32 TotalTicks, float Speed, float NormalizedPosition)
{
    if (ReplayTickText)
        ReplayTickText->SetText(FText::FromString(FString::Printf(TEXT("Tick: %d/%d"), CurrentTick, TotalTicks - 1)));
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

void UAWReplayAutopsyScreen::SetCombatantData(int32 PlayerIndex, const FString &Source, const FString &Registers)
{
    UTextBlock *SourceText = PlayerIndex == 0 ? ReplaySourceAText.Get() : ReplaySourceBText.Get();
    UTextBlock *RegisterText = PlayerIndex == 0 ? ReplayRegistersP1.Get() : ReplayRegistersP2.Get();
    if (SourceText)
        SourceText->SetText(FText::FromString(Source));
    if (RegisterText)
        RegisterText->SetText(FText::FromString(Registers));
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
void UAWReplayAutopsyScreen::OnStepP1() { BroadcastAction(EAWUIAction::ReplayStepP1); }
void UAWReplayAutopsyScreen::OnStepP2() { BroadcastAction(EAWUIAction::ReplayStepP2); }
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
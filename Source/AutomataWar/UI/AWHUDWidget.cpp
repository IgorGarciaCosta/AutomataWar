#include "AWHUDWidget.h"
#include "SAWCodeEditor.h"
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
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
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
    ShowScreen(EAWScreen::MainMenu);

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
        const double TickInterval = 1.0 / 30.0;
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

TSharedRef<SWidget> UAWHUDWidget::RebuildWidget()
{
    TSharedRef<SWidgetSwitcher> Switcher = SAssignNew(ScreenSwitcher, SWidgetSwitcher) + SWidgetSwitcher::Slot()[BuildMainMenu()] + SWidgetSwitcher::Slot()[BuildProgrammingScreen()] + SWidgetSwitcher::Slot()[BuildSimulationScreen()] + SWidgetSwitcher::Slot()[BuildReplayAutopsyScreen()] + SWidgetSwitcher::Slot()[BuildReplayBrowser()] + SWidgetSwitcher::Slot()[BuildLanguageReference()];

    return SNew(SBorder)
        .BorderBackgroundColor(AWUIColors::Background)
        .Padding(12.f)
            [SNew(SVerticalBox) + SVerticalBox::Slot().FillHeight(1.f)[Switcher] + SVerticalBox::Slot().AutoHeight().Padding(4.f)[SAssignNew(StatusText, STextBlock).Text(FText::GetEmpty()).ColorAndOpacity(FSlateColor(AWUIColors::TextSecondary))]];
}

int32 UAWHUDWidget::GetScreenCount() const
{
    return ScreenSwitcher.IsValid() ? ScreenSwitcher->GetNumWidgets() : 0;
}

int32 UAWHUDWidget::GetActiveScreenIndex() const
{
    return ScreenSwitcher.IsValid() ? ScreenSwitcher->GetActiveWidgetIndex() : -1;
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
    if (ScreenSwitcher.IsValid())
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
    if (StatusText.IsValid())
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
// Main Menu
// ═══════════════════════════════════════════════════════════════════════════════

TSharedRef<SWidget> UAWHUDWidget::BuildMainMenu()
{
    return SNew(SVerticalBox) + SVerticalBox::Slot().AutoHeight().Padding(20)[SNew(STextBlock).Text(FText::FromString(TEXT("AUTOMATA WAR"))).Font(AWUIFonts::Display(32)).ColorAndOpacity(FSlateColor(AWUIColors::AccentCyan))] + SVerticalBox::Slot().AutoHeight().Padding(10, 5)[SNew(SButton).Text(FText::FromString(TEXT("Local Match"))).OnClicked_Lambda([this]()
                                                                                                                                                                                                                                                                                                                                                               { OnLocalMatch(); return FReply::Handled(); })] +
           SVerticalBox::Slot().AutoHeight().Padding(10, 5)
               [SNew(SButton).Text(FText::FromString(TEXT("Host LAN"))).OnClicked_Lambda([this]()
                                                                                         { OnHostLAN(); return FReply::Handled(); })] +
           SVerticalBox::Slot().AutoHeight().Padding(10, 5)
               [SNew(SButton).Text(FText::FromString(TEXT("Find LAN"))).OnClicked_Lambda([this]()
                                                                                         { OnFindLAN(); return FReply::Handled(); })] +
           SVerticalBox::Slot().AutoHeight().Padding(10, 2)
               [SAssignNew(SessionListBox, SVerticalBox)] +
           SVerticalBox::Slot().AutoHeight().Padding(10, 5)
               [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1.f)[SAssignNew(JoinIPField, SEditableTextBox).HintText(FText::FromString(TEXT("IP Address...")))] + SHorizontalBox::Slot().AutoWidth().Padding(4, 0, 0, 0)[SNew(SButton).Text(FText::FromString(TEXT("Join IP"))).OnClicked_Lambda([this]()
                                                                                                                                                                                                                                                                                                            { OnJoinIP(); return FReply::Handled(); })]] +
           SVerticalBox::Slot().AutoHeight().Padding(10, 5)
               [SNew(SButton).Text(FText::FromString(TEXT("Replay Browser"))).OnClicked_Lambda([this]()
                                                                                               { OnReplayBrowserNav(); return FReply::Handled(); })] +
           SVerticalBox::Slot().AutoHeight().Padding(10, 5)
               [SNew(SButton).Text(FText::FromString(TEXT("Language Reference"))).OnClicked_Lambda([this]()
                                                                                                   { OnLanguageRef(); return FReply::Handled(); })] +
           SVerticalBox::Slot().AutoHeight().Padding(10, 5)
               [SNew(SButton).Text(FText::FromString(TEXT("Quit"))).OnClicked_Lambda([this]()
                                                                                     { OnQuit(); return FReply::Handled(); })];
}

// ═══════════════════════════════════════════════════════════════════════════════
// Programming
// ═══════════════════════════════════════════════════════════════════════════════

TSharedRef<SWidget> UAWHUDWidget::BuildProgrammingScreen()
{
    return SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1.f).Padding(4)[SNew(SVerticalBox) + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("PLAYER 1"))).ColorAndOpacity(FSlateColor(AWUIColors::AccentCyan))] + SVerticalBox::Slot().FillHeight(1.f)[SAssignNew(EditorP1, SAWCodeEditor).InitialText(FText::FromString(FAWExampleScripts::DefaultBot()))] + SVerticalBox::Slot().AutoHeight().Padding(0, 4)[SNew(SHorizontalBox) + SHorizontalBox::Slot().AutoWidth().Padding(2, 0)[SNew(SButton).Text(FText::FromString(TEXT("Submit"))).OnClicked_Lambda([this]()
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        { OnSubmitSlot(0); return FReply::Handled(); })] +
                                                                                                                                                                                                                                                                                                                                                                                                                                                         SHorizontalBox::Slot().AutoWidth().Padding(2, 0)[SNew(SButton).Text(FText::FromString(TEXT("Aggressor"))).OnClicked_Lambda([this]()
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    { OnLoadExample(0, TEXT("Aggressor")); return FReply::Handled(); })] +
                                                                                                                                                                                                                                                                                                                                                                                                                                                         SHorizontalBox::Slot().AutoWidth().Padding(2, 0)[SNew(SButton).Text(FText::FromString(TEXT("Camper"))).OnClicked_Lambda([this]()
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 { OnLoadExample(0, TEXT("Camper")); return FReply::Handled(); })] +
                                                                                                                                                                                                                                                                                                                                                                                                                                                         SHorizontalBox::Slot().AutoWidth().Padding(2, 0)[SNew(SButton).Text(FText::FromString(TEXT("Kiter"))).OnClicked_Lambda([this]()
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                { OnLoadExample(0, TEXT("Kiter")); return FReply::Handled(); })] +
                                                                                                                                                                                                                                                                                                                                                                                                                                                         SHorizontalBox::Slot().AutoWidth().Padding(2, 0)[SNew(SButton).Text(FText::FromString(TEXT("Train"))).OnClicked_Lambda([this]()
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                { OnTrainingBot(0); return FReply::Handled(); })]]] +
           SHorizontalBox::Slot().AutoWidth()[SNew(SSeparator).Orientation(Orient_Vertical)] + SHorizontalBox::Slot().FillWidth(1.f).Padding(4)[SNew(SVerticalBox) + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("PLAYER 2"))).ColorAndOpacity(FSlateColor(AWUIColors::AccentCoral))] + SVerticalBox::Slot().FillHeight(1.f)[SAssignNew(EditorP2, SAWCodeEditor).InitialText(FText::FromString(FAWExampleScripts::DefaultBot()))] + SVerticalBox::Slot().AutoHeight().Padding(0, 4)[SNew(SHorizontalBox) + SHorizontalBox::Slot().AutoWidth().Padding(2, 0)[SNew(SButton).Text(FText::FromString(TEXT("Submit"))).OnClicked_Lambda([this]()
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      { OnSubmitSlot(1); return FReply::Handled(); })] +
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       SHorizontalBox::Slot().AutoWidth().Padding(2, 0)[SNew(SButton).Text(FText::FromString(TEXT("Aggressor"))).OnClicked_Lambda([this]()
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  { OnLoadExample(1, TEXT("Aggressor")); return FReply::Handled(); })] +
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       SHorizontalBox::Slot().AutoWidth().Padding(2, 0)[SNew(SButton).Text(FText::FromString(TEXT("Camper"))).OnClicked_Lambda([this]()
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               { OnLoadExample(1, TEXT("Camper")); return FReply::Handled(); })] +
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       SHorizontalBox::Slot().AutoWidth().Padding(2, 0)[SNew(SButton).Text(FText::FromString(TEXT("Kiter"))).OnClicked_Lambda([this]()
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              { OnLoadExample(1, TEXT("Kiter")); return FReply::Handled(); })] +
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       SHorizontalBox::Slot().AutoWidth().Padding(2, 0)[SNew(SButton).Text(FText::FromString(TEXT("Train"))).OnClicked_Lambda([this]()
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              { OnTrainingBot(1); return FReply::Handled(); })]]];
}

TSharedRef<SWidget> UAWHUDWidget::BuildSimulationScreen()
{
    return SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(TEXT("SIMULATING..."))).Font(AWUIFonts::Display(24)).ColorAndOpacity(FSlateColor(AWUIColors::AccentCyan))];
}

// ═══════════════════════════════════════════════════════════════════════════════
// Replay Autopsy
// ═══════════════════════════════════════════════════════════════════════════════

TSharedRef<SWidget> UAWHUDWidget::BuildReplayAutopsyScreen()
{
    return SNew(SVerticalBox) + SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(SHorizontalBox) + SHorizontalBox::Slot().AutoWidth().Padding(2, 0)[SNew(SButton).Text(FText::FromString(TEXT("|<"))).OnClicked_Lambda([this]()
                                                                                                                                                                                                                        { OnReplayScrub(0); return FReply::Handled(); })] +
                                                                             SHorizontalBox::Slot().AutoWidth().Padding(2, 0)[SNew(SButton).Text(FText::FromString(TEXT("<"))).OnClicked_Lambda([this]()
                                                                                                                                                                                                { OnReplayStepBack(); return FReply::Handled(); })] +
                                                                             SHorizontalBox::Slot().AutoWidth().Padding(2, 0)[SNew(SButton).Text(FText::FromString(TEXT("||"))).OnClicked_Lambda([this]()
                                                                                                                                                                                                 { OnReplayPause(); return FReply::Handled(); })] +
                                                                             SHorizontalBox::Slot().AutoWidth().Padding(2, 0)[SNew(SButton).Text(FText::FromString(TEXT(">"))).OnClicked_Lambda([this]()
                                                                                                                                                                                                { OnReplayPlay(); return FReply::Handled(); })] +
                                                                             SHorizontalBox::Slot().AutoWidth().Padding(2, 0)[SNew(SButton).Text(FText::FromString(TEXT(">|"))).OnClicked_Lambda([this]()
                                                                                                                                                                                                 { OnReplayStepTick(); return FReply::Handled(); })] +
                                                                             SHorizontalBox::Slot().AutoWidth().Padding(2, 0)[SNew(SButton).Text(FText::FromString(TEXT("P1 >|"))).OnClicked_Lambda([this]()
                                                                                                                                                                                                    { OnReplayStepInstruction(0); return FReply::Handled(); })] +
                                                                             SHorizontalBox::Slot().AutoWidth().Padding(2, 0)[SNew(SButton).Text(FText::FromString(TEXT("P2 >|"))).OnClicked_Lambda([this]()
                                                                                                                                                                                                    { OnReplayStepInstruction(1); return FReply::Handled(); })] +
                                                                             SHorizontalBox::Slot().AutoWidth().Padding(8, 0)[SNew(SButton).Text(FText::FromString(TEXT(".25x"))).OnClicked_Lambda([this]()
                                                                                                                                                                                                   { OnReplaySetSpeed(0.25f); return FReply::Handled(); })] +
                                                                             SHorizontalBox::Slot().AutoWidth().Padding(2, 0)[SNew(SButton).Text(FText::FromString(TEXT("1x"))).OnClicked_Lambda([this]()
                                                                                                                                                                                                 { OnReplaySetSpeed(1.f); return FReply::Handled(); })] +
                                                                             SHorizontalBox::Slot().AutoWidth().Padding(2, 0)[SNew(SButton).Text(FText::FromString(TEXT("2x"))).OnClicked_Lambda([this]()
                                                                                                                                                                                                 { OnReplaySetSpeed(2.f); return FReply::Handled(); })] +
                                                                             SHorizontalBox::Slot().AutoWidth().Padding(2, 0)[SNew(SButton).Text(FText::FromString(TEXT("4x"))).OnClicked_Lambda([this]()
                                                                                                                                                                                                 { OnReplaySetSpeed(4.f); return FReply::Handled(); })] +
                                                                             SHorizontalBox::Slot().FillWidth(1.f).Padding(8, 0)[SAssignNew(ReplayScrubSlider, SSlider).OnValueChanged_Lambda([this](float Val)
                                                                                                                                                                                              {
					if (ReplayController.IsValid() && ReplayController->IsValid())
					{
						int32 Tick = FMath::RoundToInt32(Val * (ReplayController->GetTotalTicks() - 1));
						OnReplayScrub(Tick);
					} })]] +
           SVerticalBox::Slot().AutoHeight().Padding(8, 2)
               [SNew(SHorizontalBox) + SHorizontalBox::Slot().AutoWidth()[SAssignNew(ReplayTickText, STextBlock).Text(FText::FromString(TEXT("Tick: 0/0"))).ColorAndOpacity(FSlateColor(AWUIColors::TextPrimary))] + SHorizontalBox::Slot().AutoWidth().Padding(16, 0, 0, 0)[SAssignNew(ReplaySpeedText, STextBlock).Text(FText::FromString(TEXT("Speed: 1x"))).ColorAndOpacity(FSlateColor(AWUIColors::TextSecondary))] + SHorizontalBox::Slot().FillWidth(1.f).Padding(16, 0, 0, 0)[SAssignNew(ReplayOutcomeText, STextBlock).Text(FText::GetEmpty()).ColorAndOpacity(FSlateColor(AWUIColors::AccentCyan))]] +
           SVerticalBox::Slot().FillHeight(1.f).Padding(8, 4)
               [SNew(SHorizontalBox)
                // Left panel: P1 source + registers
                + SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 4, 0)
                      [SNew(SVerticalBox) + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("P1 Source"))).ColorAndOpacity(FSlateColor(AWUIColors::AccentCyan))] + SVerticalBox::Slot().FillHeight(1.f)[SNew(SScrollBox) + SScrollBox::Slot()[SAssignNew(ReplaySourceAText, STextBlock).Font(AWUIFonts::Mono(10)).ColorAndOpacity(FSlateColor(AWUIColors::TextPrimary))]] + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)[SAssignNew(ReplayRegistersP1, STextBlock).Font(AWUIFonts::Mono(10)).ColorAndOpacity(FSlateColor(AWUIColors::AccentCyan))]]
                // Right panel: P2 source + registers
                + SHorizontalBox::Slot().FillWidth(1.f).Padding(4, 0, 0, 0)
                      [SNew(SVerticalBox) + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("P2 Source"))).ColorAndOpacity(FSlateColor(AWUIColors::AccentCoral))] + SVerticalBox::Slot().FillHeight(1.f)[SNew(SScrollBox) + SScrollBox::Slot()[SAssignNew(ReplaySourceBText, STextBlock).Font(AWUIFonts::Mono(10)).ColorAndOpacity(FSlateColor(AWUIColors::TextPrimary))]] + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)[SAssignNew(ReplayRegistersP2, STextBlock).Font(AWUIFonts::Mono(10)).ColorAndOpacity(FSlateColor(AWUIColors::AccentCoral))]]] +
           SVerticalBox::Slot().AutoHeight().MaxHeight(120.f).Padding(8, 2)
               [SNew(SScrollBox) + SScrollBox::Slot()[SAssignNew(ReplayEventLog, STextBlock).Font(AWUIFonts::Mono(9)).ColorAndOpacity(FSlateColor(AWUIColors::TextSecondary))]] +
           SVerticalBox::Slot().AutoHeight().Padding(8, 4)
               [SNew(SButton).Text(FText::FromString(TEXT("Next Round"))).OnClicked_Lambda([this]()
                                                                                           { OnNextRound(); return FReply::Handled(); })];
}

// ═══════════════════════════════════════════════════════════════════════════════
// Replay Browser
// ═══════════════════════════════════════════════════════════════════════════════

TSharedRef<SWidget> UAWHUDWidget::BuildReplayBrowser()
{
    return SNew(SVerticalBox) + SVerticalBox::Slot().AutoHeight().Padding(10)[SNew(STextBlock).Text(FText::FromString(TEXT("REPLAY BROWSER"))).Font(AWUIFonts::Display(18)).ColorAndOpacity(FSlateColor(AWUIColors::TextPrimary))] + SVerticalBox::Slot().AutoHeight().Padding(10, 4)[SNew(SHorizontalBox) + SHorizontalBox::Slot().AutoWidth().Padding(2, 0)[SNew(SButton).Text(FText::FromString(TEXT("< Back"))).OnClicked_Lambda([this]()
                                                                                                                                                                                                                                                                                                                                                                                                                                     { ShowScreen(EAWScreen::MainMenu); return FReply::Handled(); })] +
                                                                                                                                                                                                                                                                                      SHorizontalBox::Slot().AutoWidth().Padding(2, 0)[SNew(SButton).Text(FText::FromString(TEXT("Refresh"))).OnClicked_Lambda([this]()
                                                                                                                                                                                                                                                                                                                                                                                                               { RefreshReplayList(); return FReply::Handled(); })] +
                                                                                                                                                                                                                                                                                      SHorizontalBox::Slot().AutoWidth().Padding(2, 0)[SNew(SButton).Text(FText::FromString(TEXT("Save Current"))).OnClicked_Lambda([this]()
                                                                                                                                                                                                                                                                                                                                                                                                                    { OnReplaySave(); return FReply::Handled(); })]] +
           SVerticalBox::Slot().FillHeight(1.f).Padding(10, 4)
               [SNew(SScrollBox) + SScrollBox::Slot()[SAssignNew(ReplayListBox, SVerticalBox)]] +
           SVerticalBox::Slot().AutoHeight().Padding(10, 4)
               [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1.f)
                                           [SAssignNew(ExportField, SEditableTextBox).IsReadOnly(true).HintText(FText::FromString(TEXT("Base64 export appears here...")))]] +
           SVerticalBox::Slot().AutoHeight().Padding(10, 4)
               [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1.f)[SAssignNew(ImportField, SEditableTextBox).HintText(FText::FromString(TEXT("Paste base64 replay here...")))] + SHorizontalBox::Slot().AutoWidth().Padding(4, 0, 0, 0)[SNew(SButton).Text(FText::FromString(TEXT("Import"))).OnClicked_Lambda([this]()
                                                                                                                                                                                                                                                                                                                         { OnReplayImport(); return FReply::Handled(); })]] +
           SVerticalBox::Slot().AutoHeight().Padding(10, 2)
               [SAssignNew(ReplayBrowserStatus, STextBlock).ColorAndOpacity(FSlateColor(AWUIColors::TextSecondary))];
}

// ═══════════════════════════════════════════════════════════════════════════════
// Language Reference (generated from Core definitions)
// ═══════════════════════════════════════════════════════════════════════════════

TSharedRef<SWidget> UAWHUDWidget::BuildLanguageReference()
{
    TSharedRef<SVerticalBox> Content = SNew(SVerticalBox) + SVerticalBox::Slot().AutoHeight().Padding(10)[SNew(STextBlock).Text(FText::FromString(TEXT("LANGUAGE REFERENCE"))).Font(AWUIFonts::Display(18)).ColorAndOpacity(FSlateColor(AWUIColors::TextPrimary))] + SVerticalBox::Slot().AutoHeight().Padding(10, 2)[SNew(STextBlock).Text(FText::FromString(TEXT("Instructions (8 total):"))).ColorAndOpacity(FSlateColor(AWUIColors::TextSecondary))];

    for (const Automata::InstructionDefinition &Definition : Automata::InstructionDefs)
    {
        FString Line = FString::Printf(TEXT("  %-34s E:%d T:%d  %s"),
                                       UTF8_TO_TCHAR(Definition.syntax), Definition.energyCost, Definition.tickCost,
                                       UTF8_TO_TCHAR(Definition.description));
        Content->AddSlot().AutoHeight().Padding(10, 1)
            [SNew(STextBlock).Text(FText::FromString(Line)).Font(AWUIFonts::Mono(11)).ColorAndOpacity(FSlateColor(AWUIColors::SyntaxInstruction))];
    }

    Content->AddSlot().AutoHeight().Padding(10, 8)
        [SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("Registers (%d total):"), Automata::TotalRegisterCount))).ColorAndOpacity(FSlateColor(AWUIColors::TextSecondary))];

    for (const Automata::RegisterDefinition &Definition : Automata::RegisterDefs)
    {
        FString Line = FString::Printf(TEXT("  %-14s  %s%s"), UTF8_TO_TCHAR(Definition.name),
                                       UTF8_TO_TCHAR(Definition.description), Definition.readOnly ? TEXT(" (read-only)") : TEXT(""));
        Content->AddSlot().AutoHeight().Padding(10, 1)
            [SNew(STextBlock).Text(FText::FromString(Line)).Font(AWUIFonts::Mono(11)).ColorAndOpacity(FSlateColor(AWUIColors::SyntaxRegister))];
    }

    Content->AddSlot().AutoHeight().Padding(10, 12)
        [SNew(SButton).Text(FText::FromString(TEXT("< Back"))).OnClicked_Lambda([this]()
                                                                                { ShowScreen(EAWScreen::MainMenu); return FReply::Handled(); })];

    return Content;
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
    if (!SessionListBox.IsValid())
        return;
    SessionListBox->ClearChildren();
    UAWGameSubsystem *Sub = GetSubsystem();
    if (!Sub)
        return;

    const TArray<FAWSessionInfo> Sessions = Sub->GetSessionList();
    for (int32 Index = 0; Index < Sessions.Num(); ++Index)
    {
        const FAWSessionInfo &Session = Sessions[Index];
        const FString Label = FString::Printf(TEXT("%s | %s | %d/%d | %d ms"), *Session.SessionName,
                                              *Session.HostName, Session.CurrentPlayers, Session.MaxPlayers, Session.PingMs);
        SessionListBox->AddSlot().AutoHeight().Padding(2.f)
            [SNew(SButton).Text(FText::FromString(Label)).OnClicked_Lambda([this, Index]()
                                                                           {
				if (UAWGameSubsystem* GameSubsystem = GetSubsystem()) GameSubsystem->JoinSessionByIndex(Index);
				SetStatus(TEXT("Joining LAN session..."));
				return FReply::Handled(); })];
    }
    if (Sessions.IsEmpty())
        SetStatus(TEXT("No LAN sessions found."), true);
}

void UAWHUDWidget::OnJoinIP()
{
    if (!JoinIPField.IsValid())
        return;
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
        if (SlotIndex == 0 && EditorP1.IsValid())
            Source = EditorP1->GetSourceText();
        else if (SlotIndex == 1 && EditorP2.IsValid())
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

    if (SlotIndex == 0 && EditorP1.IsValid())
        EditorP1->SetSourceText(Source);
    else if (SlotIndex == 1 && EditorP2.IsValid())
        EditorP2->SetSourceText(Source);
}

void UAWHUDWidget::OnTrainingBot(int32 SlotIndex)
{
    if (UAWGameSubsystem *Sub = GetSubsystem())
    {
        FString Source;
        if (SlotIndex == 0 && EditorP1.IsValid())
            Source = EditorP1->GetSourceText();
        else if (SlotIndex == 1 && EditorP2.IsValid())
            Source = EditorP2->GetSourceText();

        int32 Wins, Losses, Draws;
        Sub->RunTraining(Source, 10, Wins, Losses, Draws);
        SetStatus(FString::Printf(TEXT("Training: W%d L%d D%d"), Wins, Losses, Draws));
    }
}

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
    bReplayPlaying = false;
    ReplayAccumulator = 0.0;
    ReplaySpeed = 1.f;

    UWorld *World = GetWorld();
    if (!World)
        return;
    AAWGameState *GS = World->GetGameState<AAWGameState>();
    if (!GS)
        return;

    std::string SrcA = TCHAR_TO_UTF8(*GS->RevealedSource0);
    std::string SrcB = TCHAR_TO_UTF8(*GS->RevealedSource1);
    uint64_t Seed = static_cast<uint64_t>(GS->SimSeed);

    ReplayController = MakeUnique<Automata::FAWReplayController>();
    if (!ReplayController->Initialize(SrcA, SrcB, Seed))
    {
        SetStatus(TEXT("Failed to reconstruct simulation for replay."), true);
        return;
    }

    // Initialize arena renderer
    if (AAWArenaRenderer *Renderer = FindOrSpawnRenderer())
    {
        TArray<Automata::CellType> Grid;
        Grid.Reserve(ReplayController->GetGrid().size());
        for (auto c : ReplayController->GetGrid())
            Grid.Add(c);
        Renderer->InitializeArena(ReplayController->GetConfig(), Grid);
    }

    // Display sources with line numbers
    if (ReplaySourceAText.IsValid())
    {
        FString Src = GS->RevealedSource0;
        TArray<FString> Lines;
        Src.ParseIntoArrayLines(Lines);
        FString Numbered;
        for (int32 i = 0; i < Lines.Num(); ++i)
        {
            Numbered += FString::Printf(TEXT("%3d| %s\n"), i + 1, *Lines[i]);
        }
        ReplaySourceAText->SetText(FText::FromString(Numbered));
    }
    if (ReplaySourceBText.IsValid())
    {
        FString Src = GS->RevealedSource1;
        TArray<FString> Lines;
        Src.ParseIntoArrayLines(Lines);
        FString Numbered;
        for (int32 i = 0; i < Lines.Num(); ++i)
        {
            Numbered += FString::Printf(TEXT("%3d| %s\n"), i + 1, *Lines[i]);
        }
        ReplaySourceBText->SetText(FText::FromString(Numbered));
    }

    // Display outcome
    if (ReplayOutcomeText.IsValid())
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
}

void UAWHUDWidget::UpdateReplayUI()
{
    if (!ReplayController.IsValid() || !ReplayController->IsValid())
        return;

    int32 Tick = ReplayController->GetCurrentTick();
    int32 Total = ReplayController->GetTotalTicks();

    if (ReplayTickText.IsValid())
    {
        ReplayTickText->SetText(FText::FromString(FString::Printf(TEXT("Tick: %d/%d"), Tick, Total - 1)));
    }
    if (ReplaySpeedText.IsValid())
    {
        ReplaySpeedText->SetText(FText::FromString(FString::Printf(TEXT("Speed: %.2fx"), ReplaySpeed)));
    }
    if (ReplayScrubSlider.IsValid() && Total > 1)
    {
        ReplayScrubSlider->SetValue(static_cast<float>(Tick) / static_cast<float>(Total - 1));
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
    if (ReplayRegistersP1.IsValid())
        ReplayRegistersP1->SetText(FText::FromString(FormatRegs(0)));
    if (ReplayRegistersP2.IsValid())
        ReplayRegistersP2->SetText(FText::FromString(FormatRegs(1)));

    if (ReplaySourceAText.IsValid())
        ReplaySourceAText->SetText(FText::FromString(FormatReplaySource(ReplayController->GetSourceA(), ReplayController->GetProgramA(), Snap.robots[0].vm.currentInstruction)));
    if (ReplaySourceBText.IsValid())
        ReplaySourceBText->SetText(FText::FromString(FormatReplaySource(ReplayController->GetSourceB(), ReplayController->GetProgramB(), Snap.robots[1].vm.currentInstruction)));

    if (ReplayEventLog.IsValid())
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
    if (ReplaySpeedText.IsValid())
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

// ═══════════════════════════════════════════════════════════════════════════════
// Replay Browser Actions
// ═══════════════════════════════════════════════════════════════════════════════

void UAWHUDWidget::RefreshReplayList()
{
    if (!ReplayListBox.IsValid())
        return;
    ReplayListBox->ClearChildren();

    UAWGameSubsystem *Sub = GetSubsystem();
    if (!Sub)
        return;

    TArray<FAWReplayInfo> Replays = Sub->GetReplayList();
    for (const FAWReplayInfo &Info : Replays)
    {
        FString Label = FString::Printf(TEXT("%s (%d bytes)"), *Info.Filename, Info.FileSizeBytes);
        FString Filename = Info.Filename;
        ReplayListBox->AddSlot().AutoHeight().Padding(2)
            [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1.f)[SNew(STextBlock).Text(FText::FromString(Label)).ColorAndOpacity(FSlateColor(AWUIColors::TextPrimary))] + SHorizontalBox::Slot().AutoWidth().Padding(4, 0)[SNew(SButton).Text(FText::FromString(TEXT("Load"))).OnClicked_Lambda([this, Filename]()
                                                                                                                                                                                                                                                                                                         { OnReplayLoad(Filename); return FReply::Handled(); })] +
             SHorizontalBox::Slot().AutoWidth().Padding(2, 0)[SNew(SButton).Text(FText::FromString(TEXT("Export"))).OnClicked_Lambda([this, Filename]()
                                                                                                                                     { OnReplayExport(Filename); return FReply::Handled(); })]];
    }

    if (Replays.Num() == 0)
    {
        ReplayListBox->AddSlot().AutoHeight()
            [SNew(STextBlock).Text(FText::FromString(TEXT("No replays found in Saved/Replays/"))).ColorAndOpacity(FSlateColor(AWUIColors::TextSecondary))];
    }
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
        if (ReplayBrowserStatus.IsValid())
            ReplayBrowserStatus->SetText(FText::FromString(Error));
        return;
    }

    // Initialize replay controller directly from loaded data
    ReplayController = MakeUnique<Automata::FAWReplayController>();
    std::string SA = TCHAR_TO_UTF8(*Src0);
    std::string SB = TCHAR_TO_UTF8(*Src1);
    if (!ReplayController->Initialize(SA, SB, static_cast<uint64_t>(Seed)))
    {
        if (ReplayBrowserStatus.IsValid())
            ReplayBrowserStatus->SetText(FText::FromString(TEXT("Failed to resimulate replay.")));
        return;
    }

    // Set source display texts
    if (ReplaySourceAText.IsValid())
    {
        TArray<FString> Lines;
        Src0.ParseIntoArrayLines(Lines);
        FString N;
        for (int32 i = 0; i < Lines.Num(); ++i)
            N += FString::Printf(TEXT("%3d| %s\n"), i + 1, *Lines[i]);
        ReplaySourceAText->SetText(FText::FromString(N));
    }
    if (ReplaySourceBText.IsValid())
    {
        TArray<FString> Lines;
        Src1.ParseIntoArrayLines(Lines);
        FString N;
        for (int32 i = 0; i < Lines.Num(); ++i)
            N += FString::Printf(TEXT("%3d| %s\n"), i + 1, *Lines[i]);
        ReplaySourceBText->SetText(FText::FromString(N));
    }
    if (ReplayOutcomeText.IsValid())
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

    // Init arena
    if (AAWArenaRenderer *Renderer = FindOrSpawnRenderer())
    {
        TArray<Automata::CellType> Grid;
        for (auto c : ReplayController->GetGrid())
            Grid.Add(c);
        Renderer->InitializeArena(ReplayController->GetConfig(), Grid);
    }

    bReplayPlaying = false;
    ReplaySpeed = 1.f;
    UpdateReplayUI();
    UpdateArenaFromReplay();
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
        if (ReplayBrowserStatus.IsValid())
            ReplayBrowserStatus->SetText(FText::FromString(FString::Printf(TEXT("Saved: %s"), *Filename)));
        RefreshReplayList();
    }
    else
    {
        if (ReplayBrowserStatus.IsValid())
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
        if (ExportField.IsValid())
            ExportField->SetText(FText::FromString(Base64));
        if (ReplayBrowserStatus.IsValid())
            ReplayBrowserStatus->SetText(FText::FromString(TEXT("Exported to field above.")));
    }
    else
    {
        if (ReplayBrowserStatus.IsValid())
            ReplayBrowserStatus->SetText(FText::FromString(TEXT("Export failed.")));
    }
}

void UAWHUDWidget::OnReplayImport()
{
    if (!ImportField.IsValid())
        return;
    FString Base64 = ImportField->GetText().ToString().TrimStartAndEnd();
    if (Base64.IsEmpty())
    {
        if (ReplayBrowserStatus.IsValid())
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
        if (ReplayBrowserStatus.IsValid())
            ReplayBrowserStatus->SetText(FText::FromString(FString::Printf(TEXT("Imported: %s"), *Filename)));
        RefreshReplayList();
    }
    else
    {
        if (ReplayBrowserStatus.IsValid())
            ReplayBrowserStatus->SetText(FText::FromString(Error));
    }
}

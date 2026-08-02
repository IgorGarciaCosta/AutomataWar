#pragma once

/**
 * @file AWHUDWidget.h
 * @brief C++ Slate workflow for menus, programming, replay autopsy, browsing,
 *        and the generated language reference.
 */

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AutomataWar/Game/AWMatchTypes.h"
#include "AutomataWar/Core/Replay/AWReplayController.h"
#include "AWHUDWidget.generated.h"

class SAWCodeEditor;
class UAWGameSubsystem;
class AAWArenaRenderer;
class SEditableTextBox;
class SSlider;

/** Six stable Slate screen indices owned by the HUD widget switcher. */
UENUM()
enum class EAWScreen : uint8
{
    MainMenu,
    Programming,
    Simulation,
    ReplayAutopsy,
    ReplayBrowser,
    LanguageReference
};

/**
 * C++ Slate HUD implementing menu, programming, simulation, replay, browser,
 * and generated language-reference workflows.
 */
UCLASS()
class AUTOMATAWAR_API UAWHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Bind game/session delegates and select the initial or capture-mode screen. */
    virtual void NativeConstruct() override;
    /** Unbind all delegates before the UObject leaves the viewport. */
    virtual void NativeDestruct() override;
    /** Advance frame-rate-independent replay playback while the autopsy is active. */
    virtual void NativeTick(const FGeometry &MyGeometry, float InDeltaTime) override;

    /** Select one of the six stable HUD screens. */
    UFUNCTION(BlueprintCallable, Category = "AutomataWar|UI")
    void ShowScreen(EAWScreen Screen);

    /** @return The screen currently selected by the HUD. */
    UFUNCTION(BlueprintCallable, Category = "AutomataWar|UI")
    EAWScreen GetCurrentScreen() const { return CurrentScreen; }

    /** @return Number of Slate screens registered with the switcher. */
    int32 GetScreenCount() const;
    /** @return Active Slate switcher index, or -1 before construction. */
    int32 GetActiveScreenIndex() const;

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

private:
    TSharedRef<SWidget> BuildMainMenu();
    TSharedRef<SWidget> BuildProgrammingScreen();
    TSharedRef<SWidget> BuildSimulationScreen();
    TSharedRef<SWidget> BuildReplayAutopsyScreen();
    TSharedRef<SWidget> BuildReplayBrowser();
    TSharedRef<SWidget> BuildLanguageReference();

    UFUNCTION()
    void OnPhaseChanged(EAWMatchPhase NewPhase);

    UFUNCTION()
    void OnErrorReceived(const FString &Message);
    UFUNCTION()
    void OnSessionsRefreshed();

    void OnLocalMatch();
    void OnHostLAN();
    void OnFindLAN();
    void OnJoinIP();
    void OnReplayBrowserNav();
    void OnLanguageRef();
    void OnQuit();
    void OnSubmitSlot(int32 Slot);
    void OnLoadExample(int32 Slot, const FString &ScriptName);
    void OnTrainingBot(int32 Slot);
    void OnNextRound();

    void OnReplayPlay();
    void OnReplayPause();
    void OnReplayStepTick();
    void OnReplayStepBack();
    void OnReplayStepInstruction(int32 RobotIndex);
    void OnReplaySetSpeed(float Speed);
    void OnReplayScrub(int32 Tick);
    void InitializeReplayFromGameState();
    void UpdateReplayUI();
    void UpdateArenaFromReplay();

    void RefreshReplayList();
    void RefreshSessionList();
    void OnReplayLoad(const FString &Filename);
    void OnReplaySave();
    void OnReplayExport(const FString &Filename);
    void OnReplayImport();

    void SetStatus(const FString &Msg, bool bError = false);
    void PlayUISound(const TCHAR *AssetPath) const;

    UAWGameSubsystem *GetSubsystem() const;
    AAWArenaRenderer *FindOrSpawnRenderer();

    EAWScreen CurrentScreen = EAWScreen::MainMenu;
    TSharedPtr<SAWCodeEditor> EditorP1;
    TSharedPtr<SAWCodeEditor> EditorP2;
    TSharedPtr<SWidgetSwitcher> ScreenSwitcher;

    TSharedPtr<STextBlock> StatusText;
    TSharedPtr<SEditableTextBox> JoinIPField;
    TSharedPtr<SVerticalBox> SessionListBox;
    TSharedPtr<SVerticalBox> ReplayListBox;
    TSharedPtr<SEditableTextBox> ImportField;
    TSharedPtr<SEditableTextBox> ExportField;
    TSharedPtr<STextBlock> ReplayBrowserStatus;

    TSharedPtr<STextBlock> ReplayTickText;
    TSharedPtr<STextBlock> ReplaySpeedText;
    TSharedPtr<STextBlock> ReplayOutcomeText;
    TSharedPtr<STextBlock> ReplaySourceAText;
    TSharedPtr<STextBlock> ReplaySourceBText;
    TSharedPtr<STextBlock> ReplayRegistersP1;
    TSharedPtr<STextBlock> ReplayRegistersP2;
    TSharedPtr<STextBlock> ReplayEventLog;
    TSharedPtr<SSlider> ReplayScrubSlider;

    TUniquePtr<Automata::FAWReplayController> ReplayController;
    float ReplaySpeed = 1.f;
    bool bReplayPlaying = false;
    double ReplayAccumulator = 0.0;
};

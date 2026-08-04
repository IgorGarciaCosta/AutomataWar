#pragma once

/**
 * @file AWHUDWidget.h
 * @brief Runtime behavior for the designer-authored Automata War HUD.
 */

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AutomataWar/Game/AWMatchTypes.h"
#include "AutomataWar/Core/Replay/AWReplayController.h"
#include "AWHUDWidget.generated.h"

class UAWGameSubsystem;
class UAWCodeEditorWidget;
class AAWArenaRenderer;
class UComboBoxString;
class UEditableTextBox;
class USlider;
class UTextBlock;
class UWidgetSwitcher;

/** Six stable screen indices owned by the HUD Widget Blueprint switcher. */
UENUM(BlueprintType)
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
 * Supplies behavior and data to the WBP_AWHUD hierarchy. Visual structure,
 * sizing, and styling are owned by the Widget Blueprint.
 */
UCLASS(Blueprintable)
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

    /** @return Number of UMG screens registered with the switcher. */
    int32 GetScreenCount() const;
    /** @return Active UMG switcher index, or -1 before construction. */
    int32 GetActiveScreenIndex() const;

protected:
    /** Screen selected when this widget class is first constructed. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AutomataWar|UI")
    EAWScreen InitialScreen = EAWScreen::MainMenu;

private:
    UFUNCTION()
    void OnPhaseChanged(EAWMatchPhase NewPhase);

    UFUNCTION()
    void OnErrorReceived(const FString &Message);
    UFUNCTION()
    void OnSessionsRefreshed();

    UFUNCTION()
    void OnLocalMatch();
    UFUNCTION()
    void OnHostLAN();
    UFUNCTION()
    void OnFindLAN();
    UFUNCTION()
    void OnJoinSelectedSession();
    UFUNCTION()
    void OnJoinIP();
    UFUNCTION()
    void OnReplayBrowserNav();
    UFUNCTION()
    void OnLanguageRef();
    UFUNCTION()
    void OnBackToMainMenu();
    UFUNCTION()
    void OnQuit();

    void OnSubmitSlot(int32 Slot);
    void OnLoadExample(int32 Slot, const FString &ScriptName);
    void OnTrainingBot(int32 Slot);
    UFUNCTION()
    void OnSubmitP1();
    UFUNCTION()
    void OnSubmitP2();
    UFUNCTION()
    void OnAggressorP1();
    UFUNCTION()
    void OnAggressorP2();
    UFUNCTION()
    void OnCamperP1();
    UFUNCTION()
    void OnCamperP2();
    UFUNCTION()
    void OnKiterP1();
    UFUNCTION()
    void OnKiterP2();
    UFUNCTION()
    void OnTrainingP1();
    UFUNCTION()
    void OnTrainingP2();
    UFUNCTION()
    void OnNextRound();

    UFUNCTION()
    void OnReplayPlay();
    UFUNCTION()
    void OnReplayPause();
    UFUNCTION()
    void OnReplayStepTick();
    UFUNCTION()
    void OnReplayStepBack();
    void OnReplayStepInstruction(int32 RobotIndex);
    void OnReplaySetSpeed(float Speed);
    void OnReplayScrub(int32 Tick);
    UFUNCTION()
    void OnReplayScrubStart();
    UFUNCTION()
    void OnReplayStepP1();
    UFUNCTION()
    void OnReplayStepP2();
    UFUNCTION()
    void OnReplaySpeedQuarter();
    UFUNCTION()
    void OnReplaySpeedNormal();
    UFUNCTION()
    void OnReplaySpeedDouble();
    UFUNCTION()
    void OnReplaySpeedQuadruple();
    UFUNCTION()
    void OnReplayScrubChanged(float Value);
    void InitializeReplayFromGameState();
    bool InitializeReplay(const FString &SourceA, const FString &SourceB, int64 Seed);
    void UpdateReplayUI();
    void UpdateArenaFromReplay();

    void RefreshReplayList();
    void RefreshSessionList();
    void OnReplayLoad(const FString &Filename);
    UFUNCTION()
    void OnReplayLoadSelected();
    UFUNCTION()
    void OnReplaySave();
    void OnReplayExport(const FString &Filename);
    UFUNCTION()
    void OnReplayExportSelected();
    UFUNCTION()
    void OnReplayImport();
    UFUNCTION()
    void OnReplayRefresh();

    void PopulateLanguageReference();

    void SetStatus(const FString &Msg, bool bError = false);
    void PlayUISound(const TCHAR *AssetPath) const;

    UAWGameSubsystem *GetSubsystem() const;
    AAWArenaRenderer *FindOrSpawnRenderer();

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidgetSwitcher> ScreenSwitcher;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> StatusText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAWCodeEditorWidget> EditorP1;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAWCodeEditorWidget> EditorP2;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEditableTextBox> JoinIPField;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> SessionComboBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> ReplayComboBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEditableTextBox> ImportField;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEditableTextBox> ExportField;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ReplayBrowserStatus;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ReplayTickText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ReplaySpeedText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ReplayOutcomeText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ReplaySourceAText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ReplaySourceBText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ReplayRegistersP1;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ReplayRegistersP2;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ReplayEventLog;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> ReplayScrubSlider;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> LanguageReferenceText;

    EAWScreen CurrentScreen = EAWScreen::MainMenu;
    TArray<FString> ReplayFilenames;
    TUniquePtr<Automata::FAWReplayController> ReplayController;
    float ReplaySpeed = 1.f;
    bool bReplayPlaying = false;
    bool bUpdatingReplaySlider = false;
    double ReplayAccumulator = 0.0;
};

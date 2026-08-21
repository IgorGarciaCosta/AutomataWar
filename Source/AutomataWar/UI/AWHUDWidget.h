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
class UAWPlanVisualizationSubsystem;
class AAWArenaRenderer;
class UAWArenaSelectionScreen;
class UAWDifficultyScreen;
class UAWLanguageReferenceScreen;
class UAWMainMenuScreen;
class UAWMatchResultPopupWidget;
class UAWReplayAutopsyScreen;
class UAWReplayBrowserScreen;
class UBorder;
class UTextBlock;
class UTextureRenderTarget2D;
class UWidgetSwitcher;
enum class EAWAudioContext : uint8;
enum class EAWUIAction : uint8;

/** Six stable screen indices owned by the HUD Widget Blueprint switcher. */
UENUM(BlueprintType)
enum class EAWScreen : uint8
{
    MainMenu,
    Difficulty,
    ArenaSelection,
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

    /** Route the camera-owned arena feed to both gameplay screens. */
    void SetArenaRenderTarget(UTextureRenderTarget2D *RenderTarget);

protected:
    /** Screen selected when this widget class is first constructed. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AutomataWar|UI")
    EAWScreen InitialScreen = EAWScreen::MainMenu;

private:
    /** Drive non-shipping presentation capture modes requested on the command line. */
    void RunCaptureMode();

    void OnScreenAction(EAWUIAction Action);

    UFUNCTION()
    void OnPhaseChanged(EAWMatchPhase NewPhase);
    /** Initialize planning or replay presentation when a complete round record changes. */
    UFUNCTION()
    void OnResolvedRoundChanged();

    UFUNCTION()
    void OnErrorReceived(const FString &Message);
    UFUNCTION()
    void OnSubmissionResult(int32 SlotIndex, const FAWValidationResult &Result);
    UFUNCTION()
    void OnWithdrawalResult(int32 SlotIndex, const FAWValidationResult &Result);
    UFUNCTION()
    void OnSessionsRefreshed();

    /** Open single-player difficulty selection without changing match state. */
    UFUNCTION()
    void OnSinglePlayerNav();
    /** Retain the selected match preset and open procedural arena selection. */
    void OnDifficultySelected(EAWDifficulty Difficulty);
    /** Start the selected local or single-player mode on an arena. */
    void OnStartMatch(EAWArenaSize ArenaSize);
    /** Start local-versus directly for completed setup and capture tooling. */
    void StartLocalMatch(EAWDifficulty Difficulty = EAWDifficulty::Normal,
                         EAWArenaSize ArenaSize = EAWArenaSize::Standard);
    /** Open local-versus difficulty selection without changing match state. */
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
    void OnReturnToPlanningSlot(int32 Slot);
    void OnProgrammingCommandsChanged(int32 Slot);
    UFUNCTION()
    void OnSubmitP1();
    UFUNCTION()
    void OnSubmitP2();
    UFUNCTION()
    void OnNextRound();

    UFUNCTION()
    void OnReplayPlay();
    UFUNCTION()
    void OnReplayPause();
    UFUNCTION()
    void OnReplayStep();
    UFUNCTION()
    void OnReplayStepBack();
    void OnReplaySetSpeed(float Speed);
    UFUNCTION()
    void OnReplayStart();
    UFUNCTION()
    void OnReplaySpeedQuarter();
    UFUNCTION()
    void OnReplaySpeedNormal();
    UFUNCTION()
    void OnReplaySpeedDouble();
    UFUNCTION()
    void OnReplaySpeedQuadruple();
    void InitializeReplayFromGameState();
    /** Render the authoritative round-start arena while players assemble commands. */
    void InitializePlanningArenaFromGameState();
    bool BuildPlanningSimConfig(Automata::SimConfig &OutConfig) const;
    /** Reveal a pending terminal result after the visual replay has completed. */
    void ShowPendingMatchResult();
    bool InitializeReplay(const FAWResolvedRound &Round);
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
    /** Forward a high-level presentation context to the game-instance audio owner. */
    void SetAudioContext(EAWAudioContext Context) const;

    UAWGameSubsystem *GetSubsystem() const;
    UAWPlanVisualizationSubsystem *GetPlanVisualizationSubsystem() const;
    AAWArenaRenderer *FindOrSpawnRenderer();

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidgetSwitcher> ScreenSwitcher;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> StatusText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAWMainMenuScreen> MainMenuScreenWidget;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAWDifficultyScreen> DifficultyScreenWidget;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAWArenaSelectionScreen> ArenaSelectionScreenWidget;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAWReplayAutopsyScreen> ReplayAutopsyScreenWidget;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAWReplayBrowserScreen> ReplayBrowserScreenWidget;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAWLanguageReferenceScreen> LanguageReferenceScreenWidget;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAWMatchResultPopupWidget> MatchResultPopupWidget;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAWMatchResultPopupWidget> MatchResultPopupWidgetPlayerTwo;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UBorder> MatchResultScrim;

    EAWScreen CurrentScreen = EAWScreen::MainMenu;
    TArray<FString> ReplayFilenames;
    TUniquePtr<Automata::FAWReplayController> ReplayController;
    float ReplaySpeed = 1.f;
    bool bReplayPlaying = false;
    double ReplayAccumulator = 0.0;
    int32 LastProcessedReplayEventStep = INDEX_NONE;
    bool bMatchResultPending = false;
    bool bSinglePlayer = false;
    EAWDifficulty PendingDifficulty = EAWDifficulty::Normal;
};

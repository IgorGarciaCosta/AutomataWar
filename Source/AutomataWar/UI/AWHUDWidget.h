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
class AAWArenaRenderer;
class UAWDifficultyScreen;
class UAWLanguageReferenceScreen;
class UAWMainMenuScreen;
class UAWProgrammingScreen;
class UAWReplayAutopsyScreen;
class UAWReplayBrowserScreen;
class UAWSimulationScreen;
class UTextBlock;
class UTextureRenderTarget2D;
class UWidgetSwitcher;
enum class EAWAIDifficulty : uint8;
enum class EAWUIAction : uint8;

/** Seven stable screen indices owned by the HUD Widget Blueprint switcher. */
UENUM(BlueprintType)
enum class EAWScreen : uint8
{
    MainMenu,
    Difficulty,
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

    /** Route the camera-owned arena feed to both gameplay screens. */
    void SetArenaRenderTarget(UTextureRenderTarget2D *RenderTarget);

protected:
    /** Screen selected when this widget class is first constructed. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AutomataWar|UI")
    EAWScreen InitialScreen = EAWScreen::MainMenu;

private:
    void OnScreenAction(EAWUIAction Action);

    UFUNCTION()
    void OnPhaseChanged(EAWMatchPhase NewPhase);

    UFUNCTION()
    void OnErrorReceived(const FString &Message);
    UFUNCTION()
    void OnSessionsRefreshed();

    /** Open single-player difficulty selection without changing match state. */
    UFUNCTION()
    void OnSinglePlayerNav();
    /** Reset the arena and start slot 1 under the selected AI planner. */
    void OnStartSinglePlayer(EAWAIDifficulty Difficulty);
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
    bool InitializeReplay(const TArray<EAWCommand> &CommandsA, const TArray<EAWCommand> &CommandsB, int64 Seed,
                          int32 ActionPointsA = Automata::InitialActionPoints,
                          int32 ActionPointsB = Automata::InitialActionPoints,
                          const FAWRobotEffects &EffectsA = {}, const FAWRobotEffects &EffectsB = {},
                          int32 StartingSlot = 0, const TArray<uint8> &InitialState = {});
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
    TObjectPtr<UAWMainMenuScreen> MainMenuScreenWidget;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAWDifficultyScreen> DifficultyScreenWidget;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAWProgrammingScreen> ProgrammingScreenWidget;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAWSimulationScreen> SimulationScreenWidget;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAWReplayAutopsyScreen> ReplayAutopsyScreenWidget;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAWReplayBrowserScreen> ReplayBrowserScreenWidget;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAWLanguageReferenceScreen> LanguageReferenceScreenWidget;

    EAWScreen CurrentScreen = EAWScreen::MainMenu;
    TArray<FString> ReplayFilenames;
    TUniquePtr<Automata::FAWReplayController> ReplayController;
    float ReplaySpeed = 1.f;
    bool bReplayPlaying = false;
    double ReplayAccumulator = 0.0;
    int32 LastProcessedReplayEventStep = INDEX_NONE;
    bool bSinglePlayer = false;
};

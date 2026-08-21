#pragma once

/**
 * @file AWScreenWidget.h
 * @brief Native contracts for the modular Automata War HUD screens.
 */

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AutomataWar/Game/AWMatchTypes.h"
#include "AWScreenWidget.generated.h"

class UAWSimulationDockWidget;
class UAWTypewriterTextBlock;
class UButton;
class UComboBoxString;
class UEditableTextBox;
class UEnum;
class UImage;
class UTextBlock;
class UTextureRenderTarget2D;
class UWidget;

/** Semantic actions emitted by HUD screens and handled by the root HUD. */
UENUM()
enum class EAWUIAction : uint8
{
    SinglePlayer,
    DifficultyEasy,
    DifficultyNormal,
    DifficultyHard,
    ArenaCompact,
    ArenaStandard,
    ArenaExpanded,
    BackToDifficulty,
    LocalMatch,
    HostLAN,
    FindLAN,
    JoinSession,
    JoinIP,
    OpenReplayBrowser,
    OpenLanguageReference,
    Quit,
    BackToMainMenu,
    SubmitP1,
    SubmitP2,
    ReturnToPlanningP1,
    ReturnToPlanningP2,
    ReplayStart,
    ReplayBack,
    ReplayPause,
    ReplayPlay,
    ReplayStep,
    ReplaySpeedQuarter,
    ReplaySpeedNormal,
    ReplaySpeedDouble,
    ReplaySpeedQuadruple,
    NextRound,
    ReplayRefresh,
    ReplaySave,
    ReplayLoad,
    ReplayExport,
    ReplayImport
};

DECLARE_MULTICAST_DELEGATE_OneParam(FAWUIActionEvent, EAWUIAction);
DECLARE_MULTICAST_DELEGATE_OneParam(FAWProgrammingPanelEvent, int32);

/** Base class that keeps child screens independent from the root HUD type. */
UCLASS(Abstract, Blueprintable)
class AUTOMATAWAR_API UAWScreenWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Raised when the player invokes a semantic action on this screen. */
    FAWUIActionEvent OnAction;

protected:
    void BroadcastAction(EAWUIAction Action) { OnAction.Broadcast(Action); }
};

/** Main menu controls and LAN connection inputs. */
UCLASS(Blueprintable)
class AUTOMATAWAR_API UAWMainMenuScreen : public UAWScreenWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    /** Restarts the main title's delayed typewriter animation. */
    void PlayTitleAnimation();

    FString GetJoinIPAddress() const;
    int32 GetSelectedSessionIndex() const;
    void ResetSessions();
    void AddSession(const FString &Label);
    void SelectFirstSession();

private:
    /** Open the dedicated single-player difficulty screen. */
    UFUNCTION()
    void OnSinglePlayer();
    UFUNCTION()
    void OnLocalMatch();
    UFUNCTION()
    void OnHostLAN();
    UFUNCTION()
    void OnFindLAN();
    UFUNCTION()
    void OnJoinSession();
    UFUNCTION()
    void OnJoinIP();
    UFUNCTION()
    void OnOpenReplayBrowser();
    UFUNCTION()
    void OnOpenLanguageReference();
    UFUNCTION()
    void OnQuit();

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEditableTextBox> JoinIPField;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> SessionComboBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAWTypewriterTextBlock> MainTypewriterTitle;
};

/** Difficulty selection for a new single-player match. */
UCLASS(Blueprintable)
class AUTOMATAWAR_API UAWDifficultyScreen : public UAWScreenWidget
{
    GENERATED_BODY()

public:
    /** Bind difficulty and back buttons from the generated Widget Blueprint. */
    virtual void NativeConstruct() override;

private:
    /** Start a single-player match with the shortest AI plan. */
    UFUNCTION()
    void OnEasy();
    /** Start a single-player match with the balanced AI plan. */
    UFUNCTION()
    void OnNormal();
    /** Start a single-player match with the full tactical AI plan. */
    UFUNCTION()
    void OnHard();
    /** Return to the main menu without starting a match. */
    UFUNCTION()
    void OnBack();
};

/** Procedural arena-size selection shown after single-player difficulty. */
UCLASS(Blueprintable)
class AUTOMATAWAR_API UAWArenaSelectionScreen : public UAWScreenWidget
{
    GENERATED_BODY()

public:
    /** Bind arena choices and the difficulty-navigation button. */
    virtual void NativeConstruct() override;

private:
    /** Select the half-dimension compact arena. */
    UFUNCTION()
    void OnCompact();
    /** Select the default arena dimensions. */
    UFUNCTION()
    void OnStandard();
    /** Select the double-dimension expanded arena. */
    UFUNCTION()
    void OnExpanded();
    /** Return to difficulty selection without starting a match. */
    UFUNCTION()
    void OnBack();
};

/** Reusable command editor and submission transition for one combatant. */
UCLASS(Blueprintable)
class AUTOMATAWAR_API UAWProgrammingPanelWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry &MyGeometry, float InDeltaTime) override;
    virtual void SynchronizeProperties() override;

    /** Raised when this panel requests authoritative command submission. */
    FAWProgrammingPanelEvent OnSubmitted;

    /** Raised when the submitted program is reopened for editing. */
    FAWProgrammingPanelEvent OnPlanningReturned;

    /** Raised after any queue mutation, including a round reset. */
    FAWProgrammingPanelEvent OnCommandsChanged;

    /** @return The command queue currently shown by this panel. */
    const TArray<EAWCommand> &GetCommands() const { return Commands; }

    /** @return The command queue as newline-delimited source text. */
    FString GetCommandText() const;

    /** Accept or reject the submission after gameplay validation. */
    void ResolveSubmission(bool bAccepted);

    /** Reopen the panel for a new programming phase without clearing commands. */
    void ResetSubmissionState();

    /** Start a new round with an empty queue while preserving the supplied AP balance. */
    void ResetForNewRound(int32 Health, int32 ActionPoints);

    /** Replace the live HP/AP readout for this player. */
    void SetPlayerStats(int32 Health, int32 ActionPoints);

    /** Toggle read-only presentation for the AI-controlled command slot. */
    void SetAIControlled(bool bInAIControlled);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AutomataWar|UI")
    int32 PlayerIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AutomataWar|UI")
    FText PlayerLabel = INVTEXT("PLAYER 1");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AutomataWar|UI")
    FLinearColor AccentColor = FLinearColor(0.18f, 1.f, 0.42f, 1.f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AutomataWar|UI", meta = (ClampMin = "0.1"))
    float PowerTransitionDuration = 0.42f;

private:
    UFUNCTION()
    void OnMove();
    UFUNCTION()
    void OnFire();
    UFUNCTION()
    void OnTurnLeft();
    UFUNCTION()
    void OnTurnRight();
    /** Append a zero-cost idle step to the command queue. */
    UFUNCTION()
    void OnWait();
    /** Append a charged one-hit shield to the command queue. */
    UFUNCTION()
    void OnChargeShield();
    /** Append a command that doubles the next move distance. */
    UFUNCTION()
    void OnAccelerate();
    UFUNCTION()
    void OnRemove();
    /** Request submission immediately, then play the cosmetic power-off transition. */
    UFUNCTION()
    void OnSubmit();
    UFUNCTION()
    void OnReturnToPlanning();

    void AddCommand(EAWCommand Command);
    void RefreshCommands();
    void StartPowerTransition(bool bTurningOff);
    void ApplyPowerTransition();

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> ProgrammingPanelContent;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> ProgrammingReturnLayer;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> ProgrammingShutdownLine;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ProgrammingPlayerTitle;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ProgrammingPlayerSlot;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ProgrammingCommandsTitle;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ProgrammingProgramText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ProgrammingRemoveActionButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ProgrammingSubmitButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ProgrammingReturnToPlanningButton;

    TArray<EAWCommand> Commands;
    float PowerTransitionAlpha = 0.f;
    int8 PowerTransitionDirection = 0;
    bool bAwaitingSubmissionResult = false;
    bool bSubmitted = false;
    bool bAIControlled = false;
    int32 PlayerHealth = Automata::MaxHP;
    int32 AvailableActionPoints = Automata::InitialActionPoints;
};

/** Programming screen composed from two reusable combatant panels. */
UCLASS(Blueprintable)
class AUTOMATAWAR_API UAWProgrammingScreen : public UAWScreenWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    TArray<EAWCommand> GetCommands(int32 PlayerIndex) const;
    FString GetCommandText(int32 PlayerIndex) const;
    void ResolveSubmission(int32 PlayerIndex, bool bAccepted);
    void ResetSubmissionState();
    /** Clear both queues and restore each player's current HP and AP balances. */
    void ResetForNewRound(int32 Health0, int32 ActionPoints0, int32 Health1, int32 ActionPoints1);
    void SetPlayerStats(int32 PlayerIndex, int32 Health, int32 ActionPoints);
    /** Toggle slot 1 between local-player editing and AI read-only presentation. */
    void SetSinglePlayerMode(bool bSinglePlayer);

private:
    UFUNCTION()
    void OnBack();
    UFUNCTION()
    void OnPanelSubmitted(int32 PlayerIndex);
    void OnPanelReturned(int32 PlayerIndex);
    UAWProgrammingPanelWidget *GetPanel(int32 PlayerIndex) const;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAWProgrammingPanelWidget> ProgrammingP1PanelWidget;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAWProgrammingPanelWidget> ProgrammingP2PanelWidget;
};

/** Reusable read-only command dock used by simulation and replay. */
UCLASS(Blueprintable)
class AUTOMATAWAR_API UAWSimulationDockWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Replace the command list and optionally highlight one action. */
    void SetCommands(const TArray<EAWCommand> &Commands, int32 CurrentCommand = INDEX_NONE);

    /** Replace the optional tank-state readout. */
    void SetDetails(const FString &Details);

protected:
    virtual void SynchronizeProperties() override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AutomataWar|UI")
    FText PlayerLabel = INVTEXT("PLAYER PROGRAM");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AutomataWar|UI")
    FLinearColor AccentColor = FLinearColor(0.18f, 1.f, 0.42f, 1.f);

private:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> SimulationDockTitle;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> SimulationDockCommandsText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> SimulationDockDetails;
};

/** Read-only match execution presentation composed from reusable docks. */
UCLASS(Blueprintable)
class AUTOMATAWAR_API UAWSimulationScreen : public UAWScreenWidget
{
    GENERATED_BODY()

public:
    /** Display the camera-owned arena render target in the center viewport. */
    void SetArenaRenderTarget(UTextureRenderTarget2D *RenderTarget);
    void SetCommands(const TArray<EAWCommand> &PlayerOneCommands, const TArray<EAWCommand> &PlayerTwoCommands);
    void SetPlayerDetails(int32 PlayerIndex, const FString &Details);

private:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> SimulationArenaFeed;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAWSimulationDockWidget> SimulationP1DockWidget;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAWSimulationDockWidget> SimulationP2DockWidget;
};

/** Replay transport and debugger presentation. */
UCLASS(Blueprintable)
class AUTOMATAWAR_API UAWReplayAutopsyScreen : public UAWScreenWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    /** Raised when either embedded programming queue changes. */
    FAWProgrammingPanelEvent OnProgrammingCommandsChanged;

    /** Display the camera-owned arena render target in the center viewport. */
    void SetArenaRenderTarget(UTextureRenderTarget2D *RenderTarget);
    /** Switch the side bays between editable programs and replay command docks. */
    void SetProgrammingMode(bool bProgramming, bool bCanAdvanceRound);
    /** Return the editable command queue for one player slot. */
    TArray<EAWCommand> GetProgrammingCommands(int32 PlayerIndex) const;
    /** Accept or reject one panel's submission after authoritative validation. */
    void ResolveProgrammingSubmission(int32 PlayerIndex, bool bAccepted);
    /** Clear both queues and restore each player's current HP and AP balances. */
    void ResetProgrammingForNewRound(int32 Health0, int32 ActionPoints0,
                                     int32 Health1, int32 ActionPoints1);
    /** Replace one programming panel's live HP and AP readout. */
    void SetProgrammingPlayerStats(int32 PlayerIndex, int32 Health, int32 ActionPoints);
    /** Toggle player two between local editing and AI read-only presentation. */
    void SetSinglePlayerMode(bool bSinglePlayer);
    void SetSpeed(float Speed);
    void SetCombatantData(int32 PlayerIndex, const TArray<EAWCommand> &Commands, int32 CurrentCommand, const FString &Details);

private:
    UFUNCTION()
    void OnStart();
    UFUNCTION()
    void OnBack();
    UFUNCTION()
    void OnPause();
    UFUNCTION()
    void OnPlay();
    UFUNCTION()
    void OnStep();
    UFUNCTION()
    void OnSpeedQuarter();
    UFUNCTION()
    void OnSpeedNormal();
    UFUNCTION()
    void OnSpeedDouble();
    UFUNCTION()
    void OnSpeedQuadruple();
    UFUNCTION()
    void OnBackToMenu();
    UFUNCTION()
    void OnNextRound();
    /** Forward a reusable panel submission to the root HUD. */
    void OnPanelSubmitted(int32 PlayerIndex);
    /** Forward a reusable panel withdrawal to the root HUD. */
    void OnPanelReturned(int32 PlayerIndex);
    /** Forward queue edits so the root HUD can rebuild local plan visualization. */
    void OnPanelCommandsChanged(int32 PlayerIndex);
    /** Resolve one of the two embedded programming panels. */
    UAWProgrammingPanelWidget *GetProgrammingPanel(int32 PlayerIndex) const;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ReplayEyebrow;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ReplayTitle;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ReplaySpeedText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> ReplayPlaybackControls;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> NextRoundButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAWProgrammingPanelWidget> ProgrammingP1PanelWidget;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAWProgrammingPanelWidget> ProgrammingP2PanelWidget;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAWSimulationDockWidget> ReplayP1DockWidget;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAWSimulationDockWidget> ReplayP2DockWidget;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> ReplayArenaFeed;
};

/** Saved replay selection and import/export controls. */
UCLASS(Blueprintable)
class AUTOMATAWAR_API UAWReplayBrowserScreen : public UAWScreenWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    int32 GetSelectedReplayIndex() const;
    FString GetImportData() const;
    void ResetReplays();
    void AddReplay(const FString &Label);
    void SelectFirstReplay();
    void SetExportData(const FString &Data);
    void SetStatus(const FString &Status);

private:
    UFUNCTION()
    void OnBack();
    UFUNCTION()
    void OnRefresh();
    UFUNCTION()
    void OnSave();
    UFUNCTION()
    void OnLoad();
    UFUNCTION()
    void OnExport();
    UFUNCTION()
    void OnImport();

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> ReplayComboBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEditableTextBox> ImportField;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEditableTextBox> ExportField;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ReplayBrowserStatus;
};

/**
 * Presents the terminal match result as a modal overlay owned by WBP_AWMatchResultPopup.
 * It animates itself on and off screen and emits BackToMainMenu after its exit completes.
 */
UCLASS(Blueprintable)
class AUTOMATAWAR_API UAWMatchResultPopupWidget : public UAWScreenWidget
{
    GENERATED_BODY()

public:
    /** Bind the return button and initialize the popup off screen. */
    virtual void NativeConstruct() override;
    /** Advance the frame-rate-independent entrance or exit transition. */
    virtual void NativeTick(const FGeometry &MyGeometry, float InDeltaTime) override;

    /** Populate and animate the popup for a terminal match outcome. */
    void ShowResult(int32 WinnerSlot, int32 ViewerSlot, EAWMatchEndReason EndReason, bool bUsePlayerLabels);
    /** Build the reason-specific result label for one player's viewpoint. */
    static FText FormatResultText(int32 WinnerSlot, int32 ViewerSlot, EAWMatchEndReason EndReason,
                                  bool bUsePlayerLabels, const UEnum *MessageEnum = nullptr);

protected:
    /** Seconds used by each entrance and exit transition. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AutomataWar|UI", meta = (ClampMin = "0.1"))
    float TransitionDuration = 0.35f;

    /** Content-authored enum whose display names are the match result messages. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AutomataWar|UI")
    TObjectPtr<UEnum> MatchResultMessageEnum;

private:
    /** Start the exit transition before returning to the menu. */
    UFUNCTION()
    void OnReturnToMainMenu();
    /** Apply the current transition opacity and position. */
    void ApplyTransition();

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> MatchResultText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> MatchResultReturnButton;

    float TransitionAlpha = 0.f;
    int8 TransitionDirection = 0;
};

/** Programming language reference presentation. */
UCLASS(Blueprintable)
class AUTOMATAWAR_API UAWLanguageReferenceScreen : public UAWScreenWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    void SetReference(const FString &Reference);

private:
    UFUNCTION()
    void OnBack();

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> LanguageReferenceText;
};
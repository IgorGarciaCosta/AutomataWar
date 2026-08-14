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
class UButton;
class UComboBoxString;
class UEditableTextBox;
class USlider;
class UTextBlock;
class UWidget;

/** Semantic actions emitted by HUD screens and handled by the root HUD. */
UENUM()
enum class EAWUIAction : uint8
{
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
DECLARE_MULTICAST_DELEGATE_OneParam(FAWReplayScrubEvent, float);
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

    FString GetJoinIPAddress() const;
    int32 GetSelectedSessionIndex() const;
    void ResetSessions();
    void AddSession(const FString &Label);
    void SelectFirstSession();

private:
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

    /** Raised after the power-off transition completes. */
    FAWProgrammingPanelEvent OnSubmitted;

    /** Raised when the submitted program is reopened for editing. */
    FAWProgrammingPanelEvent OnPlanningReturned;

    /** @return The command queue currently shown by this panel. */
    const TArray<EAWCommand> &GetCommands() const { return Commands; }

    /** @return The command queue as newline-delimited source text. */
    FString GetCommandText() const;

    /** Accept or reject the submission after gameplay validation. */
    void ResolveSubmission(bool bAccepted);

    /** Reopen the panel for a new programming phase without clearing commands. */
    void ResetSubmissionState();

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
    UFUNCTION()
    void OnRemove();
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
    void SetCommands(const TArray<EAWCommand> &PlayerOneCommands, const TArray<EAWCommand> &PlayerTwoCommands);

private:
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

    /** Raised for player-driven timeline changes only. */
    FAWReplayScrubEvent OnScrubbed;

    void SetTimeline(float Speed, float NormalizedPosition);
    void SetSpeed(float Speed);
    void SetCombatantData(int32 PlayerIndex, const TArray<EAWCommand> &Commands, int32 CurrentCommand, const FString &Details);
    void SetEventLog(const FString &EventLog);

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
    UFUNCTION()
    void OnScrubChanged(float Value);

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ReplaySpeedText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAWSimulationDockWidget> ReplayP1DockWidget;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAWSimulationDockWidget> ReplayP2DockWidget;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ReplayEventLog;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> ReplayScrubSlider;

    bool bUpdatingSlider = false;
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
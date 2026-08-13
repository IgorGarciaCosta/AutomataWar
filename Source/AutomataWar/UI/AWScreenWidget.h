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

/** Button-built command lists for both combatants. */
UCLASS(Blueprintable)
class AUTOMATAWAR_API UAWProgrammingScreen : public UAWScreenWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    TArray<EAWCommand> GetCommands(int32 PlayerIndex) const;
    FString GetCommandText(int32 PlayerIndex) const;

private:
    UFUNCTION()
    void OnBack();
    UFUNCTION()
    void OnSubmitP1();
    UFUNCTION()
    void OnSubmitP2();
    UFUNCTION()
    void OnMoveP1();
    UFUNCTION()
    void OnFireP1();
    UFUNCTION()
    void OnTurnLeftP1();
    UFUNCTION()
    void OnTurnRightP1();
    UFUNCTION()
    void OnRemoveP1();
    UFUNCTION()
    void OnMoveP2();
    UFUNCTION()
    void OnFireP2();
    UFUNCTION()
    void OnTurnLeftP2();
    UFUNCTION()
    void OnTurnRightP2();
    UFUNCTION()
    void OnRemoveP2();

    void AddCommand(int32 PlayerIndex, EAWCommand Command);
    void RemoveLastCommand(int32 PlayerIndex);
    void RefreshCommands(int32 PlayerIndex);

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ProgramP1Text;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ProgramP2Text;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> RemoveActionP1Button;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> RemoveActionP2Button;

    TArray<EAWCommand> Commands[2];
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
    FLinearColor AccentColor = FLinearColor(0.f, 0.78f, 0.9f, 1.f);

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

    void SetOutcome(const FString &Outcome);
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
    TObjectPtr<UTextBlock> ReplayOutcomeText;

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
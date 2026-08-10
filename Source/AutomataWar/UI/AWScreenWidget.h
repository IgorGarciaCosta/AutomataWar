#pragma once

/**
 * @file AWScreenWidget.h
 * @brief Native contracts for the modular Automata War HUD screens.
 */

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AWScreenWidget.generated.h"

class UAWCodeEditorWidget;
class UAWSimulationDockWidget;
class UComboBoxString;
class UEditableTextBox;
class UMultiLineEditableTextBox;
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
    AggressorP1,
    AggressorP2,
    CamperP1,
    CamperP2,
    KiterP1,
    KiterP2,
    TrainingP1,
    TrainingP2,
    ReplayStart,
    ReplayBack,
    ReplayPause,
    ReplayPlay,
    ReplayStep,
    ReplayStepP1,
    ReplayStepP2,
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

/** Source editors and programming actions for both combatants. */
UCLASS(Blueprintable)
class AUTOMATAWAR_API UAWProgrammingScreen : public UAWScreenWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    FString GetSource(int32 PlayerIndex) const;
    void SetSource(int32 PlayerIndex, const FString &Source);

private:
    UFUNCTION()
    void OnBack();
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

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAWCodeEditorWidget> EditorP1;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAWCodeEditorWidget> EditorP2;
};

/** Reusable read-only source dock used by both simulation combatants. */
UCLASS(Blueprintable)
class AUTOMATAWAR_API UAWSimulationDockWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Replace the program displayed by this dock. */
    void SetSource(const FString &Source);

protected:
    virtual void SynchronizeProperties() override;

    /** Heading shown above the source listing. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AutomataWar|UI")
    FText PlayerLabel = INVTEXT("PLAYER PROGRAM");

    /** Player accent applied to the heading. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AutomataWar|UI")
    FLinearColor AccentColor = FLinearColor(0.f, 0.78f, 0.9f, 1.f);

private:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> SimulationDockTitle;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UMultiLineEditableTextBox> SimulationDockSourceBox;
};

/** Read-only match execution presentation composed from reusable docks. */
UCLASS(Blueprintable)
class AUTOMATAWAR_API UAWSimulationScreen : public UAWScreenWidget
{
    GENERATED_BODY()

public:
    void SetSources(const FString &PlayerOneSource, const FString &PlayerTwoSource);

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
    void SetTimeline(int32 CurrentTick, int32 TotalTicks, float Speed, float NormalizedPosition);
    void SetSpeed(float Speed);
    void SetCombatantData(int32 PlayerIndex, const FString &Source, const FString &Registers);
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
    void OnStepP1();
    UFUNCTION()
    void OnStepP2();
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
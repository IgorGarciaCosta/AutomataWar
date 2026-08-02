#pragma once

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

UCLASS()
class AUTOMATAWAR_API UAWHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "AutomataWar|UI")
	void ShowScreen(EAWScreen Screen);

	UFUNCTION(BlueprintCallable, Category = "AutomataWar|UI")
	EAWScreen GetCurrentScreen() const { return CurrentScreen; }

	int32 GetScreenCount() const;
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
	void OnErrorReceived(const FString& Message);
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
	void OnLoadExample(int32 Slot, const FString& ScriptName);
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
	void OnReplayLoad(const FString& Filename);
	void OnReplaySave();
	void OnReplayExport(const FString& Filename);
	void OnReplayImport();

	void SetStatus(const FString& Msg, bool bError = false);

	UAWGameSubsystem* GetSubsystem() const;
	AAWArenaRenderer* FindOrSpawnRenderer();

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

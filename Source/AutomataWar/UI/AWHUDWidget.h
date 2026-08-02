#pragma once

/**
 * @file AWHUDWidget.h
 * @brief Root UMG widget managing all UI states: main menu, programming/split editors,
 *        in-match HUD, replay debugger, language reference, and replay browser.
 */

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AutomataWar/Game/AWMatchTypes.h"
#include "AWHUDWidget.generated.h"

class SAWCodeEditor;
class UAWGameSubsystem;

/** UI screens managed by the root HUD widget. */
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
 * @brief Root widget that owns all Automata War UI screens.
 *
 * Built entirely in C++ using Slate/UMG. No Blueprint widget assets.
 * Keyboard and gamepad navigable menu items. Responds to phase changes
 * from GameState to transition screens automatically.
 */
UCLASS()
class AUTOMATAWAR_API UAWHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Transition to a specific screen. */
	UFUNCTION(BlueprintCallable, Category = "AutomataWar|UI")
	void ShowScreen(EAWScreen Screen);

	/** Get the currently active screen. */
	UFUNCTION(BlueprintCallable, Category = "AutomataWar|UI")
	EAWScreen GetCurrentScreen() const { return CurrentScreen; }

private:
	/** Build the main menu panel. */
	TSharedRef<SWidget> BuildMainMenu();
	/** Build the programming split-editor panel. */
	TSharedRef<SWidget> BuildProgrammingScreen();
	/** Build the simulation waiting panel. */
	TSharedRef<SWidget> BuildSimulationScreen();
	/** Build the replay autopsy/debugger panel. */
	TSharedRef<SWidget> BuildReplayAutopsyScreen();
	/** Build the replay browser panel. */
	TSharedRef<SWidget> BuildReplayBrowser();
	/** Build the language reference panel. */
	TSharedRef<SWidget> BuildLanguageReference();

	/** Handle phase change from GameState. */
	UFUNCTION()
	void OnPhaseChanged(EAWMatchPhase NewPhase);

	/** Menu actions. */
	void OnLocalMatch();
	void OnHostLAN();
	void OnFindLAN();
	void OnJoinIP(const FString& IP);
	void OnReplayBrowser();
	void OnLanguageRef();
	void OnQuit();
	void OnSubmitSlot(int32 Slot);
	void OnLoadExample(int32 Slot, const FString& ScriptName);
	void OnTrainingBot(int32 Slot);
	void OnNextRound();

	/** Replay debugger state. */
	void OnReplayPlay();
	void OnReplayPause();
	void OnReplayStepTick();
	void OnReplayStepInstruction();
	void OnReplayStepBack();
	void OnReplaySetSpeed(float Speed);
	void OnReplayScrub(int32 Tick);

	/** Utility to get subsystem. */
	UAWGameSubsystem* GetSubsystem() const;

	EAWScreen CurrentScreen = EAWScreen::MainMenu;

	/** Slate code editors (programming screen). */
	TSharedPtr<SAWCodeEditor> EditorP1;
	TSharedPtr<SAWCodeEditor> EditorP2;

	/** Root switchable container. */
	TSharedPtr<SWidgetSwitcher> ScreenSwitcher;

	/** Replay debugger state. */
	int32 ReplayCurrentTick = 0;
	float ReplaySpeed = 1.f;
	bool bReplayPlaying = false;
	double ReplayAccumulator = 0.0;
};

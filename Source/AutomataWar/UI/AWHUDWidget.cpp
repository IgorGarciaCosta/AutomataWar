/**
 * @file AWHUDWidget.cpp
 * @brief Implementation of the root HUD widget managing all UI screens.
 */

#include "AWHUDWidget.h"
#include "SAWCodeEditor.h"
#include "AWUITypes.h"
#include "AutomataWar/Game/AWGameSubsystem.h"
#include "AutomataWar/Game/AWGameState.h"
#include "AutomataWar/Game/AWGameMode.h"
#include "AutomataWar/Game/AWExampleScripts.h"
#include "AutomataWar/Game/AWReplayService.h"
#include "AutomataWar/Game/AWPlayerController.h"
#include "AutomataWar/Core/AutomataRules.h"
#include "AutomataWar/Core/Lang/AutomataCompiler.h"
#include "AutomataWar/Core/Sim/AutomataSimulation.h"
#include "AutomataWar/Core/Replay/AutomataReplay.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Components/NativeWidgetHost.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/GameStateBase.h"

void UAWHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Listen for phase changes
	if (UWorld* World = GetWorld())
	{
		if (AAWGameState* GS = World->GetGameState<AAWGameState>())
		{
			GS->OnPhaseChanged.AddDynamic(this, &UAWHUDWidget::OnPhaseChanged);
		}
	}

	ShowScreen(EAWScreen::MainMenu);
}

void UAWHUDWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		if (AAWGameState* GS = World->GetGameState<AAWGameState>())
		{
			GS->OnPhaseChanged.RemoveDynamic(this, &UAWHUDWidget::OnPhaseChanged);
		}
	}
	Super::NativeDestruct();
}

void UAWHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Replay auto-advance
	if (bReplayPlaying && CurrentScreen == EAWScreen::ReplayAutopsy)
	{
		ReplayAccumulator += InDeltaTime * ReplaySpeed;
		const double TickInterval = 1.0 / 30.0; // 30 ticks/sec visual rate
		while (ReplayAccumulator >= TickInterval)
		{
			ReplayAccumulator -= TickInterval;
			OnReplayStepTick();
		}
	}
}

UAWGameSubsystem* UAWHUDWidget::GetSubsystem() const
{
	if (UGameInstance* GI = GetGameInstance())
	{
		return GI->GetSubsystem<UAWGameSubsystem>();
	}
	return nullptr;
}

void UAWHUDWidget::ShowScreen(EAWScreen Screen)
{
	CurrentScreen = Screen;

	// Clear existing content and rebuild
	// Since we use NativeWidgetHost for Slate embedding, we rebuild the root
	// For simplicity, we use the Slate widget directly via TakeWidget override pattern
	// The actual screen switching is handled by rebuilding content in the UMG overlay

	UE_LOG(LogAutomataUI, Log, TEXT("Screen transition: %d"), (int32)Screen);
}

void UAWHUDWidget::OnPhaseChanged(EAWMatchPhase NewPhase)
{
	switch (NewPhase)
	{
	case EAWMatchPhase::Programming:
		ShowScreen(EAWScreen::Programming);
		break;
	case EAWMatchPhase::Simulation:
		ShowScreen(EAWScreen::Simulation);
		break;
	case EAWMatchPhase::ReplayAutopsy:
		ReplayCurrentTick = 0;
		bReplayPlaying = false;
		ShowScreen(EAWScreen::ReplayAutopsy);
		break;
	default:
		break;
	}
}

TSharedRef<SWidget> UAWHUDWidget::BuildMainMenu()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(20)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("AUTOMATA WAR")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 32))
			.ColorAndOpacity(FSlateColor(AWUIColors::AccentCyan))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(10, 5)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("[>] Local Match")))
			.OnClicked_Lambda([this]() { OnLocalMatch(); return FReply::Handled(); })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(10, 5)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("[H] Host LAN")))
			.OnClicked_Lambda([this]() { OnHostLAN(); return FReply::Handled(); })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(10, 5)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("[F] Find LAN")))
			.OnClicked_Lambda([this]() { OnFindLAN(); return FReply::Handled(); })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(10, 5)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("[R] Replay Browser")))
			.OnClicked_Lambda([this]() { OnReplayBrowser(); return FReply::Handled(); })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(10, 5)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("[?] Language Reference")))
			.OnClicked_Lambda([this]() { OnLanguageRef(); return FReply::Handled(); })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(10, 5)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("[X] Quit")))
			.OnClicked_Lambda([this]() { OnQuit(); return FReply::Handled(); })
		];
}

TSharedRef<SWidget> UAWHUDWidget::BuildProgrammingScreen()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("PLAYER 1")))
				.ColorAndOpacity(FSlateColor(AWUIColors::AccentCyan))
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SAssignNew(EditorP1, SAWCodeEditor)
				.InitialText(FText::FromString(FAWExampleScripts::DefaultBot()))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
				[
					SNew(SButton).Text(FText::FromString(TEXT("Submit")))
					.OnClicked_Lambda([this]() { OnSubmitSlot(0); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
				[
					SNew(SButton).Text(FText::FromString(TEXT("Aggressor")))
					.OnClicked_Lambda([this]() { OnLoadExample(0, TEXT("Aggressor")); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
				[
					SNew(SButton).Text(FText::FromString(TEXT("Camper")))
					.OnClicked_Lambda([this]() { OnLoadExample(0, TEXT("Camper")); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
				[
					SNew(SButton).Text(FText::FromString(TEXT("Kiter")))
					.OnClicked_Lambda([this]() { OnLoadExample(0, TEXT("Kiter")); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
				[
					SNew(SButton).Text(FText::FromString(TEXT("Train vs Bot")))
					.OnClicked_Lambda([this]() { OnTrainingBot(0); return FReply::Handled(); })
				]
			]
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SSeparator).Orientation(Orient_Vertical)
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("PLAYER 2")))
				.ColorAndOpacity(FSlateColor(AWUIColors::AccentCoral))
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SAssignNew(EditorP2, SAWCodeEditor)
				.InitialText(FText::FromString(FAWExampleScripts::DefaultBot()))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
				[
					SNew(SButton).Text(FText::FromString(TEXT("Submit")))
					.OnClicked_Lambda([this]() { OnSubmitSlot(1); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
				[
					SNew(SButton).Text(FText::FromString(TEXT("Aggressor")))
					.OnClicked_Lambda([this]() { OnLoadExample(1, TEXT("Aggressor")); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
				[
					SNew(SButton).Text(FText::FromString(TEXT("Camper")))
					.OnClicked_Lambda([this]() { OnLoadExample(1, TEXT("Camper")); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
				[
					SNew(SButton).Text(FText::FromString(TEXT("Kiter")))
					.OnClicked_Lambda([this]() { OnLoadExample(1, TEXT("Kiter")); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
				[
					SNew(SButton).Text(FText::FromString(TEXT("Train vs Bot")))
					.OnClicked_Lambda([this]() { OnTrainingBot(1); return FReply::Handled(); })
				]
			]
		];
}

TSharedRef<SWidget> UAWHUDWidget::BuildSimulationScreen()
{
	return SNew(SBox)
		.HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("SIMULATING...")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 24))
			.ColorAndOpacity(FSlateColor(AWUIColors::AccentCyan))
		];
}

TSharedRef<SWidget> UAWHUDWidget::BuildReplayAutopsyScreen()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(10)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("REPLAY / AUTOPSY")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
			.ColorAndOpacity(FSlateColor(AWUIColors::TextPrimary))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(10, 4)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
			[
				SNew(SButton).Text(FText::FromString(TEXT("|<")))
				.OnClicked_Lambda([this]() { OnReplayScrub(0); return FReply::Handled(); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
			[
				SNew(SButton).Text(FText::FromString(TEXT("<")))
				.OnClicked_Lambda([this]() { OnReplayStepBack(); return FReply::Handled(); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
			[
				SNew(SButton).Text(FText::FromString(TEXT("||")))
				.OnClicked_Lambda([this]() { OnReplayPause(); return FReply::Handled(); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
			[
				SNew(SButton).Text(FText::FromString(TEXT(">")))
				.OnClicked_Lambda([this]() { OnReplayPlay(); return FReply::Handled(); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
			[
				SNew(SButton).Text(FText::FromString(TEXT(">|")))
				.OnClicked_Lambda([this]() { OnReplayStepTick(); return FReply::Handled(); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
			[
				SNew(SButton).Text(FText::FromString(TEXT(">>")))
				.OnClicked_Lambda([this]() { OnReplayStepInstruction(); return FReply::Handled(); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
			[
				SNew(SButton).Text(FText::FromString(TEXT("0.25x")))
				.OnClicked_Lambda([this]() { OnReplaySetSpeed(0.25f); return FReply::Handled(); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
			[
				SNew(SButton).Text(FText::FromString(TEXT("1x")))
				.OnClicked_Lambda([this]() { OnReplaySetSpeed(1.f); return FReply::Handled(); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
			[
				SNew(SButton).Text(FText::FromString(TEXT("2x")))
				.OnClicked_Lambda([this]() { OnReplaySetSpeed(2.f); return FReply::Handled(); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
			[
				SNew(SButton).Text(FText::FromString(TEXT("4x")))
				.OnClicked_Lambda([this]() { OnReplaySetSpeed(4.f); return FReply::Handled(); })
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(10, 4)
		[
			SNew(SButton).Text(FText::FromString(TEXT("Next Round")))
			.OnClicked_Lambda([this]() { OnNextRound(); return FReply::Handled(); })
		];
}

TSharedRef<SWidget> UAWHUDWidget::BuildReplayBrowser()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(10)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("REPLAY BROWSER")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
			.ColorAndOpacity(FSlateColor(AWUIColors::TextPrimary))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(10, 4)
		[
			SNew(SButton).Text(FText::FromString(TEXT("< Back to Menu")))
			.OnClicked_Lambda([this]() { ShowScreen(EAWScreen::MainMenu); return FReply::Handled(); })
		];
}

TSharedRef<SWidget> UAWHUDWidget::BuildLanguageReference()
{
	// Generated directly from Core instruction/register definitions
	TSharedRef<SVerticalBox> Content = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(10)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("LANGUAGE REFERENCE")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
			.ColorAndOpacity(FSlateColor(AWUIColors::TextPrimary))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(10, 2)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Instructions (8 total):")))
			.ColorAndOpacity(FSlateColor(AWUIColors::TextSecondary))
		];

	// Instruction definitions from Core
	struct InstrDef { const TCHAR* Name; const TCHAR* Desc; int32 Energy; int32 Ticks; };
	const InstrDef Instrs[] = {
		{TEXT("MOVE"),   TEXT("Advance 1 cell in facing direction"), Automata::EnergyCost[0], Automata::TickCost[0]},
		{TEXT("TURN"),   TEXT("Turn 90 deg; operand -1(left) or 1(right)"), Automata::EnergyCost[1], Automata::TickCost[1]},
		{TEXT("SCAN"),   TEXT("Scan 90-deg cone; result in R0"), Automata::EnergyCost[2], Automata::TickCost[2]},
		{TEXT("FIRE"),   TEXT("Launch projectile in facing direction"), Automata::EnergyCost[3], Automata::TickCost[3]},
		{TEXT("SHIELD"), TEXT("Activate shield (absorbs next hit)"), Automata::EnergyCost[4], Automata::TickCost[4]},
		{TEXT("SET"),    TEXT("SET Rx, imm16 - load immediate into register"), Automata::EnergyCost[5], Automata::TickCost[5]},
		{TEXT("IF"),     TEXT("IF Rx cmp Ry/imm GOTO label - conditional jump"), Automata::EnergyCost[6], Automata::TickCost[6]},
		{TEXT("WAIT"),   TEXT("Do nothing for 1 tick"), Automata::EnergyCost[7], Automata::TickCost[7]},
	};
	static_assert(UE_ARRAY_COUNT(Instrs) == Automata::OpcodeCount, "Instruction count mismatch");

	for (const auto& I : Instrs)
	{
		FString Line = FString::Printf(TEXT("  %-8s  E:%d T:%d  %s"), I.Name, I.Energy, I.Ticks, I.Desc);
		Content->AddSlot().AutoHeight().Padding(10, 1)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Line))
			.Font(FCoreStyle::GetDefaultFontStyle("Mono", 11))
			.ColorAndOpacity(FSlateColor(AWUIColors::SyntaxInstruction))
		];
	}

	// Registers
	Content->AddSlot().AutoHeight().Padding(10, 8)
	[
		SNew(STextBlock)
		.Text(FText::FromString(FString::Printf(TEXT("Registers (%d total):"), Automata::TotalRegisterCount)))
		.ColorAndOpacity(FSlateColor(AWUIColors::TextSecondary))
	];

	struct RegDef { const TCHAR* Name; const TCHAR* Desc; };
	const RegDef Regs[] = {
		{TEXT("R0"), TEXT("General purpose (also SCAN result)")},
		{TEXT("R1"), TEXT("General purpose")},
		{TEXT("R2"), TEXT("General purpose")},
		{TEXT("R3"), TEXT("General purpose")},
		{TEXT("R_HP"), TEXT("Current hit-points (read-only)")},
		{TEXT("R_ENEMY_DIST"), TEXT("Manhattan distance to opponent (read-only)")},
		{TEXT("R_ENEMY_DIR"), TEXT("Relative direction to opponent 0-3 (read-only)")},
		{TEXT("R_ENERGY"), TEXT("Current energy (read-only)")},
		{TEXT("R_TICK"), TEXT("Current simulation tick (read-only)")},
	};
	static_assert(UE_ARRAY_COUNT(Regs) == Automata::TotalRegisterCount, "Register count mismatch");

	for (const auto& R : Regs)
	{
		FString Line = FString::Printf(TEXT("  %-14s  %s"), R.Name, R.Desc);
		Content->AddSlot().AutoHeight().Padding(10, 1)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Line))
			.Font(FCoreStyle::GetDefaultFontStyle("Mono", 11))
			.ColorAndOpacity(FSlateColor(AWUIColors::SyntaxRegister))
		];
	}

	Content->AddSlot().AutoHeight().Padding(10, 12)
	[
		SNew(SButton).Text(FText::FromString(TEXT("< Back")))
		.OnClicked_Lambda([this]() { ShowScreen(EAWScreen::MainMenu); return FReply::Handled(); })
	];

	return Content;
}

void UAWHUDWidget::OnLocalMatch()
{
	if (UAWGameSubsystem* Sub = GetSubsystem())
	{
		Sub->StartLocalMatch();
	}
	ShowScreen(EAWScreen::Programming);
}

void UAWHUDWidget::OnHostLAN()
{
	if (UAWGameSubsystem* Sub = GetSubsystem())
	{
		Sub->HostSession(TEXT("AutomataWar"));
	}
}

void UAWHUDWidget::OnFindLAN()
{
	if (UAWGameSubsystem* Sub = GetSubsystem())
	{
		Sub->RefreshSessions();
	}
}

void UAWHUDWidget::OnJoinIP(const FString& IP)
{
	if (UAWGameSubsystem* Sub = GetSubsystem())
	{
		Sub->JoinByIP(IP);
	}
}

void UAWHUDWidget::OnReplayBrowser()
{
	ShowScreen(EAWScreen::ReplayBrowser);
}

void UAWHUDWidget::OnLanguageRef()
{
	ShowScreen(EAWScreen::LanguageReference);
}

void UAWHUDWidget::OnQuit()
{
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			UKismetSystemLibrary::QuitGame(World, PC, EQuitPreference::Quit, false);
		}
	}
}

void UAWHUDWidget::OnSubmitSlot(int32 SlotIndex)
{
	if (UAWGameSubsystem* Sub = GetSubsystem())
	{
		FString Source;
		if (SlotIndex == 0 && EditorP1.IsValid()) Source = EditorP1->GetSourceText();
		else if (SlotIndex == 1 && EditorP2.IsValid()) Source = EditorP2->GetSourceText();

		FAWValidationResult Result = Sub->SubmitLocalScript(SlotIndex, Source);
		if (!Result.bSuccess)
		{
			UE_LOG(LogAutomataUI, Warning, TEXT("Submission slot %d failed: %s"), SlotIndex, *Result.ErrorMessage);
		}
	}
}

void UAWHUDWidget::OnLoadExample(int32 SlotIndex, const FString& ScriptName)
{
	FString Source;
	if (ScriptName == TEXT("Aggressor")) Source = FAWExampleScripts::Aggressor();
	else if (ScriptName == TEXT("Camper")) Source = FAWExampleScripts::Camper();
	else if (ScriptName == TEXT("Kiter")) Source = FAWExampleScripts::Kiter();
	else Source = FAWExampleScripts::DefaultBot();

	if (SlotIndex == 0 && EditorP1.IsValid()) EditorP1->SetSourceText(Source);
	else if (SlotIndex == 1 && EditorP2.IsValid()) EditorP2->SetSourceText(Source);
}

void UAWHUDWidget::OnTrainingBot(int32 SlotIndex)
{
	if (UAWGameSubsystem* Sub = GetSubsystem())
	{
		FString Source;
		if (SlotIndex == 0 && EditorP1.IsValid()) Source = EditorP1->GetSourceText();
		else if (SlotIndex == 1 && EditorP2.IsValid()) Source = EditorP2->GetSourceText();

		int32 Wins, Losses, Draws;
		Sub->RunTraining(Source, 10, Wins, Losses, Draws);
		UE_LOG(LogAutomataUI, Log, TEXT("Training: W%d L%d D%d"), Wins, Losses, Draws);
	}
}

void UAWHUDWidget::OnNextRound()
{
	if (UAWGameSubsystem* Sub = GetSubsystem())
	{
		Sub->NextRound();
	}
}

void UAWHUDWidget::OnReplayPlay() { bReplayPlaying = true; ReplayAccumulator = 0.0; }
void UAWHUDWidget::OnReplayPause() { bReplayPlaying = false; }

void UAWHUDWidget::OnReplayStepTick()
{
	++ReplayCurrentTick;
	// Arena renderer observes this tick via broadcast/polling
}

void UAWHUDWidget::OnReplayStepInstruction()
{
	// Step until the selected robot's instruction count changes
	// For now advance one tick (full implementation needs sim snapshot access)
	OnReplayStepTick();
}

void UAWHUDWidget::OnReplayStepBack()
{
	if (ReplayCurrentTick > 0) --ReplayCurrentTick;
	// Deterministic re-sim from tick 0 to ReplayCurrentTick is handled by arena renderer
}

void UAWHUDWidget::OnReplaySetSpeed(float Speed) { ReplaySpeed = Speed; }

void UAWHUDWidget::OnReplayScrub(int32 Tick)
{
	ReplayCurrentTick = FMath::Max(0, Tick);
}

/**
 * @file AWHUDCapture.cpp
 * @brief Non-shipping command-line driver for deterministic presentation captures.
 */

#include "AWHUDWidget.h"

#if !UE_BUILD_SHIPPING

#include "AWScreenWidget.h"
#include "AutomataWar/Game/AWPlayerController.h"
#include "AutomataWar/Visual/AWArenaRenderer.h"
#include "AutomataWar/Visual/AWItem.h"
#include "AutomataWar/Visual/AWVisualTypes.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "TimerManager.h"
#include "UnrealClient.h"

void UAWHUDWidget::RunCaptureMode()
{
    FString CaptureMode;
    if (!FParse::Value(FCommandLine::Get(), TEXT("AutomataCapture="), CaptureMode))
        return;

    float ScreenshotDelay = 1.f;
    FParse::Value(FCommandLine::Get(), TEXT("AutomataCaptureDelay="), ScreenshotDelay);
    if (CaptureMode.Equals(TEXT("Programming"), ESearchCase::IgnoreCase))
    {
        StartLocalMatch();
    }
    else if (CaptureMode.Equals(TEXT("ProgrammingExpanded"), ESearchCase::IgnoreCase))
    {
        bSinglePlayer = true;
        PendingDifficulty = EAWDifficulty::Normal;
        OnStartMatch(EAWArenaSize::Expanded);
    }
    else if (CaptureMode.Equals(TEXT("Difficulty"), ESearchCase::IgnoreCase))
    {
        OnSinglePlayerNav();
    }
    else if (CaptureMode.Equals(TEXT("LocalVersusDifficulty"), ESearchCase::IgnoreCase))
    {
        OnLocalMatch();
    }
    else if (CaptureMode.Equals(TEXT("LocalVersusArena"), ESearchCase::IgnoreCase))
    {
        OnLocalMatch();
        OnDifficultySelected(EAWDifficulty::Normal);
    }
    else if (CaptureMode.Equals(TEXT("ArenaSelection"), ESearchCase::IgnoreCase))
    {
        OnDifficultySelected(EAWDifficulty::Normal);
    }
    else if (CaptureMode.Equals(TEXT("SinglePlayerReplay"), ESearchCase::IgnoreCase))
    {
        bSinglePlayer = true;
        PendingDifficulty = EAWDifficulty::Hard;
        OnStartMatch(EAWArenaSize::Standard);
        if (AAWPlayerController *PlayerController = Cast<AAWPlayerController>(GetOwningPlayer()))
            PlayerController->SubmitCommands(0, {EAWCommand::Wait});
    }
    else if (CaptureMode.Equals(TEXT("PlanProjection"), ESearchCase::IgnoreCase) ||
             CaptureMode.Equals(TEXT("PlanProjectionRemoved"), ESearchCase::IgnoreCase))
    {
        StartLocalMatch();
        if (UUserWidget *Panel = Cast<UUserWidget>(
                ReplayAutopsyScreenWidget->GetWidgetFromName(TEXT("ProgrammingP1PanelWidget"))))
        {
            const TCHAR *Buttons[] = {
                TEXT("ProgrammingMoveButton"), TEXT("ProgrammingTurnLeftButton"),
                TEXT("ProgrammingMoveButton"), TEXT("ProgrammingFireButton"),
                TEXT("ProgrammingChargeShieldButton")};
            for (const TCHAR *ButtonName : Buttons)
                if (UButton *Button = Cast<UButton>(Panel->GetWidgetFromName(ButtonName)))
                    Button->OnClicked.Broadcast();

            if (CaptureMode.Equals(TEXT("PlanProjectionRemoved"), ESearchCase::IgnoreCase))
                if (UButton *RemoveButton = Cast<UButton>(
                        Panel->GetWidgetFromName(TEXT("ProgrammingRemoveActionButton"))))
                {
                    RemoveButton->OnClicked.Broadcast();
                    RemoveButton->OnClicked.Broadcast();
                }
        }
        ScreenshotDelay = 1.2f;
    }
    else if (CaptureMode.Equals(TEXT("ProgrammingSubmitted"), ESearchCase::IgnoreCase) ||
             CaptureMode.Equals(TEXT("ProgrammingReturned"), ESearchCase::IgnoreCase))
    {
        StartLocalMatch();
        if (UUserWidget *Panel = Cast<UUserWidget>(
                ReplayAutopsyScreenWidget->GetWidgetFromName(TEXT("ProgrammingP1PanelWidget"))))
        {
            if (UButton *MoveButton = Cast<UButton>(Panel->GetWidgetFromName(TEXT("ProgrammingMoveButton"))))
                MoveButton->OnClicked.Broadcast();
            if (UButton *SubmitButton = Cast<UButton>(Panel->GetWidgetFromName(TEXT("ProgrammingSubmitButton"))))
                SubmitButton->OnClicked.Broadcast();

            if (CaptureMode.Equals(TEXT("ProgrammingReturned"), ESearchCase::IgnoreCase))
            {
                ScreenshotDelay = 1.4f;
                TWeakObjectPtr<UUserWidget> WeakPanel = Panel;
                FTimerHandle ReturnTimer;
                GetWorld()->GetTimerManager().SetTimer(
                    ReturnTimer,
                    FTimerDelegate::CreateWeakLambda(this, [WeakPanel]()
                                                     {
                                                         if (UUserWidget *ProgrammingPanel = WeakPanel.Get())
                                                             if (UButton *ReturnButton = Cast<UButton>(ProgrammingPanel->GetWidgetFromName(TEXT("ProgrammingReturnToPlanningButton"))))
                                                                 ReturnButton->OnClicked.Broadcast(); }),
                    0.7f, false);
            }
        }
    }
    else if (CaptureMode.Equals(TEXT("Replay"), ESearchCase::IgnoreCase))
    {
        StartLocalMatch();
        if (AAWPlayerController *PlayerController = Cast<AAWPlayerController>(GetOwningPlayer()))
        {
            PlayerController->SubmitCommands(0, {EAWCommand::Fire, EAWCommand::Move, EAWCommand::Fire});
            PlayerController->SubmitCommands(1, {EAWCommand::TurnRight, EAWCommand::Move, EAWCommand::Fire});
        }
    }
    else if (CaptureMode.Equals(TEXT("MatchResult"), ESearchCase::IgnoreCase) ||
             CaptureMode.Equals(TEXT("MatchResultReturn"), ESearchCase::IgnoreCase))
    {
        StartLocalMatch();
        ShowScreen(EAWScreen::ReplayAutopsy);
        if (MatchResultScrim)
            MatchResultScrim->SetVisibility(ESlateVisibility::Visible);
        if (MatchResultPopupWidget)
            MatchResultPopupWidget->ShowResult(0, 0, EAWMatchEndReason::Health, true);
        if (MatchResultPopupWidgetPlayerTwo)
            MatchResultPopupWidgetPlayerTwo->ShowResult(0, 1, EAWMatchEndReason::Health, true);
        if (CaptureMode.Equals(TEXT("MatchResultReturn"), ESearchCase::IgnoreCase))
        {
            ScreenshotDelay = 1.4f;
            TWeakObjectPtr<UAWMatchResultPopupWidget> WeakPopup = MatchResultPopupWidget;
            FTimerHandle ReturnTimer;
            GetWorld()->GetTimerManager().SetTimer(
                ReturnTimer,
                FTimerDelegate::CreateWeakLambda(this, [WeakPopup]()
                                                 {
                                                     if (UAWMatchResultPopupWidget *Popup = WeakPopup.Get())
                                                         if (UButton *ReturnButton = Cast<UButton>(Popup->GetWidgetFromName(TEXT("MatchResultReturnButton"))))
                                                             ReturnButton->OnClicked.Broadcast(); }),
                0.7f, false);
        }
    }
    else if (CaptureMode.Equals(TEXT("MuzzleVFX"), ESearchCase::IgnoreCase) ||
             CaptureMode.Equals(TEXT("ImpactVFX"), ESearchCase::IgnoreCase))
    {
        StartLocalMatch();
        if (AAWPlayerController *PlayerController = Cast<AAWPlayerController>(GetOwningPlayer()))
        {
            PlayerController->SubmitCommands(0, {EAWCommand::Fire});
            PlayerController->SubmitCommands(1, {EAWCommand::Fire});
        }
    }
    else if (CaptureMode.Equals(TEXT("ShieldVFX"), ESearchCase::IgnoreCase))
    {
        StartLocalMatch();
        if (AAWPlayerController *PlayerController = Cast<AAWPlayerController>(GetOwningPlayer()))
        {
            PlayerController->SubmitCommands(0, {EAWCommand::ChargeShield});
            PlayerController->SubmitCommands(1, {EAWCommand::Wait});
        }
    }
    else if (CaptureMode.Equals(TEXT("PickupVFX"), ESearchCase::IgnoreCase))
    {
        StartLocalMatch();
        AAWArenaRenderer *Renderer = FindOrSpawnRenderer();
        UClass *ItemClass = LoadClass<AAWItem>(nullptr, TEXT("/Game/Blueprints/Items/BP_ActionPointItem.BP_ActionPointItem_C"));
        if (Renderer && ItemClass)
        {
            const FVector ArenaCenter(8.f * AWVisualConfig::CellSize, 8.f * AWVisualConfig::CellSize, 42.f);
            AAWItem *Item = GetWorld()->SpawnActor<AAWItem>(
                ItemClass, Renderer->GetActorLocation() + ArenaCenter, FRotator::ZeroRotator);
            FTimerHandle PickupTimer;
            GetWorld()->GetTimerManager().SetTimer(
                PickupTimer,
                FTimerDelegate::CreateWeakLambda(Item, [Item]()
                                                 { Item->SetCollected(true, true); }),
                FMath::Max(0.05f, ScreenshotDelay - 0.15f), false);
        }
    }

    if (!FParse::Param(FCommandLine::Get(), TEXT("AutomataCaptureScreenshot")))
        return;

    const FString ScreenshotPath = FPaths::ProjectSavedDir() / TEXT("Screenshots") /
                                   FString::Printf(TEXT("HUD_%s.png"), *CaptureMode);
    FTimerHandle ScreenshotTimer;
    GetWorld()->GetTimerManager().SetTimer(
        ScreenshotTimer,
        FTimerDelegate::CreateLambda([ScreenshotPath]()
                                     { FScreenshotRequest::RequestScreenshot(ScreenshotPath, true, false); }),
        ScreenshotDelay, false);
    if (FParse::Param(FCommandLine::Get(), TEXT("AutomataCaptureExit")))
    {
        FTimerHandle ExitTimer;
        GetWorld()->GetTimerManager().SetTimer(
            ExitTimer,
            FTimerDelegate::CreateLambda([]()
                                         { FPlatformMisc::RequestExit(false); }),
            ScreenshotDelay + 0.75f, false);
    }
}

#else

void UAWHUDWidget::RunCaptureMode()
{
}

#endif
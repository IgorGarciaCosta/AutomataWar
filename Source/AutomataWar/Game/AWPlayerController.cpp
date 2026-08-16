#include "AWPlayerController.h"
#include "AWGameMode.h"
#include "AWPlayerState.h"
#include "AutomataWar/UI/AWHUDWidget.h"
#include "AutomataWar/Visual/AWIsometricCamera.h"
#include "AutomataWar/Visual/AWVisualTypes.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

AAWPlayerController::AAWPlayerController()
{
    bShowMouseCursor = true;
    static ConstructorHelpers::FClassFinder<UAWHUDWidget> HUDWidgetBlueprint(TEXT("/Game/UI/WBP_AWHUD"));
    HUDWidgetClass = UAWHUDWidget::StaticClass();
    if (HUDWidgetBlueprint.Succeeded())
        HUDWidgetClass = HUDWidgetBlueprint.Class;

    static ConstructorHelpers::FClassFinder<UUserWidget> CursorWidgetBlueprint(TEXT("/Game/UI/WBP_AWCursor"));
    if (CursorWidgetBlueprint.Succeeded())
        CursorWidgetClass = CursorWidgetBlueprint.Class;
}

void AAWPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (IsLocalController())
    {
        if (CursorWidgetClass && GetWorld()->GetGameViewport())
        {
            CursorWidget = CreateWidget<UUserWidget>(this, CursorWidgetClass);
            for (EMouseCursor::Type CursorType : {
                     EMouseCursor::Default, EMouseCursor::Hand, EMouseCursor::TextEditBeam})
            {
                GetWorld()->GetGameViewport()->SetSoftwareCursorWidget(CursorType, CursorWidget);
            }
            GetWorld()->GetGameViewport()->SetUseSoftwareCursorWidgets(true);
        }

        AAWIsometricCamera *ArenaCamera = nullptr;
        for (TActorIterator<AAWIsometricCamera> It(GetWorld()); It; ++It)
        {
            ArenaCamera = *It;
            break;
        }
        if (ArenaCamera)
        {
            ArenaCamera->FrameArena(Automata::DefaultGridWidth, Automata::DefaultGridHeight, AWVisualConfig::CellSize);
        }

        HUDWidget = CreateWidget<UAWHUDWidget>(this, HUDWidgetClass);
        if (HUDWidget)
        {
            HUDWidget->SetArenaRenderTarget(ArenaCamera ? ArenaCamera->GetArenaRenderTarget() : nullptr);
            HUDWidget->AddToViewport(0);
        }

        FInputModeUIOnly InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        SetInputMode(InputMode);
        SetShowMouseCursor(true);
    }
}

void AAWPlayerController::SubmitCommands(const TArray<EAWCommand> &Commands)
{
    if (HasAuthority())
    {
        // Local or listen-server host: call GameMode directly
        if (AAWGameMode *GM = GetWorld()->GetAuthGameMode<AAWGameMode>())
        {
            AAWPlayerState *PS = GetPlayerState<AAWPlayerState>();
            if (PS)
            {
                FAWValidationResult Result = GM->HandleSubmission(PS->CommandSlot, Commands);
                Client_SubmissionResult(Result.bSuccess, Result.ErrorMessage);
            }
        }
    }
    else
    {
        Server_SubmitCommands(Commands);
    }
}

void AAWPlayerController::Server_SubmitCommands_Implementation(const TArray<EAWCommand> &Commands)
{
    AAWGameMode *GM = GetWorld()->GetAuthGameMode<AAWGameMode>();
    AAWPlayerState *PS = GetPlayerState<AAWPlayerState>();
    if (!GM || !PS)
        return;

    FAWValidationResult Result = GM->HandleSubmission(PS->CommandSlot, Commands);
    Client_SubmissionResult(Result.bSuccess, Result.ErrorMessage);
}

void AAWPlayerController::Client_SubmissionResult_Implementation(bool bSuccess, const FString &ErrorMessage)
{
    FAWValidationResult Result;
    Result.bSuccess = bSuccess;
    Result.ErrorMessage = ErrorMessage;

    if (AAWPlayerState *PS = GetPlayerState<AAWPlayerState>())
    {
        PS->LastError = ErrorMessage;
    }

    OnSubmissionResult.Broadcast(Result);
}

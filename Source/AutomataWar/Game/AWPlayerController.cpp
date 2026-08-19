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

void AAWPlayerController::SubmitCommands(int32 LocalSlot, const TArray<EAWCommand> &Commands)
{
    if (!HasAuthority())
    {
        Server_SubmitCommands(Commands);
        return;
    }

    const int32 Slot = ResolveCommandSlot(LocalSlot);
    FAWValidationResult Result;
    if (AAWGameMode *GM = GetWorld()->GetAuthGameMode<AAWGameMode>())
        Result = GM->HandleSubmission(Slot, Commands);
    else
        Result.ErrorMessage = TEXT("No authoritative GameMode.");
    Client_SubmissionResult(Slot, Result.bSuccess, Result.ErrorMessage);
}

void AAWPlayerController::Server_SubmitCommands_Implementation(const TArray<EAWCommand> &Commands)
{
    SubmitCommands(INDEX_NONE, Commands);
}

void AAWPlayerController::WithdrawCommands(int32 LocalSlot)
{
    if (!HasAuthority())
    {
        Server_WithdrawCommands();
        return;
    }

    const int32 Slot = ResolveCommandSlot(LocalSlot);
    FAWValidationResult Result;
    if (AAWGameMode *GM = GetWorld()->GetAuthGameMode<AAWGameMode>())
        Result = GM->WithdrawSubmission(Slot);
    else
        Result.ErrorMessage = TEXT("No authoritative GameMode.");
    Client_WithdrawalResult(Slot, Result.bSuccess, Result.ErrorMessage);
}

void AAWPlayerController::Server_WithdrawCommands_Implementation()
{
    WithdrawCommands(INDEX_NONE);
}

void AAWPlayerController::Client_SubmissionResult_Implementation(int32 Slot, bool bSuccess, const FString &ErrorMessage)
{
    FAWValidationResult Result;
    Result.bSuccess = bSuccess;
    Result.ErrorMessage = ErrorMessage;
    OnSubmissionResult.Broadcast(Slot, Result);
}

void AAWPlayerController::Client_WithdrawalResult_Implementation(int32 Slot, bool bSuccess, const FString &ErrorMessage)
{
    FAWValidationResult Result;
    Result.bSuccess = bSuccess;
    Result.ErrorMessage = ErrorMessage;
    OnWithdrawalResult.Broadcast(Slot, Result);
}

int32 AAWPlayerController::ResolveCommandSlot(int32 LocalSlot) const
{
    if (GetNetMode() == NM_Standalone)
        return LocalSlot;
    if (const AAWPlayerState *PS = GetPlayerState<AAWPlayerState>())
        return PS->CommandSlot;
    return INDEX_NONE;
}

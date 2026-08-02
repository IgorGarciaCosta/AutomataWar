#include "AWPlayerController.h"
#include "AWGameMode.h"
#include "AWPlayerState.h"
#include "AutomataWar/UI/AWHUDWidget.h"
#include "Blueprint/UserWidget.h"

AAWPlayerController::AAWPlayerController()
{
	bShowMouseCursor = true;
}

void AAWPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		HUDWidget = CreateWidget<UAWHUDWidget>(this, UAWHUDWidget::StaticClass());
		if (HUDWidget)
		{
			HUDWidget->AddToViewport(0);
		}

		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		SetShowMouseCursor(true);
	}
}

void AAWPlayerController::SubmitScript(const FString& Source)
{
	if (HasAuthority())
	{
		// Local or listen-server host: call GameMode directly
		if (AAWGameMode* GM = GetWorld()->GetAuthGameMode<AAWGameMode>())
		{
			AAWPlayerState* PS = GetPlayerState<AAWPlayerState>();
			if (PS)
			{
				FAWValidationResult Result = GM->HandleSubmission(PS->ScriptSlot, Source);
				Client_SubmissionResult(Result.bSuccess, Result.ErrorMessage);
			}
		}
	}
	else
	{
		Server_SubmitScript(Source);
	}
}

void AAWPlayerController::Server_SubmitScript_Implementation(const FString& Source)
{
	AAWGameMode* GM = GetWorld()->GetAuthGameMode<AAWGameMode>();
	AAWPlayerState* PS = GetPlayerState<AAWPlayerState>();
	if (!GM || !PS) return;

	FAWValidationResult Result = GM->HandleSubmission(PS->ScriptSlot, Source);
	Client_SubmissionResult(Result.bSuccess, Result.ErrorMessage);
}

void AAWPlayerController::Client_SubmissionResult_Implementation(bool bSuccess, const FString& ErrorMessage)
{
	FAWValidationResult Result;
	Result.bSuccess = bSuccess;
	Result.ErrorMessage = ErrorMessage;

	if (AAWPlayerState* PS = GetPlayerState<AAWPlayerState>())
	{
		PS->LastError = ErrorMessage;
	}

	OnSubmissionResult.Broadcast(Result);
}

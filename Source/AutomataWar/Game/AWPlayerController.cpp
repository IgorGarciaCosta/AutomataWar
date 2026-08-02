#include "AWPlayerController.h"
#include "AWGameMode.h"
#include "AWPlayerState.h"
#include "AutomataWar/UI/AWHUDWidget.h"
#include "AutomataWar/Visual/AWIsometricCamera.h"
#include "AutomataWar/Visual/AWVisualTypes.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "EngineUtils.h"

AAWPlayerController::AAWPlayerController()
{
    bShowMouseCursor = true;
}

void AAWPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (IsLocalController())
    {
        AAWIsometricCamera *ArenaCamera = nullptr;
        for (TActorIterator<AAWIsometricCamera> It(GetWorld()); It; ++It)
        {
            ArenaCamera = *It;
            break;
        }
        if (!ArenaCamera)
        {
            ArenaCamera = GetWorld()->SpawnActor<AAWIsometricCamera>();
        }
        if (ArenaCamera)
        {
            ArenaCamera->FrameArena(Automata::DefaultGridWidth, Automata::DefaultGridHeight, AWVisualConfig::CellSize);
            SetViewTarget(ArenaCamera);
        }

        bool bHasKeyLight = false;
        for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
        {
            bHasKeyLight = true;
            break;
        }
        if (!bHasKeyLight)
        {
            ADirectionalLight *KeyLight = GetWorld()->SpawnActor<ADirectionalLight>(FVector::ZeroVector, FRotator(-52.f, -38.f, 0.f));
            if (KeyLight && KeyLight->GetLightComponent())
            {
                KeyLight->GetLightComponent()->SetIntensity(7.5f);
                KeyLight->GetLightComponent()->SetLightColor(FLinearColor(0.82f, 0.9f, 1.f));
            }
        }

        bool bHasSkyLight = false;
        for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It)
        {
            bHasSkyLight = true;
            break;
        }
        if (!bHasSkyLight)
        {
            ASkyLight *SkyLight = GetWorld()->SpawnActor<ASkyLight>();
            if (SkyLight && SkyLight->GetLightComponent())
            {
                SkyLight->GetLightComponent()->SetIntensity(0.65f);
                SkyLight->GetLightComponent()->SetLightColor(FLinearColor(0.38f, 0.45f, 0.55f));
                SkyLight->GetLightComponent()->RecaptureSky();
            }
        }

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

void AAWPlayerController::SubmitScript(const FString &Source)
{
    if (HasAuthority())
    {
        // Local or listen-server host: call GameMode directly
        if (AAWGameMode *GM = GetWorld()->GetAuthGameMode<AAWGameMode>())
        {
            AAWPlayerState *PS = GetPlayerState<AAWPlayerState>();
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

void AAWPlayerController::Server_SubmitScript_Implementation(const FString &Source)
{
    AAWGameMode *GM = GetWorld()->GetAuthGameMode<AAWGameMode>();
    AAWPlayerState *PS = GetPlayerState<AAWPlayerState>();
    if (!GM || !PS)
        return;

    FAWValidationResult Result = GM->HandleSubmission(PS->ScriptSlot, Source);
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

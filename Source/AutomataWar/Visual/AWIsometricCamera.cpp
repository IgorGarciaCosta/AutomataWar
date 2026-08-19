/**
 * @file AWIsometricCamera.cpp
 * @brief Implementation of the isometric camera for arena viewing.
 */

#include "AWIsometricCamera.h"
#include "AWVisualTypes.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"

AAWIsometricCamera::AAWIsometricCamera()
{
    PrimaryActorTick.bCanEverTick = false;

    USceneComponent *Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(Root);
    Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);

    ArenaCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("ArenaCapture"));
    ArenaCapture->SetupAttachment(Root);
    ArenaCapture->ProjectionType = ECameraProjectionMode::Orthographic;
    ArenaCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    ArenaCapture->bCaptureEveryFrame = true;
    ArenaCapture->bCaptureOnMovement = true;
}

void AAWIsometricCamera::FrameArena(int32 GridWidth, int32 GridHeight, float CellSize)
{
    const float ArenaWidth = GridWidth * CellSize;
    const float ArenaHeight = GridHeight * CellSize;
    const FVector Center(ArenaWidth * 0.5f, ArenaHeight * 0.5f, 0.f);
    const float ArenaSpan = FMath::Max(ArenaWidth, ArenaHeight);
    const float FramingWidth = ArenaSpan * 1.08f;

    Camera->SetOrthoWidth(FramingWidth);
    ArenaCapture->OrthoWidth = FramingWidth;
    SetActorLocation(Center + FVector(0.f, 0.f, ArenaSpan * 0.5f));
    SetActorRotation(FRotator(-90.f, 0.f, 0.f));
    EnsureRenderTarget();
}

void AAWIsometricCamera::EnsureRenderTarget()
{
    if (ArenaRenderTarget)
        return;

    ArenaRenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("ArenaRenderTarget"));
    ArenaRenderTarget->ClearColor = FLinearColor::Black;
    ArenaRenderTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8_SRGB;
    ArenaRenderTarget->InitAutoFormat(1024, 1024);
    ArenaRenderTarget->UpdateResourceImmediate(true);
    ArenaCapture->TextureTarget = ArenaRenderTarget;
}

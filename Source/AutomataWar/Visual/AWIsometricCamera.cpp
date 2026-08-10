/**
 * @file AWIsometricCamera.cpp
 * @brief Implementation of the isometric camera for arena viewing.
 */

#include "AWIsometricCamera.h"
#include "AWVisualTypes.h"
#include "Camera/CameraComponent.h"

AAWIsometricCamera::AAWIsometricCamera()
{
    PrimaryActorTick.bCanEverTick = false;

    USceneComponent *Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(Root);
    Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);
}

void AAWIsometricCamera::FrameArena(int32 GridWidth, int32 GridHeight, float CellSize)
{
    const float ArenaWidth = GridWidth * CellSize;
    const float ArenaHeight = GridHeight * CellSize;
    const FVector Center(ArenaWidth * 0.5f, ArenaHeight * 0.5f, 0.f);
    const float ArenaSpan = FMath::Max(ArenaWidth, ArenaHeight);

    Camera->SetOrthoWidth(ArenaSpan * 2.0f);
    SetActorLocation(Center + FVector(0.f, 0.f, ArenaSpan * 0.5f));
    SetActorRotation(FRotator(-90.f, 0.f, 0.f));
}

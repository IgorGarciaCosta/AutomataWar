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

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Root);
	Camera->SetProjectionMode(ECameraProjectionMode::Perspective);
	Camera->SetFieldOfView(45.f);
}

void AAWIsometricCamera::FrameArena(int32 GridWidth, int32 GridHeight, float CellSize)
{
	float ArenaW = GridWidth * CellSize;
	float ArenaH = GridHeight * CellSize;
	FVector Center(ArenaW * 0.5f, ArenaH * 0.5f, 0.f);

	// Isometric: 45-deg yaw, ~35-deg pitch from above
	float Distance = FMath::Max(ArenaW, ArenaH) * 1.2f;
	FVector Offset(-Distance * 0.7f, -Distance * 0.7f, Distance * 0.8f);

	SetActorLocation(Center + Offset);
	SetActorRotation((Center - GetActorLocation()).Rotation());
}

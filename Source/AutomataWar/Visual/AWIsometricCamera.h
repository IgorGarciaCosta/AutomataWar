#pragma once

/**
 * @file AWIsometricCamera.h
 * @brief Fixed isometric camera actor framing the entire arena.
 *
 * Positioned to view the full 16x16 grid from a 45-degree isometric angle.
 * No player control; purely presentation. Set as view target at match start.
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AWIsometricCamera.generated.h"

class UCameraComponent;

/**
 * @brief Isometric camera providing a top-down-angled view of the arena.
 */
UCLASS()
class AUTOMATAWAR_API AAWIsometricCamera : public AActor
{
    GENERATED_BODY()

public:
    AAWIsometricCamera();

    /** Get the camera component. */
    UCameraComponent *GetCameraComponent() const { return Camera; }

    /** Reposition to frame a grid of given dimensions. */
    void FrameArena(int32 GridWidth, int32 GridHeight, float CellSize);

protected:
    UPROPERTY(VisibleAnywhere, Category = "Camera")
    TObjectPtr<UCameraComponent> Camera;
};

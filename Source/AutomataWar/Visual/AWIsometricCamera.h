#pragma once

/**
 * @file AWIsometricCamera.h
 * @brief Fixed top-down camera actor framing the arena inside the HUD viewport.
 *
 * Uses orthographic projection so the tactical grid has no perspective distortion.
 * No player control; purely presentation. Set as view target at match start.
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AWIsometricCamera.generated.h"

class UCameraComponent;

/**
 * @brief Orthographic camera providing a direct top-down view of the arena.
 */
UCLASS(Blueprintable)
class AUTOMATAWAR_API AAWIsometricCamera : public AActor
{
    GENERATED_BODY()

public:
    /** Create the fixed orthographic camera component. */
    AAWIsometricCamera();

    /** Get the camera component. */
    UCameraComponent *GetCameraComponent() const { return Camera; }

    /** Reposition to frame a grid of given dimensions. */
    void FrameArena(int32 GridWidth, int32 GridHeight, float CellSize);

protected:
    UPROPERTY(VisibleAnywhere, Category = "Camera")
    TObjectPtr<UCameraComponent> Camera;
};

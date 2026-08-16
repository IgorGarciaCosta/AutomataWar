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
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

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

    /** Return the square arena feed consumed by the gameplay HUD. */
    UTextureRenderTarget2D *GetArenaRenderTarget() const { return ArenaRenderTarget; }

    /** Reposition to frame a grid of given dimensions. */
    void FrameArena(int32 GridWidth, int32 GridHeight, float CellSize);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<UCameraComponent> Camera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<USceneCaptureComponent2D> ArenaCapture;

private:
    /** Allocate the transient square render target once per camera instance. */
    void EnsureRenderTarget();

    UPROPERTY(Transient)
    TObjectPtr<UTextureRenderTarget2D> ArenaRenderTarget;
};

#pragma once

/**
 * @file AWPlanProjectionComponent.h
 * @brief Owns local-only plan ghost actors, trails, aim lines, and shield previews.
 */

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AutomataWar/Core/Sim/AutomataSimulation.h"
#include "AWPlanProjectionComponent.generated.h"

class AAWArenaRenderer;
class AAWTankActor;
class UStaticMeshComponent;

/**
 * Presentation component responsible for the complete lifecycle of both plan projections.
 * It is created by AAWArenaRenderer and consumes deterministic snapshots without mutating them.
 */
UCLASS(ClassGroup = (AutomataWar))
class AUTOMATAWAR_API UAWPlanProjectionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    /** Disable ticking because projections rebuild only when command queues change. */
    UAWPlanProjectionComponent();

    /** Destroy projection actors before the owning renderer leaves the world. */
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** Rebuild one player's tank ghost, movement trail, firing vectors, and shield preview. */
    void UpdateProjection(int32 RobotIndex, const Automata::RobotState &InitialRobot,
                          const TArray<Automata::StepSnapshot> &Snapshots,
                          const TArray<Automata::SimEvent> &Events, int32 AnimatedStep);

    /** Hide or reveal a retained projection without rebuilding its geometry. */
    void SetProjectionVisible(int32 RobotIndex, bool bVisible);

    /** Destroy one retained projection and all runtime-created geometry it owns. */
    void ClearProjection(int32 RobotIndex);

    /** Destroy both retained projections before replay or arena reinitialization. */
    void ClearAllProjections();

private:
    /** Return the owning arena renderer or null when detached. */
    AAWArenaRenderer *GetRenderer() const;

    /** Resolve or spawn the ghost tank used by one projection. */
    AAWTankActor *EnsureProjectionTank(int32 RobotIndex);

    /** Create one translucent world-space beam between deterministic grid positions. */
    UStaticMeshComponent *CreateBeam(const FVector &Start, const FVector &End,
                                     const FLinearColor &Color, float Opacity);

    /** Create or remove the shield preview attached to one ghost tank. */
    void SetProjectionShield(int32 RobotIndex, bool bActive);

    /** Destroy runtime trail, aim, and shield components for one projection. */
    void ClearProjectionGeometry(int32 RobotIndex);

    TWeakObjectPtr<AAWTankActor> ProjectionTanks[2];
    TArray<TWeakObjectPtr<UStaticMeshComponent>> TrailComponents[2];
    TArray<TWeakObjectPtr<UStaticMeshComponent>> AimComponents[2];
    TWeakObjectPtr<UStaticMeshComponent> ShieldEffects[2];
    bool bProjectionVisible[2] = {false, false};
};
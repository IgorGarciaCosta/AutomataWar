#pragma once

/**
 * @file AWArenaRenderer.h
 * @brief Actor that generates a variable-size floor grid, cover objects, robots,
 *        projectiles, and VFX from simulation snapshots. Purely presentational.
 *
 * Spawned by GameMode or placed in level. Reads simulation state via snapshot arrays;
 * never writes to Core. All collision disabled.
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AutomataWar/Core/Sim/AutomataSimulation.h"
#include "AWArenaRenderer.generated.h"

class UProceduralMeshComponent;
class UActorComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UNiagaraComponent;
class AAWAPItemSpawner;
class AAWTankActor;
class ATableObstable;

/**
 * @brief Presentation-only arena actor: grid floor, cover, projectile bolts,
 *        VFX, and audio driven entirely from sim event/snapshot data.
 */
UCLASS(Blueprintable)
class AUTOMATAWAR_API AAWArenaRenderer : public AActor
{
    GENERATED_BODY()

public:
    /** Create presentation components with collision and gameplay feedback disabled. */
    AAWArenaRenderer();

    /** Build initial visual state after the actor enters the world. */
    virtual void BeginPlay() override;
    /** Advance presentation-only projectile bolts and their growing shot traces. */
    virtual void Tick(float DeltaTime) override;
    /** Initialize the grid and cover visuals from a sim config. */
    void InitializeArena(const Automata::SimConfig &Config, const TArray<Automata::CellType> &Grid,
                         const TArray<Automata::SimEvent> &Events);

    /** Set the current step snapshot for interpolation display. */
    void SetSnapshot(const Automata::StepSnapshot &Snapshot);

    /** Reconcile shield visuals with effects after end-of-round duration decay. */
    void SetFinalEffects(const std::array<FAWRobotEffects, 2> &FinalEffects);

    /** Process a batch of sim events for VFX/audio triggers. */
    void ProcessEvents(const TArray<Automata::SimEvent> &Events, int32 FromStep, int32 ToStep);

    /** Reset visuals to initial state. */
    void ResetVisuals();

    /** Rebuild one player's local-only tank ghost, movement trail, and firing vectors. */
    void UpdatePlanProjection(int32 RobotIndex, const Automata::RobotState &InitialRobot,
                              const TArray<Automata::StepSnapshot> &Snapshots,
                              const TArray<Automata::SimEvent> &Events, int32 AnimatedStep);

    /** Hide or restore a retained plan without discarding its queue-derived geometry. */
    void SetPlanProjectionVisible(int32 RobotIndex, bool bVisible);

    /** Destroy one retained plan projection. */
    void ClearPlanProjection(int32 RobotIndex);

    /** Destroy every retained plan projection before replay or a new arena is initialized. */
    void ClearAllPlanProjections();

protected:
    /** Build the floor grid mesh. */
    void BuildFloorGrid(int32 Width, int32 Height);

    /** Resize the level-authored table and move decorative actors outside the active grid. */
    void ResizeArenaPresentation(int32 Width, int32 Height);

    /** Spawn cover block visuals. */
    void SpawnCoverVisuals(int32 Width, int32 Height, const TArray<Automata::CellType> &Grid);

    /** Trigger muzzle flash VFX on a tank's cannon socket. */
    void TriggerMuzzleFlash(int32 RobotIdx);
    void TriggerMuzzleFlash(AAWTankActor *Tank);

    /** Trigger impact VFX at position. */
    void TriggerImpact(FVector WorldPos);

    /** Trigger destruction VFX at position. */
    void TriggerDestruction(FVector WorldPos);

    /** Create or destroy the energy sphere owned by one tank's active shield state. */
    void SetShieldActive(int32 RobotIdx, bool bActive);

    /** Apply final shield state once all projectiles for the last replay step arrive. */
    void ApplyPendingFinalShieldState();

    /** Begin a visible projectile whose gameplay result has already been resolved. */
    void SpawnProjectile(FVector Start, FVector End, int32 TargetRobot,
                         bool bShielded, bool bDestroyedTarget);

    /** Resize and orient a beam component between two world-space points. */
    void UpdateProjectileBeam(UStaticMeshComponent *Beam, const FVector &Start, const FVector &End);

    /** Spawn a point-light fallback and remove it after its visual lifetime. */
    void SpawnTransientLight(FVector WorldPos, float Intensity, float Radius, FColor Color, float Lifespan);

    /** Remove a runtime-created component after its visual lifetime. */
    void ScheduleComponentDestruction(UActorComponent *Component, float Lifespan);

    /** Play optional sound at location with soft-path fallback silence. */
    void PlaySFX(const TCHAR *SoftPath, FVector Location);

    /** Convert grid coords to world position. */
    FVector GridToWorld(int32 X, int32 Y) const;

    /** Convert direction enum to world rotation. */
    FRotator DirToRotation(Automata::Dir D) const;

    /** Resolve the two level-authored tank actors when references are unset. */
    void ResolveTankActors();

    /** Resolve or create the actor that owns AP pickup visuals. */
    void ResolveActionPointItemSpawner();

    AAWTankActor *EnsurePlanProjectionTank(int32 RobotIndex);
    UStaticMeshComponent *CreatePlanBeam(const FVector &Start, const FVector &End,
                                         const FLinearColor &Color, float Opacity);
    void SetPlanProjectionShield(int32 RobotIndex, bool bActive);
    void ClearPlanProjectionGeometry(int32 RobotIndex);

    UPROPERTY(VisibleAnywhere, Category = "Arena")
    TObjectPtr<UProceduralMeshComponent> FloorMesh;

    /** Tank instances authored directly in the arena level. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Arena|Actors")
    TObjectPtr<AAWTankActor> PlayerOneTank;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Arena|Actors")
    TObjectPtr<AAWTankActor> PlayerTwoTank;

    /** Blueprint class used for runtime obstacle actors. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arena|Actors")
    TSubclassOf<ATableObstable> ObstacleClass;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Arena|Actors")
    TObjectPtr<AAWAPItemSpawner> ActionPointItemSpawner;

    /** Current display snapshot. */
    Automata::StepSnapshot CurrentSnapshot;

    UPROPERTY(Transient)
    TMap<int32, TObjectPtr<ATableObstable>> Obstacles;

    /** Original dressing transforms retained so repeated replay initialization cannot accumulate offsets. */
    TMap<TWeakObjectPtr<AActor>, FTransform> AuthoredDressingTransforms;

    bool bHasSnapshot = false;

    /** Runtime-only state for one in-flight replay projectile. */
    struct FProjectileVisual
    {
        TWeakObjectPtr<UStaticMeshComponent> Bolt;
        TWeakObjectPtr<UStaticMeshComponent> Beam;
        TWeakObjectPtr<UNiagaraComponent> Trail;
        FVector Start = FVector::ZeroVector;
        FVector End = FVector::ZeroVector;
        float Elapsed = 0.f;
        float Duration = 0.f;
        int32 TargetRobot = INDEX_NONE;
        bool bShielded = false;
        bool bShieldRemainsActive = false;
        bool bDestroyedTarget = false;
    };

    TArray<FProjectileVisual> Projectiles;
    TWeakObjectPtr<UStaticMeshComponent> ShieldEffects[2];
    bool PendingFinalShieldState[2] = {false, false};
    bool bHasPendingFinalShieldState = false;

    TWeakObjectPtr<AAWTankActor> PlanProjectionTanks[2];
    TArray<TWeakObjectPtr<UStaticMeshComponent>> PlanTrailComponents[2];
    TArray<TWeakObjectPtr<UStaticMeshComponent>> PlanAimComponents[2];
    TWeakObjectPtr<UStaticMeshComponent> PlanShieldEffects[2];
    bool bPlanProjectionVisible[2] = {false, false};
};

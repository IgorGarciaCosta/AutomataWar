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
class AAWAPItemSpawner;
class AAWTankActor;
class ATableObstable;
class UAWCombatEffectsComponent;
class UAWPlanProjectionComponent;

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
    /** Initialize the grid and cover visuals from a sim config. */
    void InitializeArena(const Automata::SimConfig &Config, const TArray<Automata::CellType> &Grid,
                         const TArray<Automata::SimEvent> &Events);

    /** Set the current step snapshot for interpolation display. */
    void SetSnapshot(const Automata::StepSnapshot &Snapshot);

    /** Restore replay-visible damage to the initialized round state and cancel in-flight effects. */
    void ResetDamagePresentation();

    /** Restore replay-visible damage to a prior snapshot and cancel in-flight effects. */
    void ResetDamagePresentation(const Automata::StepSnapshot &Snapshot);

    /** Return tank health after projectile arrivals rather than canonical step execution. */
    int32 GetPresentedRobotHealth(int32 RobotIndex) const;

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
    friend class UAWCombatEffectsComponent;
    friend class UAWPlanProjectionComponent;

    /** Build the floor grid mesh. */
    void BuildFloorGrid(int32 Width, int32 Height);

    /** Resize the level-authored table and move decorative actors outside the active grid. */
    void ResizeArenaPresentation(int32 Width, int32 Height);

    /** Spawn cover block visuals. */
    void SpawnCoverVisuals(int32 Width, int32 Height, const TArray<Automata::CellType> &Grid);

    /** Trigger muzzle flash VFX on a concrete tank's cannon socket. */
    void TriggerMuzzleFlash(AAWTankActor *Tank);

    /** Convert grid coords to world position. */
    FVector GridToWorld(int32 X, int32 Y) const;

    /** Convert direction enum to world rotation. */
    FRotator DirToRotation(Automata::Dir D) const;

    /** Resolve the two level-authored tank actors when references are unset. */
    void ResolveTankActors();

    /** Resolve or create the actor that owns AP pickup visuals. */
    void ResolveActionPointItemSpawner();

    UPROPERTY(VisibleAnywhere, Category = "Arena")
    TObjectPtr<UProceduralMeshComponent> FloorMesh;

    /** Component owning transient projectiles, shields, VFX, lights, and spatial audio. */
    UPROPERTY(VisibleAnywhere, Category = "Arena|Presentation")
    TObjectPtr<UAWCombatEffectsComponent> CombatEffects;

    /** Component owning local-only plan ghosts and their runtime geometry. */
    UPROPERTY(VisibleAnywhere, Category = "Arena|Presentation")
    TObjectPtr<UAWPlanProjectionComponent> PlanProjection;

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

    UPROPERTY(Transient)
    TMap<int32, TObjectPtr<ATableObstable>> Obstacles;

    /** Width used to translate shot coordinates into obstacle map indices. */
    int32 ArenaGridWidth = 0;

    /** Round-start cover health used when replay navigation returns to step zero. */
    std::vector<int32_t> InitialObstacleHealth;

    /** Round-start tank health used when replay navigation returns to step zero. */
    std::array<int32_t, 2> InitialRobotHealth = {Automata::MaxHP, Automata::MaxHP};

    /** Original dressing transforms retained so repeated replay initialization cannot accumulate offsets. */
    TMap<TWeakObjectPtr<AActor>, FTransform> AuthoredDressingTransforms;
};

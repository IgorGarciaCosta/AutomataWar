#pragma once

/**
 * @file AWArenaRenderer.h
 * @brief Actor that generates the visual 16x16 floor grid, cover objects, robots,
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
class UAudioComponent;
class UNiagaraComponent;
class AAWTankActor;

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
    void InitializeArena(const Automata::SimConfig &Config, const TArray<Automata::CellType> &Grid);

    /** Set the current snapshot for interpolation display. */
    void SetSnapshot(const Automata::TickSnapshot &Snapshot);

    /** Process a batch of sim events for VFX/audio triggers. */
    void ProcessEvents(const TArray<Automata::SimEvent> &Events, int32 FromTick, int32 ToTick);

    /** Reset visuals to initial state. */
    void ResetVisuals();

protected:
    /** Build the floor grid mesh. */
    void BuildFloorGrid(int32 Width, int32 Height);

    /** Spawn cover block visuals. */
    void SpawnCoverVisuals(int32 Width, int32 Height, const TArray<Automata::CellType> &Grid);

    /** Spawn a projectile bolt visual. */
    void SpawnProjectileBolt(int32 OwnerIdx, FVector WorldPos, FVector Direction);

    /** Trigger muzzle flash VFX on a tank's cannon socket. */
    void TriggerMuzzleFlash(int32 RobotIdx);

    /** Trigger impact VFX at position. */
    void TriggerImpact(FVector WorldPos);

    /** Trigger shield bubble VFX on robot. */
    void TriggerShieldBubble(int32 RobotIdx);

    /** Trigger destruction VFX at position. */
    void TriggerDestruction(FVector WorldPos);

    /** Spawn a point-light fallback and remove it after its visual lifetime. */
    void SpawnTransientLight(FVector WorldPos, float Intensity, float Radius, FColor Color, float Lifespan);

    /** Remove a runtime-created component after its visual lifetime. */
    void ScheduleComponentDestruction(UActorComponent *Component, float Lifespan);

    /** Play optional sound at location with soft-path fallback silence. */
    void PlaySFX(const TCHAR *SoftPath, FVector Location);

    void StartMovementSound(int32 RobotIdx);
    void StopMovementSound(int32 RobotIdx);

    /** Convert grid coords to world position. */
    FVector GridToWorld(int32 X, int32 Y) const;

    /** Convert direction enum to world rotation. */
    FRotator DirToRotation(Automata::Dir D) const;

    /** Resolve the two level-authored tank actors when references are unset. */
    void ResolveTankActors();

    UPROPERTY(VisibleAnywhere, Category = "Arena")
    TObjectPtr<UProceduralMeshComponent> FloorMesh;

    /** Tank instances authored directly in the arena level. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Arena|Actors")
    TObjectPtr<AAWTankActor> PlayerOneTank;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Arena|Actors")
    TObjectPtr<AAWTankActor> PlayerTwoTank;

    /** Current display snapshot. */
    Automata::TickSnapshot CurrentSnapshot;
    TWeakObjectPtr<UAudioComponent> MovementAudio[2];
    bool bHasSnapshot = false;
};

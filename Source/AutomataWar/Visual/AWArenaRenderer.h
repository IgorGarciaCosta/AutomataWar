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
class UStaticMeshComponent;
class UPointLightComponent;
class UNiagaraComponent;

/**
 * @brief Presentation-only arena actor: grid floor, cover blocks, robot visuals,
 *        projectile bolts, VFX, and audio driven entirely from sim event/snapshot data.
 */
UCLASS()
class AUTOMATAWAR_API AAWArenaRenderer : public AActor
{
    GENERATED_BODY()

public:
    AAWArenaRenderer();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

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

    /** Build robot composite meshes. */
    void BuildRobotVisuals();

    /** Spawn a projectile bolt visual. */
    void SpawnProjectileBolt(int32 OwnerIdx, FVector WorldPos, FVector Direction);

    /** Trigger muzzle flash VFX at position. */
    void TriggerMuzzleFlash(FVector WorldPos);

    /** Trigger impact VFX at position. */
    void TriggerImpact(FVector WorldPos);

    /** Trigger shield bubble VFX on robot. */
    void TriggerShieldBubble(int32 RobotIdx);

    /** Trigger destruction VFX at position. */
    void TriggerDestruction(FVector WorldPos);

    /** Play optional sound at location with soft-path fallback silence. */
    void PlaySFX(const TCHAR *SoftPath, FVector Location);

    /** Convert grid coords to world position. */
    FVector GridToWorld(int32 X, int32 Y) const;

    /** Convert direction enum to world rotation. */
    FRotator DirToRotation(Automata::Dir D) const;

    UPROPERTY(VisibleAnywhere, Category = "Arena")
    TObjectPtr<UProceduralMeshComponent> FloorMesh;

    UPROPERTY(VisibleAnywhere, Category = "Arena")
    TObjectPtr<USceneComponent> RobotRoot0;

    UPROPERTY(VisibleAnywhere, Category = "Arena")
    TObjectPtr<USceneComponent> RobotRoot1;

    UPROPERTY(VisibleAnywhere, Category = "Arena")
    TObjectPtr<UPointLightComponent> PlayerOneAccentLight;

    UPROPERTY(VisibleAnywhere, Category = "Arena")
    TObjectPtr<UPointLightComponent> PlayerTwoAccentLight;

    /** Dynamic material instances for robots. */
    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> RobotMat0;
    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> RobotMat1;

    /** Current display snapshot. */
    Automata::TickSnapshot CurrentSnapshot;
    /** Target positions for interpolation. */
    FVector TargetPos0;
    FVector TargetPos1;
    FRotator TargetRot0;
    FRotator TargetRot1;

    int32 GridWidth = 0;
    int32 GridHeight = 0;
};

#pragma once

/**
 * @file AWCombatEffectsComponent.h
 * @brief Owns transient replay projectiles, shields, VFX, lights, and spatial audio.
 */

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AutomataWar/Core/Sim/AutomataSimulation.h"
#include "AWCombatEffectsComponent.generated.h"

class AAWArenaRenderer;
class AAWTankActor;
class UNiagaraComponent;
class UStaticMeshComponent;

/**
 * Presentation component that translates deterministic combat events into transient effects.
 * It ticks only presentation state and never mutates simulation snapshots or gameplay authority.
 */
UCLASS(ClassGroup = (AutomataWar))
class AUTOMATAWAR_API UAWCombatEffectsComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    /** Enable component ticking for in-flight projectile interpolation. */
    UAWCombatEffectsComponent();

    /** Remove transient components and tank-attached shields when the renderer ends play. */
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** Advance transient projectile visuals and dispatch arrival effects. */
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction *ThisTickFunction) override;

    /** Retain the current canonical snapshot and reconcile active shield visuals. */
    void SetSnapshot(const Automata::StepSnapshot &Snapshot);

    /** Cancel transient effects and restore the replay-visible tank health baseline. */
    void ResetDamagePresentation(const std::array<int32, 2> &RobotHealth);

    /** Return health after projectile arrivals rather than ahead-of-animation simulation state. */
    int32 GetPresentedRobotHealth(int32 RobotIndex) const;

    /** Process replay events in the requested inclusive step range. */
    void ProcessEvents(const TArray<Automata::SimEvent> &Events, int32 FromStep, int32 ToStep);

    /** Reconcile shields after end-of-round duration decay once projectiles arrive. */
    void SetFinalEffects(const std::array<FAWRobotEffects, 2> &FinalEffects);

    /** Destroy all transient effects and clear shield state. */
    void ResetEffects();

    /** Trigger the configured muzzle flash on a concrete tank actor. */
    void TriggerMuzzleFlash(AAWTankActor *Tank);

private:
    /** Return the owning arena renderer or null when detached. */
    AAWArenaRenderer *GetRenderer() const;

    /** Trigger impact VFX or its light fallback at a world position. */
    void TriggerImpact(FVector WorldPosition);

    /** Trigger layered destruction VFX or its light fallback at a world position. */
    void TriggerDestruction(FVector WorldPosition);

    /** Create or remove the energy sphere representing one tank's active shield. */
    void SetShieldActive(int32 RobotIndex, bool bActive);

    /** Apply deferred final shield state after every in-flight projectile completes. */
    void ApplyPendingFinalShieldState();

    /** Begin a projectile whose deterministic gameplay result is already known. */
    void SpawnProjectile(FVector Start, FVector End, int32 TargetRobot,
                         int32 TargetObstacleCell, int32 Damage, bool bShielded);

    /** Resize and orient a beam component between two world-space points. */
    static void UpdateBeam(UStaticMeshComponent *Beam, const FVector &Start, const FVector &End);

    /** Spawn a point-light fallback and remove it after its visual lifetime. */
    void SpawnTransientLight(FVector WorldPosition, float Intensity, float Radius,
                             FColor Color, float Lifespan);

    /** Remove a runtime-created component after its visual lifetime. */
    void ScheduleComponentDestruction(UActorComponent *Component, float Lifespan);

    /** Play optional spatial audio at a world position. */
    void PlaySFX(const TCHAR *AssetPath, FVector Location);

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
        int32 TargetObstacleCell = INDEX_NONE;
        int32 Damage = 0;
        bool bShielded = false;
        bool bShieldRemainsActive = false;
    };

    /** Apply one projectile's known damage when its presentation reaches the target. */
    bool ApplyProjectileDamage(const FProjectileVisual &Projectile);

    Automata::StepSnapshot CurrentSnapshot;
    std::array<int32, 2> PresentedRobotHealth;
    TArray<FProjectileVisual> Projectiles;
    TWeakObjectPtr<UStaticMeshComponent> ShieldEffects[2];
    bool PendingFinalShieldState[2] = {false, false};
    bool bHasPendingFinalShieldState = false;
};
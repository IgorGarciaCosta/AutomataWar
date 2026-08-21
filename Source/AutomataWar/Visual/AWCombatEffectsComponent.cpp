/**
 * @file AWCombatEffectsComponent.cpp
 * @brief Transient deterministic-event presentation implementation.
 */

#include "AWCombatEffectsComponent.h"
#include "AWArenaRenderer.h"
#include "AWTankActor.h"
#include "AWVisualTypes.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

UAWCombatEffectsComponent::UAWCombatEffectsComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UAWCombatEffectsComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ResetEffects();
    Super::EndPlay(EndPlayReason);
}

void UAWCombatEffectsComponent::TickComponent(
    float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    for (int32 Index = Projectiles.Num() - 1; Index >= 0; --Index)
    {
        FProjectileVisual &Projectile = Projectiles[Index];
        Projectile.Elapsed += DeltaTime;
        const float Alpha = FMath::Clamp(Projectile.Elapsed / Projectile.Duration, 0.f, 1.f);
        const FVector Position = FMath::Lerp(Projectile.Start, Projectile.End, Alpha);
        if (UStaticMeshComponent *Bolt = Projectile.Bolt.Get())
            Bolt->SetWorldLocation(Position);
        UpdateBeam(Projectile.Beam.Get(), Projectile.Start, Position);

        if (Alpha < 1.f)
            continue;

        if (Projectile.bShielded)
            SetShieldActive(Projectile.TargetRobot, Projectile.bShieldRemainsActive);
        TriggerImpact(Projectile.End);
        PlaySFX(AWVisualAssets::SFX_Impact, Projectile.End);
        if (Projectile.bDestroyedTarget)
        {
            TriggerDestruction(Projectile.End);
            PlaySFX(AWVisualAssets::SFX_Destroy, Projectile.End);
        }
        if (UNiagaraComponent *Trail = Projectile.Trail.Get())
            Trail->DeactivateImmediate();
        if (UStaticMeshComponent *Bolt = Projectile.Bolt.Get())
            Bolt->DestroyComponent();
        if (UStaticMeshComponent *Beam = Projectile.Beam.Get())
            Beam->DestroyComponent();
        Projectiles.RemoveAtSwap(Index);
    }

    if (Projectiles.IsEmpty() && bHasPendingFinalShieldState)
        ApplyPendingFinalShieldState();
}

AAWArenaRenderer *UAWCombatEffectsComponent::GetRenderer() const
{
    return Cast<AAWArenaRenderer>(GetOwner());
}

void UAWCombatEffectsComponent::SetSnapshot(const Automata::StepSnapshot &Snapshot)
{
    CurrentSnapshot = Snapshot;
    bHasPendingFinalShieldState = false;
    for (int32 RobotIndex = 0; RobotIndex < 2; ++RobotIndex)
        SetShieldActive(RobotIndex, HasActiveShield(Snapshot.robots[RobotIndex].effects));
}

void UAWCombatEffectsComponent::ProcessEvents(
    const TArray<Automata::SimEvent> &Events, int32 FromStep, int32 ToStep)
{
    AAWArenaRenderer *Renderer = GetRenderer();
    if (!Renderer)
        return;

    TMap<int32, FVector> ShotStarts;
    TSet<int32> ShieldedTargets;
    for (const Automata::SimEvent &Event : Events)
    {
        if (Event.step < FromStep || Event.step > ToStep)
            continue;

        const Automata::RobotState &Robot = CurrentSnapshot.robots[Event.robot];
        const FVector Position = Renderer->GridToWorld(Robot.x, Robot.y);
        switch (Event.type)
        {
        case Automata::EventType::Fire:
        {
            Renderer->ResolveTankActors();
            AAWTankActor *Tank = Event.robot == 0 ? Renderer->PlayerOneTank.Get() : Renderer->PlayerTwoTank.Get();
            const FTransform MuzzleTransform = Tank
                                                   ? Tank->GetMuzzleTransform()
                                                   : FTransform(Renderer->DirToRotation(Robot.facing),
                                                                Position + FVector(0, 0, AWVisualConfig::ProjectileZ));
            ShotStarts.Add(Event.robot, MuzzleTransform.GetLocation());
            TriggerMuzzleFlash(Tank);
            PlaySFX(AWVisualAssets::SFX_Fire, MuzzleTransform.GetLocation());
            break;
        }
        case Automata::EventType::ShotBlocked:
        {
            const FVector Start = ShotStarts.FindRef(Event.robot);
            SpawnProjectile(Start.IsNearlyZero() ? Position : Start,
                            Renderer->GridToWorld(Event.paramA, Event.paramB),
                            INDEX_NONE, false, false);
            break;
        }
        case Automata::EventType::ShieldCharged:
            SetShieldActive(Event.robot, true);
            break;
        case Automata::EventType::ShieldAbsorbed:
            SetShieldActive(Event.robot, true);
            ShieldedTargets.Add(Event.robot);
            break;
        case Automata::EventType::Hit:
        {
            const int32 Shooter = Event.paramB;
            const FVector Start = ShotStarts.FindRef(Shooter);
            SpawnProjectile(
                Start.IsNearlyZero()
                    ? Renderer->GridToWorld(CurrentSnapshot.robots[Shooter].x,
                                            CurrentSnapshot.robots[Shooter].y)
                    : Start,
                Position, Event.robot, ShieldedTargets.Contains(Event.robot),
                CurrentSnapshot.robots[Event.robot].hp <= 0);
            break;
        }
        default:
            break;
        }
    }
}

void UAWCombatEffectsComponent::SetFinalEffects(
    const std::array<FAWRobotEffects, 2> &FinalEffects)
{
    for (int32 RobotIndex = 0; RobotIndex < 2; ++RobotIndex)
        PendingFinalShieldState[RobotIndex] = HasActiveShield(FinalEffects[RobotIndex]);
    bHasPendingFinalShieldState = true;
    if (Projectiles.IsEmpty())
        ApplyPendingFinalShieldState();
}

void UAWCombatEffectsComponent::ResetEffects()
{
    bHasPendingFinalShieldState = false;
    SetShieldActive(0, false);
    SetShieldActive(1, false);
    for (FProjectileVisual &Projectile : Projectiles)
    {
        if (UNiagaraComponent *Trail = Projectile.Trail.Get())
            Trail->DestroyComponent();
        if (UStaticMeshComponent *Bolt = Projectile.Bolt.Get())
            Bolt->DestroyComponent();
        if (UStaticMeshComponent *Beam = Projectile.Beam.Get())
            Beam->DestroyComponent();
    }
    Projectiles.Reset();
}

void UAWCombatEffectsComponent::TriggerMuzzleFlash(AAWTankActor *Tank)
{
    if (!Tank)
        return;

    UNiagaraSystem *System = LoadObject<UNiagaraSystem>(nullptr, AWVisualAssets::NS_MuzzleFlash);
    if (System)
    {
        if (UNiagaraComponent *MuzzleFlash = UNiagaraFunctionLibrary::SpawnSystemAttached(
                System, Tank->GetMuzzleComponent(), Tank->GetMuzzleSocketName(), FVector::ZeroVector,
                FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true))
            ScheduleComponentDestruction(MuzzleFlash, AWVisualConfig::MuzzleFlashLifespan);
    }
    else
    {
        SpawnTransientLight(Tank->GetMuzzleTransform().GetLocation(), 4000.f, 200.f,
                            FColor::Yellow, AWVisualConfig::TransientVFXLifespan);
    }
}

void UAWCombatEffectsComponent::TriggerImpact(FVector WorldPosition)
{
    UNiagaraSystem *System = LoadObject<UNiagaraSystem>(nullptr, AWVisualAssets::NS_Impact);
    if (System)
    {
        if (UNiagaraComponent *Impact = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                GetWorld(), System, WorldPosition, FRotator::ZeroRotator, FVector(0.8f)))
            ScheduleComponentDestruction(Impact, AWVisualConfig::ImpactVFXLifespan);
    }
    else
    {
        SpawnTransientLight(WorldPosition, 5000.f, 150.f, FColor::Orange,
                            AWVisualConfig::TransientVFXLifespan);
    }
}

void UAWCombatEffectsComponent::TriggerDestruction(FVector WorldPosition)
{
    UNiagaraSystem *System = LoadObject<UNiagaraSystem>(nullptr, AWVisualAssets::NS_Destruction);
    if (System)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(), System, WorldPosition, FRotator::ZeroRotator, FVector(1.45f), true, true);
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(), System, WorldPosition + FVector(0.f, 0.f, 35.f),
            FRotator(0.f, 90.f, 0.f), FVector(0.85f), true, true);
        SpawnTransientLight(WorldPosition, 18000.f, 500.f, FColor(255, 72, 18), 0.45f);
        SpawnTransientLight(WorldPosition + FVector(0.f, 0.f, 80.f),
                            7000.f, 300.f, FColor(255, 170, 60), 1.1f);
    }
    else
    {
        SpawnTransientLight(WorldPosition, 15000.f, 400.f, FColor::Red,
                            AWVisualConfig::TransientVFXLifespan * 2.f);
    }
}

void UAWCombatEffectsComponent::SetShieldActive(int32 RobotIndex, bool bActive)
{
    if (RobotIndex < 0 || RobotIndex > 1)
        return;
    if (!bActive)
    {
        if (UStaticMeshComponent *Shield = ShieldEffects[RobotIndex].Get())
            Shield->DestroyComponent();
        ShieldEffects[RobotIndex].Reset();
        return;
    }
    if (ShieldEffects[RobotIndex].IsValid())
        return;

    AAWArenaRenderer *Renderer = GetRenderer();
    if (!Renderer)
        return;
    Renderer->ResolveTankActors();
    AAWTankActor *Tank = RobotIndex == 0 ? Renderer->PlayerOneTank.Get() : Renderer->PlayerTwoTank.Get();
    UStaticMesh *Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    UMaterialInterface *ShieldMaterial = LoadObject<UMaterialInterface>(nullptr, AWVisualAssets::M_ShieldEnergy);
    if (!Tank || !Sphere || !ShieldMaterial)
        return;

    UStaticMeshComponent *Shield = NewObject<UStaticMeshComponent>(Tank);
    Shield->SetupAttachment(Tank->GetRootComponent());
    Shield->SetStaticMesh(Sphere);
    Shield->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Shield->SetCastShadow(false);
    Shield->SetReceivesDecals(false);
    Shield->SetRelativeScale3D(FVector(1.35f));
    Shield->SetTranslucentSortPriority(10);
    Tank->AddInstanceComponent(Shield);
    Shield->RegisterComponent();

    UMaterialInstanceDynamic *Material = UMaterialInstanceDynamic::Create(ShieldMaterial, Shield);
    Material->SetVectorParameterValue(TEXT("ShieldColor"), FLinearColor(0.02f, 4.f, 8.f, 1.f));
    Material->SetScalarParameterValue(TEXT("Opacity"), 0.22f);
    Shield->SetMaterial(0, Material);
    ShieldEffects[RobotIndex] = Shield;
}

void UAWCombatEffectsComponent::ApplyPendingFinalShieldState()
{
    for (int32 RobotIndex = 0; RobotIndex < 2; ++RobotIndex)
        SetShieldActive(RobotIndex, PendingFinalShieldState[RobotIndex]);
    bHasPendingFinalShieldState = false;
}

void UAWCombatEffectsComponent::SpawnProjectile(
    FVector Start, FVector End, int32 TargetRobot, bool bShielded, bool bDestroyedTarget)
{
    AAWArenaRenderer *Renderer = GetRenderer();
    UStaticMesh *Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    UStaticMesh *Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!Renderer || !Sphere || !Cube)
        return;

    UStaticMeshComponent *Bolt = NewObject<UStaticMeshComponent>(Renderer);
    Bolt->SetupAttachment(Renderer->GetRootComponent());
    Bolt->SetStaticMesh(Sphere);
    Bolt->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Bolt->SetWorldScale3D(FVector(0.11f));
    Bolt->SetWorldLocation(Start);
    Renderer->AddInstanceComponent(Bolt);
    Bolt->RegisterComponent();

    UStaticMeshComponent *Beam = NewObject<UStaticMeshComponent>(Renderer);
    Beam->SetupAttachment(Renderer->GetRootComponent());
    Beam->SetStaticMesh(Cube);
    Beam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Renderer->AddInstanceComponent(Beam);
    Beam->RegisterComponent();

    if (UMaterialInterface *EffectMaterial = LoadObject<UMaterialInterface>(nullptr, AWVisualAssets::M_Effect))
    {
        UMaterialInstanceDynamic *Material = UMaterialInstanceDynamic::Create(EffectMaterial, Renderer);
        Material->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(8.f, 1.8f, 0.15f, 1.f));
        Material->SetScalarParameterValue(TEXT("Opacity"), 0.92f);
        Bolt->SetMaterial(0, Material);
        Beam->SetMaterial(0, Material);
    }

    UNiagaraComponent *Trail = nullptr;
    if (UNiagaraSystem *TrailSystem = LoadObject<UNiagaraSystem>(nullptr, AWVisualAssets::NS_ProjectileTrail))
        Trail = UNiagaraFunctionLibrary::SpawnSystemAttached(
            TrailSystem, Bolt, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget, true);

    FProjectileVisual Projectile;
    Projectile.Bolt = Bolt;
    Projectile.Beam = Beam;
    Projectile.Trail = Trail;
    Projectile.Start = Start;
    Projectile.End = End;
    Projectile.Duration = FMath::Clamp(
        FVector::Distance(Start, End) / AWVisualConfig::ProjectileSpeed,
        AWVisualConfig::ProjectileMinDuration, AWVisualConfig::ProjectileMaxDuration);
    Projectile.TargetRobot = TargetRobot;
    Projectile.bShielded = bShielded;
    Projectile.bShieldRemainsActive = TargetRobot >= 0 && TargetRobot < 2 &&
                                      HasActiveShield(CurrentSnapshot.robots[TargetRobot].effects);
    Projectile.bDestroyedTarget = bDestroyedTarget;
    Projectiles.Add(Projectile);
    UpdateBeam(Beam, Start, Start);
}

void UAWCombatEffectsComponent::UpdateBeam(
    UStaticMeshComponent *Beam, const FVector &Start, const FVector &End)
{
    if (!Beam)
        return;

    const FVector Delta = End - Start;
    const float Length = Delta.Size();
    Beam->SetWorldLocation((Start + End) * 0.5f);
    Beam->SetWorldRotation(Delta.IsNearlyZero() ? FRotator::ZeroRotator : Delta.Rotation());
    Beam->SetWorldScale3D(FVector(
        FMath::Max(0.001f, Length / 100.f),
        AWVisualConfig::ProjectileBeamThickness,
        AWVisualConfig::ProjectileBeamThickness));
}

void UAWCombatEffectsComponent::SpawnTransientLight(
    FVector WorldPosition, float Intensity, float Radius, FColor Color, float Lifespan)
{
    AAWArenaRenderer *Renderer = GetRenderer();
    if (!Renderer)
        return;

    UPointLightComponent *Light = NewObject<UPointLightComponent>(Renderer);
    Light->SetupAttachment(Renderer->GetRootComponent());
    Light->SetWorldLocation(WorldPosition);
    Light->SetIntensity(Intensity);
    Light->SetAttenuationRadius(Radius);
    Light->SetLightColor(Color);
    Light->RegisterComponent();
    ScheduleComponentDestruction(Light, Lifespan);
}

void UAWCombatEffectsComponent::ScheduleComponentDestruction(
    UActorComponent *Component, float Lifespan)
{
    FTimerHandle Handle;
    TWeakObjectPtr<UActorComponent> WeakComponent = Component;
    GetWorld()->GetTimerManager().SetTimer(
        Handle, [WeakComponent]()
        { if (WeakComponent.IsValid()) WeakComponent->DestroyComponent(); }, Lifespan, false);
}

void UAWCombatEffectsComponent::PlaySFX(const TCHAR *AssetPath, FVector Location)
{
    if (USoundBase *Sound = LoadObject<USoundBase>(nullptr, AssetPath))
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), Sound, Location);
}
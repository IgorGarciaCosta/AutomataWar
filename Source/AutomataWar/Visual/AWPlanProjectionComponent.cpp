/**
 * @file AWPlanProjectionComponent.cpp
 * @brief Local-only plan projection presentation implementation.
 */

#include "AWPlanProjectionComponent.h"
#include "AWArenaRenderer.h"
#include "AWTankActor.h"
#include "AWVisualTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"

UAWPlanProjectionComponent::UAWPlanProjectionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UAWPlanProjectionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearAllProjections();
    Super::EndPlay(EndPlayReason);
}

AAWArenaRenderer *UAWPlanProjectionComponent::GetRenderer() const
{
    return Cast<AAWArenaRenderer>(GetOwner());
}

void UAWPlanProjectionComponent::UpdateProjection(
    int32 RobotIndex, const Automata::RobotState &InitialRobot,
    const TArray<Automata::StepSnapshot> &Snapshots,
    const TArray<Automata::SimEvent> &Events, int32 AnimatedStep)
{
    AAWArenaRenderer *Renderer = GetRenderer();
    if (!Renderer || RobotIndex < 0 || RobotIndex > 1 || Snapshots.IsEmpty())
    {
        ClearProjection(RobotIndex);
        return;
    }

    AAWTankActor *ProjectionTank = EnsureProjectionTank(RobotIndex);
    if (!ProjectionTank)
        return;

    ClearProjectionGeometry(RobotIndex);
    bProjectionVisible[RobotIndex] = true;
    ProjectionTank->SetActorHiddenInGame(false);

    const FVector GhostOffset(0.f, 0.f, 12.f);
    ProjectionTank->SetTargetTransform(
        Renderer->GridToWorld(InitialRobot.x, InitialRobot.y) + GhostOffset,
        Renderer->DirToRotation(InitialRobot.facing));
    const Automata::RobotState &FinalRobot = Snapshots.Last().robots[RobotIndex];
    ProjectionTank->SetTargetTransform(
        Renderer->GridToWorld(FinalRobot.x, FinalRobot.y) + GhostOffset,
        Renderer->DirToRotation(FinalRobot.facing));

    Renderer->ResolveTankActors();
    AAWTankActor *SourceTank = RobotIndex == 0 ? Renderer->PlayerOneTank.Get() : Renderer->PlayerTwoTank.Get();
    const FLinearColor PlanColor = SourceTank ? SourceTank->GetPlayerColor() : FLinearColor(0.f, 0.85f, 0.95f);

    int32 TrailX = InitialRobot.x;
    int32 TrailY = InitialRobot.y;
    for (const Automata::SimEvent &Event : Events)
    {
        if (Event.robot != RobotIndex || Event.type != Automata::EventType::Move)
            continue;

        FVector Start = Renderer->GridToWorld(TrailX, TrailY);
        FVector End = Renderer->GridToWorld(Event.paramA, Event.paramB);
        Start.Z = AWVisualConfig::FloorZ + 4.f;
        End.Z = Start.Z;
        if (UStaticMeshComponent *Beam = CreateBeam(Start, End, PlanColor, 0.62f))
            TrailComponents[RobotIndex].Add(Beam);
        TrailX = Event.paramA;
        TrailY = Event.paramB;
    }

    for (const Automata::SimEvent &FireEvent : Events)
    {
        if (FireEvent.robot != RobotIndex || FireEvent.type != Automata::EventType::Fire)
            continue;

        const Automata::StepSnapshot *ShotSnapshot = Snapshots.FindByPredicate(
            [&FireEvent](const Automata::StepSnapshot &Snapshot)
            { return Snapshot.step == FireEvent.step; });
        if (!ShotSnapshot)
            continue;

        const Automata::RobotState &Shooter = ShotSnapshot->robots[RobotIndex];
        const int32 Direction = static_cast<int32>(Shooter.facing);
        FVector Start = Renderer->GridToWorld(Shooter.x, Shooter.y);
        Start += FVector(Automata::DirDX[Direction], Automata::DirDY[Direction], 0.f) *
                 (AWVisualConfig::CellSize * 0.45f);
        Start.Z = AWVisualConfig::ProjectileZ;
        FVector End = Start;
        bool bFoundEnd = false;

        for (const Automata::SimEvent &ResultEvent : Events)
        {
            if (ResultEvent.step != FireEvent.step)
                continue;
            if (ResultEvent.type == Automata::EventType::ShotBlocked && ResultEvent.robot == RobotIndex)
            {
                End = Renderer->GridToWorld(ResultEvent.paramA, ResultEvent.paramB);
                bFoundEnd = true;
                break;
            }
            if (ResultEvent.type == Automata::EventType::Hit && ResultEvent.paramB == RobotIndex)
            {
                const Automata::RobotState &Target = ShotSnapshot->robots[ResultEvent.robot];
                End = Renderer->GridToWorld(Target.x, Target.y);
                bFoundEnd = true;
                break;
            }
        }

        if (bFoundEnd)
        {
            End.Z = Start.Z;
            if (UStaticMeshComponent *Beam = CreateBeam(Start, End, PlanColor, 0.82f))
                AimComponents[RobotIndex].Add(Beam);
        }

        if (FireEvent.step == AnimatedStep)
            Renderer->TriggerMuzzleFlash(ProjectionTank);
    }

    SetProjectionShield(RobotIndex, HasActiveShield(FinalRobot.effects));
}

void UAWPlanProjectionComponent::SetProjectionVisible(int32 RobotIndex, bool bVisible)
{
    if (RobotIndex < 0 || RobotIndex > 1)
        return;

    bProjectionVisible[RobotIndex] = bVisible;
    if (AAWTankActor *Tank = ProjectionTanks[RobotIndex].Get())
        Tank->SetActorHiddenInGame(!bVisible);
    for (TWeakObjectPtr<UStaticMeshComponent> Component : TrailComponents[RobotIndex])
        if (Component.IsValid())
            Component->SetVisibility(bVisible, true);
    for (TWeakObjectPtr<UStaticMeshComponent> Component : AimComponents[RobotIndex])
        if (Component.IsValid())
            Component->SetVisibility(bVisible, true);
    if (UStaticMeshComponent *Shield = ShieldEffects[RobotIndex].Get())
        Shield->SetVisibility(bVisible, true);
}

void UAWPlanProjectionComponent::ClearProjection(int32 RobotIndex)
{
    if (RobotIndex < 0 || RobotIndex > 1)
        return;

    ClearProjectionGeometry(RobotIndex);
    if (AAWTankActor *Tank = ProjectionTanks[RobotIndex].Get())
        Tank->Destroy();
    ProjectionTanks[RobotIndex].Reset();
    bProjectionVisible[RobotIndex] = false;
}

void UAWPlanProjectionComponent::ClearAllProjections()
{
    ClearProjection(0);
    ClearProjection(1);
}

AAWTankActor *UAWPlanProjectionComponent::EnsureProjectionTank(int32 RobotIndex)
{
    AAWArenaRenderer *Renderer = GetRenderer();
    if (!Renderer || RobotIndex < 0 || RobotIndex > 1)
        return nullptr;
    if (AAWTankActor *ExistingTank = ProjectionTanks[RobotIndex].Get())
        return ExistingTank;

    Renderer->ResolveTankActors();
    AAWTankActor *SourceTank = RobotIndex == 0 ? Renderer->PlayerOneTank.Get() : Renderer->PlayerTwoTank.Get();
    if (!SourceTank || !Renderer->GetWorld())
        return nullptr;

    const FTransform SpawnTransform = SourceTank->GetActorTransform();
    AAWTankActor *ProjectionTank = Renderer->GetWorld()->SpawnActorDeferred<AAWTankActor>(
        SourceTank->GetClass(), SpawnTransform, Renderer, nullptr,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!ProjectionTank)
        return nullptr;

    ProjectionTank->ConfigureAsPlanProjection(*SourceTank);
    UGameplayStatics::FinishSpawningActor(ProjectionTank, SpawnTransform);
    ProjectionTank->SetActorHiddenInGame(true);
    ProjectionTanks[RobotIndex] = ProjectionTank;
    return ProjectionTank;
}

UStaticMeshComponent *UAWPlanProjectionComponent::CreateBeam(
    const FVector &Start, const FVector &End, const FLinearColor &Color, float Opacity)
{
    AAWArenaRenderer *Renderer = GetRenderer();
    UStaticMesh *Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    UMaterialInterface *EffectMaterial = LoadObject<UMaterialInterface>(nullptr, AWVisualAssets::M_Effect);
    if (!Renderer || !Cube || !EffectMaterial)
        return nullptr;

    UStaticMeshComponent *Beam = NewObject<UStaticMeshComponent>(Renderer);
    Beam->SetupAttachment(Renderer->GetRootComponent());
    Beam->SetStaticMesh(Cube);
    Beam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Beam->SetCastShadow(false);
    Beam->SetReceivesDecals(false);
    Beam->SetTranslucentSortPriority(15);
    Renderer->AddInstanceComponent(Beam);
    Beam->RegisterComponent();

    UMaterialInstanceDynamic *Material = UMaterialInstanceDynamic::Create(EffectMaterial, Beam);
    Material->SetVectorParameterValue(TEXT("EmissiveColor"), Color * 7.f);
    Material->SetScalarParameterValue(TEXT("Opacity"), Opacity);
    Beam->SetMaterial(0, Material);
    const FVector Delta = End - Start;
    Beam->SetWorldLocation((Start + End) * 0.5f);
    Beam->SetWorldRotation(Delta.IsNearlyZero() ? FRotator::ZeroRotator : Delta.Rotation());
    Beam->SetWorldScale3D(FVector(
        FMath::Max(0.001f, Delta.Size() / 100.f),
        AWVisualConfig::ProjectileBeamThickness,
        AWVisualConfig::ProjectileBeamThickness));
    return Beam;
}

void UAWPlanProjectionComponent::SetProjectionShield(int32 RobotIndex, bool bActive)
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

    AAWTankActor *Tank = ProjectionTanks[RobotIndex].Get();
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
    Shield->SetTranslucentSortPriority(21);
    Tank->AddInstanceComponent(Shield);
    Shield->RegisterComponent();

    UMaterialInstanceDynamic *Material = UMaterialInstanceDynamic::Create(ShieldMaterial, Shield);
    Material->SetVectorParameterValue(TEXT("ShieldColor"), Tank->GetPlayerColor() * 5.f);
    Material->SetScalarParameterValue(TEXT("Opacity"), 0.24f);
    Shield->SetMaterial(0, Material);
    ShieldEffects[RobotIndex] = Shield;
}

void UAWPlanProjectionComponent::ClearProjectionGeometry(int32 RobotIndex)
{
    if (RobotIndex < 0 || RobotIndex > 1)
        return;
    for (TWeakObjectPtr<UStaticMeshComponent> Component : TrailComponents[RobotIndex])
        if (Component.IsValid())
            Component->DestroyComponent();
    for (TWeakObjectPtr<UStaticMeshComponent> Component : AimComponents[RobotIndex])
        if (Component.IsValid())
            Component->DestroyComponent();
    TrailComponents[RobotIndex].Reset();
    AimComponents[RobotIndex].Reset();
    SetProjectionShield(RobotIndex, false);
}
/**
 * @file AWArenaRenderer.cpp
 * @brief Implementation of the presentation-only arena renderer.
 */

#include "AWArenaRenderer.h"
#include "AWAPItemSpawner.h"
#include "AWIsometricCamera.h"
#include "AWTankActor.h"
#include "AWVisualTypes.h"
#include "TableObstable.h"
#include "AutomataWar/UI/AWUITypes.h"
#include "ProceduralMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Containers/Set.h"
#include "Sound/SoundBase.h"

namespace
{
    const FName ArenaGroundTag(TEXT("ArenaGround"));
    const FName ArenaFoundationTag(TEXT("ArenaFoundation"));
    const FName ArenaLegacyGridTag(TEXT("ArenaLegacyGrid"));
    const FName ArenaRailSouthTag(TEXT("Rail_South"));
    const FName ArenaRailNorthTag(TEXT("Rail_North"));
    const FName ArenaRailWestTag(TEXT("Rail_West"));
    const FName ArenaRailEastTag(TEXT("Rail_East"));
    const FName ArenaDressingTag(TEXT("ArenaDressing"));
}

AAWArenaRenderer::AAWArenaRenderer()
{
    PrimaryActorTick.bCanEverTick = true;

    USceneComponent *Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    Root->SetMobility(EComponentMobility::Static);

    FloorMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("FloorMesh"));
    FloorMesh->SetupAttachment(Root);
    FloorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FloorMesh->bUseComplexAsSimpleCollision = false;

    ObstacleClass = ATableObstable::StaticClass();
}

void AAWArenaRenderer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    for (int32 Index = Projectiles.Num() - 1; Index >= 0; --Index)
    {
        FProjectileVisual &Projectile = Projectiles[Index];
        Projectile.Elapsed += DeltaTime;
        const float Alpha = FMath::Clamp(Projectile.Elapsed / Projectile.Duration, 0.f, 1.f);
        const FVector Position = FMath::Lerp(Projectile.Start, Projectile.End, Alpha);
        if (UStaticMeshComponent *Bolt = Projectile.Bolt.Get())
            Bolt->SetWorldLocation(Position);
        UpdateProjectileBeam(Projectile.Beam.Get(), Projectile.Start, Position);

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

void AAWArenaRenderer::BeginPlay()
{
    Super::BeginPlay();
    ResolveTankActors();
    ResolveActionPointItemSpawner();
}

void AAWArenaRenderer::InitializeArena(const Automata::SimConfig &Config, const TArray<Automata::CellType> &Grid,
                                       const TArray<Automata::SimEvent> &Events)
{
    ResizeArenaPresentation(Config.gridWidth, Config.gridHeight);
    BuildFloorGrid(Config.gridWidth, Config.gridHeight);
    SpawnCoverVisuals(Config.gridWidth, Config.gridHeight, Grid);
    for (TActorIterator<AAWIsometricCamera> It(GetWorld()); It; ++It)
    {
        It->FrameArena(Config.gridWidth, Config.gridHeight, AWVisualConfig::CellSize);
        break;
    }
    ResolveActionPointItemSpawner();
    if (ActionPointItemSpawner)
        ActionPointItemSpawner->InitializeItems(Config, Grid, Events);
}

void AAWArenaRenderer::ResizeArenaPresentation(int32 Width, int32 Height)
{
    if (!GetWorld() || Width <= 0 || Height <= 0)
        return;

    const float CellSize = AWVisualConfig::CellSize;
    const FVector ArenaOrigin = GetActorLocation();
    const FVector ArenaCenter = ArenaOrigin + FVector(Width * CellSize * 0.5f, Height * CellSize * 0.5f, 0.f);
    const FVector AuthoredCenter = ArenaOrigin + FVector(
                                                     Automata::DefaultGridWidth * CellSize * 0.5f,
                                                     Automata::DefaultGridHeight * CellSize * 0.5f, 0.f);
    const float WidthRatio = static_cast<float>(Width) / Automata::DefaultGridWidth;
    const float HeightRatio = static_cast<float>(Height) / Automata::DefaultGridHeight;
    const bool bUseAuthoredGrid = Width == Automata::DefaultGridWidth && Height == Automata::DefaultGridHeight;

    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor *Actor = *It;
        FVector Location = Actor->GetActorLocation();
        FVector Scale = Actor->GetActorScale3D();

        if (Actor->ActorHasTag(ArenaGroundTag))
        {
            Actor->SetActorLocation(FVector(ArenaCenter.X, ArenaCenter.Y, Location.Z));
            Actor->SetActorScale3D(FVector(Width * 2.f, Height * 2.f, Scale.Z));
        }
        else if (Actor->ActorHasTag(ArenaFoundationTag))
        {
            Actor->SetActorLocation(FVector(ArenaCenter.X, ArenaCenter.Y, Location.Z));
            Actor->SetActorScale3D(FVector(Width + 2.f, Height + 2.f, Scale.Z));
        }
        else if (Actor->ActorHasTag(ArenaLegacyGridTag))
        {
            Actor->SetActorHiddenInGame(!bUseAuthoredGrid);
        }
        else if (Actor->ActorHasTag(ArenaRailSouthTag) || Actor->ActorHasTag(ArenaRailNorthTag))
        {
            const bool bNorth = Actor->ActorHasTag(ArenaRailNorthTag);
            Actor->SetActorLocation(FVector(ArenaCenter.X, ArenaOrigin.Y + (bNorth ? Height * CellSize + 75.f : -75.f), Location.Z));
            Actor->SetActorScale3D(FVector(Width + 2.f, Scale.Y, Scale.Z));
        }
        else if (Actor->ActorHasTag(ArenaRailWestTag) || Actor->ActorHasTag(ArenaRailEastTag))
        {
            const bool bEast = Actor->ActorHasTag(ArenaRailEastTag);
            Actor->SetActorLocation(FVector(ArenaOrigin.X + (bEast ? Width * CellSize + 75.f : -75.f), ArenaCenter.Y, Location.Z));
            Actor->SetActorScale3D(FVector(Scale.X, Height + 2.f, Scale.Z));
        }
        else if (Actor->ActorHasTag(ArenaDressingTag))
        {
            const FTransform &AuthoredTransform = AuthoredDressingTransforms.FindOrAdd(Actor, Actor->GetActorTransform());
            const FVector AuthoredLocation = AuthoredTransform.GetLocation();
            Actor->SetActorLocation(FVector(
                ArenaCenter.X + (AuthoredLocation.X - AuthoredCenter.X) * WidthRatio,
                ArenaCenter.Y + (AuthoredLocation.Y - AuthoredCenter.Y) * HeightRatio,
                AuthoredLocation.Z));
        }
    }
}

void AAWArenaRenderer::SetSnapshot(const Automata::StepSnapshot &Snapshot)
{
    CurrentSnapshot = Snapshot;
    bHasSnapshot = true;
    bHasPendingFinalShieldState = false;
    for (const TPair<int32, TObjectPtr<ATableObstable>> &Entry : Obstacles)
        if (Entry.Value && Entry.Key >= 0 && static_cast<size_t>(Entry.Key) < Snapshot.obstacleHealth.size())
            Entry.Value->SetHealth(Snapshot.obstacleHealth[static_cast<size_t>(Entry.Key)]);

    ResolveTankActors();
    if (PlayerOneTank)
        PlayerOneTank->SetTargetTransform(GridToWorld(Snapshot.robots[0].x, Snapshot.robots[0].y), DirToRotation(Snapshot.robots[0].facing));
    if (PlayerTwoTank)
        PlayerTwoTank->SetTargetTransform(GridToWorld(Snapshot.robots[1].x, Snapshot.robots[1].y), DirToRotation(Snapshot.robots[1].facing));
    for (int32 RobotIndex = 0; RobotIndex < 2; ++RobotIndex)
        SetShieldActive(RobotIndex, HasActiveShield(Snapshot.robots[RobotIndex].effects));
    const int32 ActiveRobot = Snapshot.robots[0].currentCommand != INDEX_NONE ? 0 : Snapshot.robots[1].currentCommand != INDEX_NONE ? 1
                                                                                                                                    : INDEX_NONE;
    if (PlayerOneTank)
        PlayerOneTank->SetActiveIndicator(ActiveRobot == 0);
    if (PlayerTwoTank)
        PlayerTwoTank->SetActiveIndicator(ActiveRobot == 1);
    if (ActionPointItemSpawner)
        ActionPointItemSpawner->SetReplayStep(Snapshot.step);
}

void AAWArenaRenderer::ProcessEvents(const TArray<Automata::SimEvent> &Events, int32 FromStep, int32 ToStep)
{
    TMap<int32, FVector> ShotStarts;
    TSet<int32> ShieldedTargets;
    for (const Automata::SimEvent &Evt : Events)
    {
        if (Evt.step < FromStep || Evt.step > ToStep)
            continue;

        const auto &Robot = CurrentSnapshot.robots[Evt.robot];
        FVector Pos = GridToWorld(Robot.x, Robot.y);

        switch (Evt.type)
        {
        case Automata::EventType::Fire:
        {
            ResolveTankActors();
            AAWTankActor *Tank = Evt.robot == 0 ? PlayerOneTank.Get() : PlayerTwoTank.Get();
            const FTransform MuzzleTransform = Tank ? Tank->GetMuzzleTransform() : FTransform(DirToRotation(Robot.facing), Pos + FVector(0, 0, AWVisualConfig::ProjectileZ));
            ShotStarts.Add(Evt.robot, MuzzleTransform.GetLocation());
            TriggerMuzzleFlash(Evt.robot);
            PlaySFX(AWVisualAssets::SFX_Fire, MuzzleTransform.GetLocation());
            break;
        }
        case Automata::EventType::ShotBlocked:
        {
            const FVector Start = ShotStarts.FindRef(Evt.robot);
            SpawnProjectile(Start.IsNearlyZero() ? Pos : Start, GridToWorld(Evt.paramA, Evt.paramB),
                            INDEX_NONE, false, false);
            break;
        }
        case Automata::EventType::ShieldCharged:
            SetShieldActive(Evt.robot, true);
            break;
        case Automata::EventType::ShieldAbsorbed:
            SetShieldActive(Evt.robot, true);
            ShieldedTargets.Add(Evt.robot);
            break;
        case Automata::EventType::Hit:
        {
            const int32 Shooter = Evt.paramB;
            const FVector Start = ShotStarts.FindRef(Shooter);
            SpawnProjectile(Start.IsNearlyZero() ? GridToWorld(CurrentSnapshot.robots[Shooter].x, CurrentSnapshot.robots[Shooter].y) : Start,
                            Pos, Evt.robot, ShieldedTargets.Contains(Evt.robot),
                            CurrentSnapshot.robots[Evt.robot].hp <= 0);
            break;
        }
        default:
            break;
        }
    }
}

void AAWArenaRenderer::ResetVisuals()
{
    bHasSnapshot = false;
    bHasPendingFinalShieldState = false;
    SetShieldActive(0, false);
    SetShieldActive(1, false);
    ResolveTankActors();
    if (PlayerOneTank)
        PlayerOneTank->ResetVisual();
    if (PlayerTwoTank)
        PlayerTwoTank->ResetVisual();
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
    if (ActionPointItemSpawner)
        ActionPointItemSpawner->SetReplayStep(INDEX_NONE);
    for (const TPair<int32, TObjectPtr<ATableObstable>> &Entry : Obstacles)
        if (Entry.Value)
            Entry.Value->ResetHealth();
}

void AAWArenaRenderer::BuildFloorGrid(int32 Width, int32 Height)
{
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FColor> Colors;

    const float CellSize = AWVisualConfig::CellSize;
    const float Gap = AWVisualConfig::GridGap;

    for (int32 Y = 0; Y < Height; ++Y)
    {
        for (int32 X = 0; X < Width; ++X)
        {
            int32 BaseIdx = Vertices.Num();
            float X0 = X * CellSize;
            float Y0 = Y * CellSize;

            Vertices.Add(FVector(X0 + Gap, Y0 + Gap, AWVisualConfig::FloorZ));
            Vertices.Add(FVector(X0 + CellSize - Gap, Y0 + Gap, AWVisualConfig::FloorZ));
            Vertices.Add(FVector(X0 + CellSize - Gap, Y0 + CellSize - Gap, AWVisualConfig::FloorZ));
            Vertices.Add(FVector(X0 + Gap, Y0 + CellSize - Gap, AWVisualConfig::FloorZ));

            Triangles.Add(BaseIdx);
            Triangles.Add(BaseIdx + 1);
            Triangles.Add(BaseIdx + 2);
            Triangles.Add(BaseIdx);
            Triangles.Add(BaseIdx + 2);
            Triangles.Add(BaseIdx + 3);

            for (int32 i = 0; i < 4; ++i)
                Normals.Add(FVector::UpVector);
            UVs.Add(FVector2D(0, 0));
            UVs.Add(FVector2D(1, 0));
            UVs.Add(FVector2D(1, 1));
            UVs.Add(FVector2D(0, 1));

            // Checkerboard pattern: dark neutral with luminous grid lines
            bool bDark = ((X + Y) % 2 == 0);
            FColor CellColor = bDark ? FColor(8, 10, 14) : FColor(12, 15, 20);
            for (int32 i = 0; i < 4; ++i)
                Colors.Add(CellColor);
        }
    }

    FloorMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, TArray<FProcMeshTangent>(), false);
    if (UMaterialInterface *ArenaMaterial = LoadObject<UMaterialInterface>(nullptr, AWVisualAssets::M_ArenaCell))
    {
        FloorMesh->SetMaterial(0, ArenaMaterial);
    }
}

void AAWArenaRenderer::SpawnCoverVisuals(int32 Width, int32 Height, const TArray<Automata::CellType> &Grid)
{
    for (const TPair<int32, TObjectPtr<ATableObstable>> &Entry : Obstacles)
        if (Entry.Value)
            Entry.Value->Destroy();
    Obstacles.Reset();

    if (!ObstacleClass)
        return;

    // CoverIdx drives deterministic color and shape variation.
    int32 CoverIdx = 0;
    for (int32 Y = 0; Y < Height; ++Y)
    {
        for (int32 X = 0; X < Width; ++X)
        {
            int32 CellIdx = Y * Width + X;
            if (CellIdx >= Grid.Num())
                break;

            if (Grid[CellIdx] == Automata::CellType::Cover)
            {
                FVector Pos = GridToWorld(X, Y);
                Pos.Z = AWVisualConfig::FloorZ;

                // Cycle through four tones so adjacent obstacles remain distinct.
                FLinearColor CoverColor;
                switch (CoverIdx % 4)
                {
                case 0:
                    CoverColor = FLinearColor(0.15f, 0.12f, 0.08f);
                    break;
                case 1:
                    CoverColor = FLinearColor(0.08f, 0.12f, 0.15f);
                    break;
                case 2:
                    CoverColor = FLinearColor(0.12f, 0.08f, 0.12f);
                    break;
                default:
                    CoverColor = FLinearColor(0.10f, 0.14f, 0.10f);
                    break;
                }

                FActorSpawnParameters SpawnParameters;
                SpawnParameters.Owner = this;
                SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                if (ATableObstable *Obstacle = GetWorld()->SpawnActor<ATableObstable>(ObstacleClass, Pos, FRotator::ZeroRotator, SpawnParameters))
                {
                    Obstacle->InitializeObstacle(CellIdx, CoverColor);
                    Obstacles.Add(CellIdx, Obstacle);
                }

                ++CoverIdx;
            }
        }
    }
}

void AAWArenaRenderer::TriggerMuzzleFlash(int32 RobotIdx)
{
    ResolveTankActors();
    AAWTankActor *Tank = RobotIdx == 0 ? PlayerOneTank.Get() : PlayerTwoTank.Get();
    if (!Tank)
        return;

    UNiagaraSystem *NS = LoadObject<UNiagaraSystem>(nullptr, AWVisualAssets::NS_MuzzleFlash);
    if (NS)
    {
        if (UNiagaraComponent *MuzzleFlash = UNiagaraFunctionLibrary::SpawnSystemAttached(
                NS, Tank->GetMuzzleComponent(), Tank->GetMuzzleSocketName(), FVector::ZeroVector,
                FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true))
            ScheduleComponentDestruction(MuzzleFlash, AWVisualConfig::MuzzleFlashLifespan);
    }
    else
    {
        SpawnTransientLight(Tank->GetMuzzleTransform().GetLocation(), 4000.f, 200.f, FColor::Yellow, AWVisualConfig::TransientVFXLifespan);
    }
}

void AAWArenaRenderer::TriggerImpact(FVector WorldPos)
{
    UNiagaraSystem *NS = LoadObject<UNiagaraSystem>(nullptr, AWVisualAssets::NS_Impact);
    if (NS)
    {
        if (UNiagaraComponent *Impact = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                GetWorld(), NS, WorldPos, FRotator::ZeroRotator, FVector(0.8f)))
            ScheduleComponentDestruction(Impact, AWVisualConfig::ImpactVFXLifespan);
    }
    else
    {
        SpawnTransientLight(WorldPos, 5000.f, 150.f, FColor::Orange, AWVisualConfig::TransientVFXLifespan);
    }
}

void AAWArenaRenderer::TriggerDestruction(FVector WorldPos)
{
    UNiagaraSystem *NS = LoadObject<UNiagaraSystem>(nullptr, AWVisualAssets::NS_Destruction);
    if (NS)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), NS, WorldPos, FRotator::ZeroRotator,
                                                       FVector(1.45f), true, true);
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), NS, WorldPos + FVector(0.f, 0.f, 35.f),
                                                       FRotator(0.f, 90.f, 0.f), FVector(0.85f), true, true);
        SpawnTransientLight(WorldPos, 18000.f, 500.f, FColor(255, 72, 18), 0.45f);
        SpawnTransientLight(WorldPos + FVector(0.f, 0.f, 80.f), 7000.f, 300.f, FColor(255, 170, 60), 1.1f);
    }
    else
    {
        SpawnTransientLight(WorldPos, 15000.f, 400.f, FColor::Red, AWVisualConfig::TransientVFXLifespan * 2.f);
    }
}

void AAWArenaRenderer::SetShieldActive(int32 RobotIdx, bool bActive)
{
    if (RobotIdx < 0 || RobotIdx > 1)
        return;

    if (!bActive)
    {
        if (UStaticMeshComponent *ShieldEffect = ShieldEffects[RobotIdx].Get())
            ShieldEffect->DestroyComponent();
        ShieldEffects[RobotIdx].Reset();
        return;
    }
    if (ShieldEffects[RobotIdx].IsValid())
        return;

    ResolveTankActors();
    AAWTankActor *Tank = RobotIdx == 0 ? PlayerOneTank.Get() : PlayerTwoTank.Get();
    UStaticMesh *Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    UMaterialInterface *ShieldMaterial = LoadObject<UMaterialInterface>(nullptr, AWVisualAssets::M_ShieldEnergy);
    if (!Tank || !Sphere || !ShieldMaterial)
        return;

    UStaticMeshComponent *ShieldEffect = NewObject<UStaticMeshComponent>(Tank);
    ShieldEffect->SetupAttachment(Tank->GetRootComponent());
    ShieldEffect->SetStaticMesh(Sphere);
    ShieldEffect->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ShieldEffect->SetCastShadow(false);
    ShieldEffect->SetReceivesDecals(false);
    ShieldEffect->SetRelativeScale3D(FVector(1.35f));
    ShieldEffect->SetTranslucentSortPriority(10);
    Tank->AddInstanceComponent(ShieldEffect);
    ShieldEffect->RegisterComponent();

    UMaterialInstanceDynamic *DynamicShieldMaterial = UMaterialInstanceDynamic::Create(ShieldMaterial, ShieldEffect);
    DynamicShieldMaterial->SetVectorParameterValue(TEXT("ShieldColor"), FLinearColor(0.02f, 4.f, 8.f, 1.f));
    DynamicShieldMaterial->SetScalarParameterValue(TEXT("Opacity"), 0.22f);
    ShieldEffect->SetMaterial(0, DynamicShieldMaterial);
    ShieldEffects[RobotIdx] = ShieldEffect;
}

void AAWArenaRenderer::SetFinalEffects(const std::array<FAWRobotEffects, 2> &FinalEffects)
{
    for (int32 RobotIndex = 0; RobotIndex < 2; ++RobotIndex)
        PendingFinalShieldState[RobotIndex] = HasActiveShield(FinalEffects[RobotIndex]);
    bHasPendingFinalShieldState = true;
    if (Projectiles.IsEmpty())
        ApplyPendingFinalShieldState();
}

void AAWArenaRenderer::ApplyPendingFinalShieldState()
{
    for (int32 RobotIndex = 0; RobotIndex < 2; ++RobotIndex)
        SetShieldActive(RobotIndex, PendingFinalShieldState[RobotIndex]);
    bHasPendingFinalShieldState = false;
}

void AAWArenaRenderer::SpawnProjectile(FVector Start, FVector End, int32 TargetRobot,
                                       bool bShielded, bool bDestroyedTarget)
{
    UStaticMesh *Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    UStaticMesh *Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!Sphere || !Cube)
        return;

    UStaticMeshComponent *Bolt = NewObject<UStaticMeshComponent>(this);
    Bolt->SetupAttachment(GetRootComponent());
    Bolt->SetStaticMesh(Sphere);
    Bolt->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Bolt->SetWorldScale3D(FVector(0.11f));
    Bolt->SetWorldLocation(Start);
    AddInstanceComponent(Bolt);
    Bolt->RegisterComponent();

    UStaticMeshComponent *Beam = NewObject<UStaticMeshComponent>(this);
    Beam->SetupAttachment(GetRootComponent());
    Beam->SetStaticMesh(Cube);
    Beam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    AddInstanceComponent(Beam);
    Beam->RegisterComponent();

    if (UMaterialInterface *EffectMaterial = LoadObject<UMaterialInterface>(nullptr, AWVisualAssets::M_Effect))
    {
        UMaterialInstanceDynamic *ProjectileMaterial = UMaterialInstanceDynamic::Create(EffectMaterial, this);
        ProjectileMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(8.f, 1.8f, 0.15f, 1.f));
        ProjectileMaterial->SetScalarParameterValue(TEXT("Opacity"), 0.92f);
        Bolt->SetMaterial(0, ProjectileMaterial);
        Beam->SetMaterial(0, ProjectileMaterial);
    }

    UNiagaraComponent *Trail = nullptr;
    if (UNiagaraSystem *TrailSystem = LoadObject<UNiagaraSystem>(nullptr, AWVisualAssets::NS_ProjectileTrail))
        Trail = UNiagaraFunctionLibrary::SpawnSystemAttached(TrailSystem, Bolt, NAME_None,
                                                             FVector::ZeroVector, FRotator::ZeroRotator,
                                                             EAttachLocation::SnapToTarget, true);

    FProjectileVisual Projectile;
    Projectile.Bolt = Bolt;
    Projectile.Beam = Beam;
    Projectile.Trail = Trail;
    Projectile.Start = Start;
    Projectile.End = End;
    Projectile.Duration = FMath::Clamp(FVector::Distance(Start, End) / AWVisualConfig::ProjectileSpeed,
                                       AWVisualConfig::ProjectileMinDuration, AWVisualConfig::ProjectileMaxDuration);
    Projectile.TargetRobot = TargetRobot;
    Projectile.bShielded = bShielded;
    Projectile.bShieldRemainsActive = TargetRobot >= 0 && TargetRobot < 2 &&
                                      HasActiveShield(CurrentSnapshot.robots[TargetRobot].effects);
    Projectile.bDestroyedTarget = bDestroyedTarget;
    Projectiles.Add(Projectile);
    UpdateProjectileBeam(Beam, Start, Start);
}

void AAWArenaRenderer::UpdateProjectileBeam(UStaticMeshComponent *Beam, const FVector &Start, const FVector &End)
{
    if (!Beam)
        return;

    const FVector Delta = End - Start;
    const float Length = Delta.Size();
    Beam->SetWorldLocation((Start + End) * 0.5f);
    Beam->SetWorldRotation(Delta.IsNearlyZero() ? FRotator::ZeroRotator : Delta.Rotation());
    Beam->SetWorldScale3D(FVector(FMath::Max(0.001f, Length / 100.f),
                                  AWVisualConfig::ProjectileBeamThickness,
                                  AWVisualConfig::ProjectileBeamThickness));
}

void AAWArenaRenderer::SpawnTransientLight(FVector WorldPos, float Intensity, float Radius, FColor Color, float Lifespan)
{
    UPointLightComponent *Light = NewObject<UPointLightComponent>(this);
    Light->SetupAttachment(GetRootComponent());
    Light->SetWorldLocation(WorldPos);
    Light->SetIntensity(Intensity);
    Light->SetAttenuationRadius(Radius);
    Light->SetLightColor(Color);
    Light->RegisterComponent();
    ScheduleComponentDestruction(Light, Lifespan);
}

void AAWArenaRenderer::ScheduleComponentDestruction(UActorComponent *Component, float Lifespan)
{
    FTimerHandle Handle;
    TWeakObjectPtr<UActorComponent> WeakComponent = Component;
    GetWorld()->GetTimerManager().SetTimer(Handle, [WeakComponent]()
                                           { if (WeakComponent.IsValid()) WeakComponent->DestroyComponent(); }, Lifespan, false);
}

void AAWArenaRenderer::PlaySFX(const TCHAR *SoftPath, FVector Location)
{
    USoundBase *Sound = LoadObject<USoundBase>(nullptr, SoftPath);
    if (Sound)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), Sound, Location);
    }
}

FVector AAWArenaRenderer::GridToWorld(int32 X, int32 Y) const
{
    return FVector(
        X * AWVisualConfig::CellSize + AWVisualConfig::CellSize * 0.5f,
        Y * AWVisualConfig::CellSize + AWVisualConfig::CellSize * 0.5f,
        AWVisualConfig::RobotZ);
}

FRotator AAWArenaRenderer::DirToRotation(Automata::Dir D) const
{
    switch (D)
    {
    case Automata::Dir::North:
        return FRotator(0, -90, 0);
    case Automata::Dir::East:
        return FRotator(0, 0, 0);
    case Automata::Dir::South:
        return FRotator(0, 90, 0);
    case Automata::Dir::West:
        return FRotator(0, 180, 0);
    default:
        return FRotator::ZeroRotator;
    }
}

void AAWArenaRenderer::ResolveTankActors()
{
    if (PlayerOneTank && PlayerTwoTank)
        return;

    for (TActorIterator<AAWTankActor> It(GetWorld()); It; ++It)
    {
        AAWTankActor *Tank = *It;
        if (Tank->GetRobotIndex() == 0 && !PlayerOneTank)
            PlayerOneTank = Tank;
        else if (Tank->GetRobotIndex() == 1 && !PlayerTwoTank)
            PlayerTwoTank = Tank;
    }
}

void AAWArenaRenderer::ResolveActionPointItemSpawner()
{
    if (ActionPointItemSpawner)
        return;

    for (TActorIterator<AAWAPItemSpawner> It(GetWorld()); It; ++It)
    {
        ActionPointItemSpawner = *It;
        return;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = this;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ActionPointItemSpawner = GetWorld()->SpawnActor<AAWAPItemSpawner>(
        AAWAPItemSpawner::StaticClass(), GetActorTransform(), SpawnParameters);
}

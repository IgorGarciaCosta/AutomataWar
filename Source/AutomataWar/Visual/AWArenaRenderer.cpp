/**
 * @file AWArenaRenderer.cpp
 * @brief Implementation of the presentation-only arena renderer.
 */

#include "AWArenaRenderer.h"
#include "AWTankActor.h"
#include "AWVisualTypes.h"
#include "TableObstable.h"
#include "AutomataWar/UI/AWUITypes.h"
#include "ProceduralMeshComponent.h"
#include "Components/AudioComponent.h"
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
#include "Sound/SoundBase.h"

AAWArenaRenderer::AAWArenaRenderer()
{
    PrimaryActorTick.bCanEverTick = false;

    USceneComponent *Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    Root->SetMobility(EComponentMobility::Static);

    FloorMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("FloorMesh"));
    FloorMesh->SetupAttachment(Root);
    FloorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FloorMesh->bUseComplexAsSimpleCollision = false;
}

void AAWArenaRenderer::BeginPlay()
{
    Super::BeginPlay();
    ResolveTankActors();
}

void AAWArenaRenderer::InitializeArena(const Automata::SimConfig &Config, const TArray<Automata::CellType> &Grid)
{
    BuildFloorGrid(Config.gridWidth, Config.gridHeight);
    SpawnCoverVisuals(Config.gridWidth, Config.gridHeight, Grid);
}

void AAWArenaRenderer::SetSnapshot(const Automata::TickSnapshot &Snapshot)
{
    if (bHasSnapshot)
    {
        for (int32 RobotIdx = 0; RobotIdx < 2; ++RobotIdx)
        {
            const bool bMoved = CurrentSnapshot.robots[RobotIdx].x != Snapshot.robots[RobotIdx].x ||
                                CurrentSnapshot.robots[RobotIdx].y != Snapshot.robots[RobotIdx].y;
            if (!bMoved)
                StopMovementSound(RobotIdx);
        }
    }

    CurrentSnapshot = Snapshot;
    bHasSnapshot = true;
    for (const TPair<int32, TObjectPtr<ATableObstable>> &Entry : Obstacles)
        if (Entry.Value && Entry.Key >= 0 && static_cast<size_t>(Entry.Key) < Snapshot.obstacleHealth.size())
            Entry.Value->SetHealth(Snapshot.obstacleHealth[static_cast<size_t>(Entry.Key)]);

    ResolveTankActors();
    if (PlayerOneTank)
        PlayerOneTank->SetTargetTransform(GridToWorld(Snapshot.robots[0].x, Snapshot.robots[0].y), DirToRotation(Snapshot.robots[0].facing));
    if (PlayerTwoTank)
        PlayerTwoTank->SetTargetTransform(GridToWorld(Snapshot.robots[1].x, Snapshot.robots[1].y), DirToRotation(Snapshot.robots[1].facing));
}

void AAWArenaRenderer::ProcessEvents(const TArray<Automata::SimEvent> &Events, int32 FromTick, int32 ToTick)
{
    for (const Automata::SimEvent &Evt : Events)
    {
        if (Evt.tick < FromTick || Evt.tick > ToTick)
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
            TriggerMuzzleFlash(Evt.robot);
            SpawnProjectileBolt(Evt.robot, MuzzleTransform.GetLocation(), DirToRotation(Robot.facing).Vector());
            PlaySFX(AWVisualAssets::SFX_Fire, MuzzleTransform.GetLocation());
            break;
        }
        case Automata::EventType::ProjectileBlocked:
            TriggerImpact(GridToWorld(Evt.paramA, Evt.paramB));
            break;
        case Automata::EventType::Hit:
            TriggerImpact(Pos);
            PlaySFX(AWVisualAssets::SFX_Impact, Pos);
            break;
        case Automata::EventType::ShieldActivate:
            TriggerShieldBubble(Evt.robot);
            PlaySFX(AWVisualAssets::SFX_Shield, Pos);
            break;
        case Automata::EventType::Move:
            StartMovementSound(Evt.robot);
            break;
        default:
            break;
        }

        if (Evt.type == Automata::EventType::Hit)
        {
            const int32 TargetRobot = Evt.robot;
            if (CurrentSnapshot.robots[TargetRobot].hp <= 0)
            {
                FVector DeathPos = GridToWorld(CurrentSnapshot.robots[TargetRobot].x, CurrentSnapshot.robots[TargetRobot].y);
                TriggerDestruction(DeathPos);
                PlaySFX(AWVisualAssets::SFX_Destroy, DeathPos);
            }
        }
    }
}

void AAWArenaRenderer::ResetVisuals()
{
    StopMovementSound(0);
    StopMovementSound(1);
    bHasSnapshot = false;
    ResolveTankActors();
    if (PlayerOneTank)
        PlayerOneTank->ResetVisual();
    if (PlayerTwoTank)
        PlayerTwoTank->ResetVisual();
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

    // Cube is the fallback shape; cylinder is used for variant 0 when available.
    UStaticMesh *CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    UStaticMesh *CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (!CubeMesh)
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

                UStaticMesh *Mesh = CubeMesh;
                FVector Scale;
                int32 Variant = CoverIdx % 3;
                switch (Variant)
                {
                case 0:
                    Mesh = CylinderMesh ? CylinderMesh : CubeMesh;
                    Pos.Z += 45.f;
                    Scale = FVector(0.35f, 0.35f, 0.9f);
                    break;
                case 1:
                    Pos.Z += 30.f;
                    Scale = FVector(0.4f, 0.9f, 0.6f);
                    break;
                default:
                    Pos.Z += 20.f;
                    Scale = FVector(0.85f, 0.85f, 0.4f);
                    break;
                }

                FActorSpawnParameters SpawnParameters;
                SpawnParameters.Owner = this;
                SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                if (ATableObstable *Obstacle = GetWorld()->SpawnActor<ATableObstable>(ATableObstable::StaticClass(), Pos, FRotator::ZeroRotator, SpawnParameters))
                {
                    Obstacle->InitializeObstacle(CellIdx, Mesh, Scale, CoverColor);
                    Obstacles.Add(CellIdx, Obstacle);
                }

                ++CoverIdx;
            }
        }
    }
}

void AAWArenaRenderer::SpawnProjectileBolt(int32 OwnerIdx, FVector WorldPos, FVector Direction)
{
    UStaticMesh *CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!CubeMesh)
        return;

    UStaticMeshComponent *Bolt = NewObject<UStaticMeshComponent>(this);
    Bolt->SetupAttachment(GetRootComponent());
    Bolt->SetStaticMesh(CubeMesh);
    Bolt->SetWorldLocation(WorldPos);
    Bolt->SetWorldScale3D(FVector(0.3f, 0.08f, 0.08f));
    Bolt->SetWorldRotation(Direction.Rotation());
    Bolt->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Bolt->SetCustomPrimitiveDataFloat(0, OwnerIdx == 0 ? 0.f : 1.f);
    Bolt->RegisterComponent();
    if (UMaterialInterface *EffectMaterial = LoadObject<UMaterialInterface>(nullptr, AWVisualAssets::M_Effect))
        Bolt->SetMaterial(0, EffectMaterial);

    UMaterialInstanceDynamic *Mat = Bolt->CreateDynamicMaterialInstance(0);
    if (Mat)
    {
        FLinearColor Color = (OwnerIdx == 0) ? AWUIColors::AccentCyan : AWUIColors::AccentCoral;
        Mat->SetVectorParameterValue(TEXT("BaseColor"), Color);
        Mat->SetVectorParameterValue(TEXT("EmissiveColor"), Color * 5.f);
    }

    UPointLightComponent *Light = NewObject<UPointLightComponent>(Bolt);
    Light->SetupAttachment(Bolt);
    Light->SetIntensity(500.f);
    Light->SetAttenuationRadius(150.f);
    Light->SetLightColor((OwnerIdx == 0) ? FColor::Cyan : FColor(255, 90, 80));
    Light->RegisterComponent();

    ScheduleComponentDestruction(Bolt, AWVisualConfig::ProjectileBoltLifespan);
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
        UNiagaraFunctionLibrary::SpawnSystemAttached(NS, Tank->GetMuzzleComponent(), Tank->GetMuzzleSocketName(),
                                                     FVector::ZeroVector, FRotator::ZeroRotator,
                                                     EAttachLocation::SnapToTarget, true);
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
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), NS, WorldPos);
    }
    else
    {
        SpawnTransientLight(WorldPos, 5000.f, 150.f, FColor::Orange, AWVisualConfig::TransientVFXLifespan);
    }
}

void AAWArenaRenderer::TriggerShieldBubble(int32 RobotIdx)
{
    ResolveTankActors();
    AAWTankActor *Tank = RobotIdx == 0 ? PlayerOneTank.Get() : PlayerTwoTank.Get();
    if (!Tank)
        return;

    FVector Pos = Tank->GetActorLocation();

    UNiagaraSystem *NS = LoadObject<UNiagaraSystem>(nullptr, AWVisualAssets::NS_Shield);
    if (NS)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), NS, Pos);
    }
    else
    {
        UStaticMesh *SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
        if (SphereMesh)
        {
            UStaticMeshComponent *Shield = NewObject<UStaticMeshComponent>(this);
            Shield->SetupAttachment(GetRootComponent());
            Shield->SetStaticMesh(SphereMesh);
            Shield->SetWorldLocation(Pos);
            Shield->SetWorldScale3D(FVector(1.5f));
            Shield->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Shield->RegisterComponent();
            if (UMaterialInterface *EffectMaterial = LoadObject<UMaterialInterface>(nullptr, AWVisualAssets::M_Effect))
                Shield->SetMaterial(0, EffectMaterial);

            UMaterialInstanceDynamic *Mat = Shield->CreateDynamicMaterialInstance(0);
            if (Mat)
            {
                FLinearColor Color = (RobotIdx == 0) ? AWUIColors::AccentCyan : AWUIColors::AccentCoral;
                Mat->SetVectorParameterValue(TEXT("EmissiveColor"), Color * 3.f);
                Mat->SetScalarParameterValue(TEXT("Opacity"), 0.3f);
            }

            ScheduleComponentDestruction(Shield, AWVisualConfig::ShieldBubbleLifespan);
        }
    }
}

void AAWArenaRenderer::TriggerDestruction(FVector WorldPos)
{
    UNiagaraSystem *NS = LoadObject<UNiagaraSystem>(nullptr, AWVisualAssets::NS_Destruction);
    if (NS)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), NS, WorldPos);
    }
    else
    {
        SpawnTransientLight(WorldPos, 15000.f, 400.f, FColor::Red, AWVisualConfig::TransientVFXLifespan * 2.f);
    }
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

void AAWArenaRenderer::StartMovementSound(int32 RobotIdx)
{
    if (MovementAudio[RobotIdx].IsValid())
    {
        if (!MovementAudio[RobotIdx]->IsPlaying())
            MovementAudio[RobotIdx]->Play();
        return;
    }

    ResolveTankActors();
    AAWTankActor *Tank = RobotIdx == 0 ? PlayerOneTank.Get() : PlayerTwoTank.Get();
    USoundBase *Sound = LoadObject<USoundBase>(nullptr, AWVisualAssets::SFX_Move);
    if (Tank && Sound)
    {
        MovementAudio[RobotIdx] = UGameplayStatics::SpawnSoundAttached(
            Sound, Tank->GetRootComponent(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator,
            EAttachLocation::KeepRelativeOffset, true, 1.f, 1.f, 0.f, nullptr, nullptr, false);
    }
}

void AAWArenaRenderer::StopMovementSound(int32 RobotIdx)
{
    if (MovementAudio[RobotIdx].IsValid())
        MovementAudio[RobotIdx]->Stop();
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

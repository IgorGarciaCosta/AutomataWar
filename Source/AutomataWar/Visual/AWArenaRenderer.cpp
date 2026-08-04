/**
 * @file AWArenaRenderer.cpp
 * @brief Implementation of the presentation-only arena renderer.
 */

#include "AWArenaRenderer.h"
#include "AWTankActor.h"
#include "AWVisualTypes.h"
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
    GridWidth = Config.gridWidth;
    GridHeight = Config.gridHeight;
    BuildFloorGrid(GridWidth, GridHeight);
    SpawnCoverVisuals(GridWidth, GridHeight, Grid);
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
    UStaticMesh *CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    UStaticMesh *CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    UMaterialInterface *CoverMaterial = LoadObject<UMaterialInterface>(nullptr, AWVisualAssets::M_Cover);
    if (!CubeMesh)
        return;
    auto TintCover = [CoverMaterial](UStaticMeshComponent *Component, const FLinearColor &Color)
    {
        if (CoverMaterial)
            Component->SetMaterial(0, CoverMaterial);
        if (UMaterialInstanceDynamic *Material = Component->CreateDynamicMaterialInstance(0))
            Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
    };

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

                int32 Variant = CoverIdx % 3;
                switch (Variant)
                {
                case 0:
                {
                    UStaticMesh *Mesh = CylinderMesh ? CylinderMesh : CubeMesh;
                    UStaticMeshComponent *Block = NewObject<UStaticMeshComponent>(this);
                    Block->SetupAttachment(GetRootComponent());
                    Block->SetStaticMesh(Mesh);
                    Block->SetWorldLocation(Pos + FVector(0, 0, 45));
                    Block->SetWorldScale3D(FVector(0.35f, 0.35f, 0.9f));
                    Block->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                    Block->RegisterComponent();
                    TintCover(Block, CoverColor);
                    break;
                }
                case 1:
                {
                    UStaticMeshComponent *A = NewObject<UStaticMeshComponent>(this);
                    A->SetupAttachment(GetRootComponent());
                    A->SetStaticMesh(CubeMesh);
                    A->SetWorldLocation(Pos + FVector(-15, 0, 30));
                    A->SetWorldScale3D(FVector(0.4f, 0.9f, 0.6f));
                    A->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                    A->RegisterComponent();
                    TintCover(A, CoverColor);

                    UStaticMeshComponent *B = NewObject<UStaticMeshComponent>(this);
                    B->SetupAttachment(GetRootComponent());
                    B->SetStaticMesh(CubeMesh);
                    B->SetWorldLocation(Pos + FVector(20, -20, 20));
                    B->SetWorldScale3D(FVector(0.35f, 0.35f, 0.4f));
                    B->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                    B->RegisterComponent();
                    TintCover(B, CoverColor * 0.8f);
                    break;
                }
                default:
                {
                    UStaticMeshComponent *Block = NewObject<UStaticMeshComponent>(this);
                    Block->SetupAttachment(GetRootComponent());
                    Block->SetStaticMesh(CubeMesh);
                    Block->SetWorldLocation(Pos + FVector(0, 0, 22));
                    Block->SetWorldScale3D(FVector(0.85f, 0.85f, 0.4f));
                    Block->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                    Block->RegisterComponent();
                    TintCover(Block, CoverColor);
                    break;
                }
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

    // Destroy after lifespan
    FTimerHandle Handle;
    TWeakObjectPtr<UStaticMeshComponent> WeakBolt = Bolt;
    GetWorld()->GetTimerManager().SetTimer(Handle, [WeakBolt]()
                                           {
		if (WeakBolt.IsValid()) WeakBolt->DestroyComponent(); }, AWVisualConfig::ProjectileBoltLifespan, false);
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
        UPointLightComponent *Flash = NewObject<UPointLightComponent>(this);
        Flash->SetupAttachment(GetRootComponent());
        Flash->SetWorldLocation(Tank->GetMuzzleTransform().GetLocation());
        Flash->SetIntensity(4000.f);
        Flash->SetAttenuationRadius(200.f);
        Flash->SetLightColor(FColor::Yellow);
        Flash->RegisterComponent();

        FTimerHandle H;
        TWeakObjectPtr<UPointLightComponent> Weak = Flash;
        GetWorld()->GetTimerManager().SetTimer(H, [Weak]()
                                               { if (Weak.IsValid()) Weak->DestroyComponent(); }, AWVisualConfig::TransientVFXLifespan, false);
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
        UPointLightComponent *Flash = NewObject<UPointLightComponent>(this);
        Flash->SetupAttachment(GetRootComponent());
        Flash->SetWorldLocation(WorldPos);
        Flash->SetIntensity(5000.f);
        Flash->SetAttenuationRadius(150.f);
        Flash->SetLightColor(FColor::Orange);
        Flash->RegisterComponent();

        FTimerHandle H;
        TWeakObjectPtr<UPointLightComponent> Weak = Flash;
        GetWorld()->GetTimerManager().SetTimer(H, [Weak]()
                                               { if (Weak.IsValid()) Weak->DestroyComponent(); }, AWVisualConfig::TransientVFXLifespan, false);
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

            FTimerHandle H;
            TWeakObjectPtr<UStaticMeshComponent> Weak = Shield;
            GetWorld()->GetTimerManager().SetTimer(H, [Weak]()
                                                   { if (Weak.IsValid()) Weak->DestroyComponent(); }, AWVisualConfig::ShieldBubbleLifespan, false);
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
        UPointLightComponent *Flash = NewObject<UPointLightComponent>(this);
        Flash->SetupAttachment(GetRootComponent());
        Flash->SetWorldLocation(WorldPos);
        Flash->SetIntensity(15000.f);
        Flash->SetAttenuationRadius(400.f);
        Flash->SetLightColor(FColor::Red);
        Flash->RegisterComponent();

        FTimerHandle H;
        TWeakObjectPtr<UPointLightComponent> Weak = Flash;
        GetWorld()->GetTimerManager().SetTimer(H, [Weak]()
                                               { if (Weak.IsValid()) Weak->DestroyComponent(); }, AWVisualConfig::TransientVFXLifespan * 2.f, false);
    }
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

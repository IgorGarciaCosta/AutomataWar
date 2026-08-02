/**
 * @file AWArenaRenderer.cpp
 * @brief Implementation of the presentation-only arena renderer.
 */

#include "AWArenaRenderer.h"
#include "AWVisualTypes.h"
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
#include "UObject/ConstructorHelpers.h"
#include "Sound/SoundBase.h"

AAWArenaRenderer::AAWArenaRenderer()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    USceneComponent *Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    Root->SetMobility(EComponentMobility::Static);

    FloorMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("FloorMesh"));
    FloorMesh->SetupAttachment(Root);
    FloorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FloorMesh->bUseComplexAsSimpleCollision = false;

    RobotRoot0 = CreateDefaultSubobject<USceneComponent>(TEXT("RobotRoot0"));
    RobotRoot0->SetupAttachment(Root);

    RobotRoot1 = CreateDefaultSubobject<USceneComponent>(TEXT("RobotRoot1"));
    RobotRoot1->SetupAttachment(Root);

    PlayerOneAccentLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PlayerOneAccentLight"));
    PlayerOneAccentLight->SetupAttachment(Root);
    PlayerOneAccentLight->SetRelativeLocation(FVector(250.f, 250.f, 260.f));
    PlayerOneAccentLight->SetLightColor(FColor(0, 210, 255));
    PlayerOneAccentLight->SetIntensity(2600.f);
    PlayerOneAccentLight->SetAttenuationRadius(850.f);

    PlayerTwoAccentLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PlayerTwoAccentLight"));
    PlayerTwoAccentLight->SetupAttachment(Root);
    PlayerTwoAccentLight->SetRelativeLocation(FVector(1350.f, 1350.f, 260.f));
    PlayerTwoAccentLight->SetLightColor(FColor(255, 78, 64));
    PlayerTwoAccentLight->SetIntensity(2600.f);
    PlayerTwoAccentLight->SetAttenuationRadius(850.f);
}

void AAWArenaRenderer::BeginPlay()
{
    Super::BeginPlay();
    BuildRobotVisuals();
}

void AAWArenaRenderer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Smooth interpolation toward target positions
    const float Alpha = FMath::Clamp(DeltaTime * AWVisualConfig::InterpSpeed, 0.f, 1.f);

    if (RobotRoot0)
    {
        FVector Cur = RobotRoot0->GetComponentLocation();
        RobotRoot0->SetWorldLocation(FMath::Lerp(Cur, TargetPos0, Alpha));
        RobotRoot0->SetWorldRotation(FMath::Lerp(RobotRoot0->GetComponentRotation(), TargetRot0, Alpha));
    }
    if (RobotRoot1)
    {
        FVector Cur = RobotRoot1->GetComponentLocation();
        RobotRoot1->SetWorldLocation(FMath::Lerp(Cur, TargetPos1, Alpha));
        RobotRoot1->SetWorldRotation(FMath::Lerp(RobotRoot1->GetComponentRotation(), TargetRot1, Alpha));
    }
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
    CurrentSnapshot = Snapshot;
    TargetPos0 = GridToWorld(Snapshot.robots[0].x, Snapshot.robots[0].y);
    TargetPos1 = GridToWorld(Snapshot.robots[1].x, Snapshot.robots[1].y);
    TargetRot0 = DirToRotation(Snapshot.robots[0].facing);
    TargetRot1 = DirToRotation(Snapshot.robots[1].facing);
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
            TriggerMuzzleFlash(Pos);
            SpawnProjectileBolt(Evt.robot, Pos, DirToRotation(CurrentSnapshot.robots[Evt.robot].facing).Vector());
            PlaySFX(AWVisualAssets::SFX_Fire, Pos);
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
            PlaySFX(AWVisualAssets::SFX_Move, Pos);
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
    if (RobotRoot0)
        RobotRoot0->SetWorldLocation(FVector::ZeroVector);
    if (RobotRoot1)
        RobotRoot1->SetWorldLocation(FVector::ZeroVector);
    TargetPos0 = FVector::ZeroVector;
    TargetPos1 = FVector::ZeroVector;
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

            if (Grid[CellIdx] == Automata::CellType::Cover || Grid[CellIdx] == Automata::CellType::Wall)
            {
                FVector Pos = GridToWorld(X, Y);
                bool bIsWall = (Grid[CellIdx] == Automata::CellType::Wall);

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

                // Walls: full-height uniform cubes
                if (bIsWall)
                {
                    UStaticMeshComponent *Block = NewObject<UStaticMeshComponent>(this);
                    Block->SetupAttachment(GetRootComponent());
                    Block->SetStaticMesh(CubeMesh);
                    Block->SetWorldLocation(Pos + FVector(0, 0, 50));
                    Block->SetWorldScale3D(FVector(0.95f, 0.95f, 1.0f));
                    Block->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                    Block->RegisterComponent();
                    TintCover(Block, FLinearColor(0.04f, 0.04f, 0.05f));
                }
                else
                {
                    // Cover: 3 silhouette variants for visual interest
                    int32 Variant = CoverIdx % 3;
                    switch (Variant)
                    {
                    case 0: // Tall narrow pillar
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
                    case 1: // L-shaped: two cubes offset
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
                    default: // Low wide crate
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
                }

                ++CoverIdx;
            }
        }
    }
}

void AAWArenaRenderer::BuildRobotVisuals()
{
    UStaticMesh *CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    UStaticMesh *CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    UMaterialInterface *RobotMaterial = LoadObject<UMaterialInterface>(nullptr, AWVisualAssets::M_Robot);
    if (!CubeMesh)
        return;

    // Helper to create a sub-part
    auto MakePart = [&](USceneComponent *Parent, UStaticMesh *Mesh, FVector Loc, FVector Scale, FLinearColor Color) -> UStaticMeshComponent *
    {
        UStaticMeshComponent *Part = NewObject<UStaticMeshComponent>(this);
        Part->SetupAttachment(Parent);
        Part->SetStaticMesh(Mesh);
        Part->SetRelativeLocation(Loc);
        Part->SetRelativeScale3D(Scale);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->RegisterComponent();
        if (RobotMaterial)
            Part->SetMaterial(0, RobotMaterial);
        UMaterialInstanceDynamic *Mat = Part->CreateDynamicMaterialInstance(0);
        if (Mat)
        {
            Mat->SetVectorParameterValue(TEXT("BaseColor"), Color);
            Mat->SetVectorParameterValue(TEXT("EmissiveColor"), Color * 0.5f);
        }
        return Part;
    };

    // Robot 0: tracked tank — broad body, two treads, shoulders, turret, barrel
    if (RobotRoot0)
    {
        FLinearColor CyanBase(0.0f, 0.3f, 0.4f);
        FLinearColor CyanDark(0.0f, 0.15f, 0.2f);
        FLinearColor CyanBright(0.0f, 0.6f, 0.7f);

        // Main body (broad box)
        UStaticMeshComponent *Body = MakePart(RobotRoot0, CubeMesh, FVector(0, 0, 22), FVector(0.7f, 1.1f, 0.35f), CyanBase);
        RobotMat0 = Body->CreateDynamicMaterialInstance(0);
        if (RobotMat0)
        {
            RobotMat0->SetVectorParameterValue(TEXT("BaseColor"), CyanBase);
            RobotMat0->SetVectorParameterValue(TEXT("EmissiveColor"), AWUIColors::AccentCyan * 2.f);
        }

        // Left tread
        MakePart(RobotRoot0, CubeMesh, FVector(0, -60, 12), FVector(0.9f, 0.15f, 0.2f), CyanDark);
        // Right tread
        MakePart(RobotRoot0, CubeMesh, FVector(0, 60, 12), FVector(0.9f, 0.15f, 0.2f), CyanDark);
        // Shoulders (wider top plate)
        MakePart(RobotRoot0, CubeMesh, FVector(0, 0, 38), FVector(0.5f, 1.3f, 0.12f), CyanBright);
        // Turret base
        MakePart(RobotRoot0, CubeMesh, FVector(10, 0, 46), FVector(0.35f, 0.35f, 0.2f), CyanBase);
        // Barrel
        MakePart(RobotRoot0, CubeMesh, FVector(50, 0, 48), FVector(0.6f, 0.1f, 0.08f), CyanDark);
    }

    // Robot 1: tripod walker — tall cylindrical body, three splayed legs, head, barrel
    if (RobotRoot1)
    {
        FLinearColor CoralBase(0.4f, 0.1f, 0.08f);
        FLinearColor CoralDark(0.2f, 0.05f, 0.04f);
        FLinearColor CoralBright(0.7f, 0.2f, 0.15f);

        UStaticMesh *BodyMesh = CylinderMesh ? CylinderMesh : CubeMesh;

        // Tall central body
        UStaticMeshComponent *Body = MakePart(RobotRoot1, BodyMesh, FVector(0, 0, 45), FVector(0.35f, 0.35f, 0.7f), CoralBase);
        RobotMat1 = Body->CreateDynamicMaterialInstance(0);
        if (RobotMat1)
        {
            RobotMat1->SetVectorParameterValue(TEXT("BaseColor"), CoralBase);
            RobotMat1->SetVectorParameterValue(TEXT("EmissiveColor"), AWUIColors::AccentCoral * 2.f);
        }

        // Three splayed legs (120 degrees apart)
        for (int32 i = 0; i < 3; ++i)
        {
            float Angle = i * 120.f;
            float Rad = FMath::DegreesToRadians(Angle);
            FVector LegOffset(FMath::Cos(Rad) * 40.f, FMath::Sin(Rad) * 40.f, 8.f);
            MakePart(RobotRoot1, CubeMesh, LegOffset, FVector(0.12f, 0.12f, 0.35f), CoralDark);
            // Hover pad at bottom of each leg
            FVector PadOffset(FMath::Cos(Rad) * 45.f, FMath::Sin(Rad) * 45.f, 2.f);
            MakePart(RobotRoot1, CubeMesh, PadOffset, FVector(0.2f, 0.2f, 0.05f), CoralBright);
        }

        // Head dome
        MakePart(RobotRoot1, BodyMesh, FVector(0, 0, 72), FVector(0.22f, 0.22f, 0.2f), CoralBright);
        // Barrel
        MakePart(RobotRoot1, CubeMesh, FVector(40, 0, 65), FVector(0.5f, 0.08f, 0.06f), CoralDark);
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
    Bolt->SetWorldLocation(WorldPos + FVector(0, 0, AWVisualConfig::ProjectileZ));
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

void AAWArenaRenderer::TriggerMuzzleFlash(FVector WorldPos)
{
    UNiagaraSystem *NS = LoadObject<UNiagaraSystem>(nullptr, AWVisualAssets::NS_MuzzleFlash);
    if (NS)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), NS, WorldPos);
    }
    else
    {
        UPointLightComponent *Flash = NewObject<UPointLightComponent>(this);
        Flash->SetupAttachment(GetRootComponent());
        Flash->SetWorldLocation(WorldPos + FVector(0, 0, 50));
        Flash->SetIntensity(8000.f);
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
    USceneComponent *RobotRoot = (RobotIdx == 0) ? RobotRoot0.Get() : RobotRoot1.Get();
    if (!RobotRoot)
        return;

    FVector Pos = RobotRoot->GetComponentLocation();

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

/**
 * @file AWArenaRenderer.cpp
 * @brief Implementation of the presentation-only arena renderer.
 */

#include "AWArenaRenderer.h"
#include "AWAPItemSpawner.h"
#include "AWCombatEffectsComponent.h"
#include "AWIsometricCamera.h"
#include "AWPlanProjectionComponent.h"
#include "AWTankActor.h"
#include "AWVisualTypes.h"
#include "TableObstable.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "EngineUtils.h"

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
    PrimaryActorTick.bCanEverTick = false;

    USceneComponent *Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    Root->SetMobility(EComponentMobility::Static);

    FloorMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("FloorMesh"));
    FloorMesh->SetupAttachment(Root);
    FloorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FloorMesh->bUseComplexAsSimpleCollision = false;

    CombatEffects = CreateDefaultSubobject<UAWCombatEffectsComponent>(TEXT("CombatEffects"));
    PlanProjection = CreateDefaultSubobject<UAWPlanProjectionComponent>(TEXT("PlanProjection"));

    ObstacleClass = ATableObstable::StaticClass();
}

void AAWArenaRenderer::BeginPlay()
{
    Super::BeginPlay();
    ResolveTankActors();
    ResolveActionPointItemSpawner();
}

void AAWArenaRenderer::InitializeArena(const Automata::SimConfig &Config,
                                       const TArray<Automata::CellType> &Grid,
                                       const TArray<Automata::SimEvent> &Events)
{
    ClearAllPlanProjections();
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
            Actor->SetActorLocation(FVector(
                ArenaCenter.X, ArenaOrigin.Y + (bNorth ? Height * CellSize + 75.f : -75.f), Location.Z));
            Actor->SetActorScale3D(FVector(Width + 2.f, Scale.Y, Scale.Z));
        }
        else if (Actor->ActorHasTag(ArenaRailWestTag) || Actor->ActorHasTag(ArenaRailEastTag))
        {
            const bool bEast = Actor->ActorHasTag(ArenaRailEastTag);
            Actor->SetActorLocation(FVector(
                ArenaOrigin.X + (bEast ? Width * CellSize + 75.f : -75.f), ArenaCenter.Y, Location.Z));
            Actor->SetActorScale3D(FVector(Scale.X, Height + 2.f, Scale.Z));
        }
        else if (Actor->ActorHasTag(ArenaDressingTag))
        {
            const FTransform &AuthoredTransform = AuthoredDressingTransforms.FindOrAdd(
                Actor, Actor->GetActorTransform());
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
    CombatEffects->SetSnapshot(Snapshot);
    for (const TPair<int32, TObjectPtr<ATableObstable>> &Entry : Obstacles)
        if (Entry.Value && Entry.Key >= 0 &&
            static_cast<size_t>(Entry.Key) < Snapshot.obstacleHealth.size())
            Entry.Value->SetHealth(Snapshot.obstacleHealth[static_cast<size_t>(Entry.Key)]);

    ResolveTankActors();
    if (PlayerOneTank)
        PlayerOneTank->SetTargetTransform(
            GridToWorld(Snapshot.robots[0].x, Snapshot.robots[0].y),
            DirToRotation(Snapshot.robots[0].facing));
    if (PlayerTwoTank)
        PlayerTwoTank->SetTargetTransform(
            GridToWorld(Snapshot.robots[1].x, Snapshot.robots[1].y),
            DirToRotation(Snapshot.robots[1].facing));
    const int32 ActiveRobot = Snapshot.robots[0].currentCommand != INDEX_NONE
                                  ? 0
                              : Snapshot.robots[1].currentCommand != INDEX_NONE ? 1
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
    CombatEffects->ProcessEvents(Events, FromStep, ToStep);
}

void AAWArenaRenderer::ResetVisuals()
{
    CombatEffects->ResetEffects();
    ResolveTankActors();
    if (PlayerOneTank)
        PlayerOneTank->ResetVisual();
    if (PlayerTwoTank)
        PlayerTwoTank->ResetVisual();
    if (ActionPointItemSpawner)
        ActionPointItemSpawner->SetReplayStep(INDEX_NONE);
    for (const TPair<int32, TObjectPtr<ATableObstable>> &Entry : Obstacles)
        if (Entry.Value)
            Entry.Value->ResetHealth();
}

void AAWArenaRenderer::UpdatePlanProjection(int32 RobotIndex, const Automata::RobotState &InitialRobot,
                                            const TArray<Automata::StepSnapshot> &Snapshots,
                                            const TArray<Automata::SimEvent> &Events, int32 AnimatedStep)
{
    PlanProjection->UpdateProjection(RobotIndex, InitialRobot, Snapshots, Events, AnimatedStep);
}

void AAWArenaRenderer::SetPlanProjectionVisible(int32 RobotIndex, bool bVisible)
{
    PlanProjection->SetProjectionVisible(RobotIndex, bVisible);
}

void AAWArenaRenderer::ClearPlanProjection(int32 RobotIndex)
{
    PlanProjection->ClearProjection(RobotIndex);
}

void AAWArenaRenderer::ClearAllPlanProjections()
{
    PlanProjection->ClearAllProjections();
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

    // CoverIdx drives deterministic color and silhouette variation.
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
                    CoverColor = FLinearColor(0.28f, 0.10f, 0.035f);
                    break;
                case 1:
                    CoverColor = FLinearColor(0.04f, 0.16f, 0.26f);
                    break;
                case 2:
                    CoverColor = FLinearColor(0.24f, 0.055f, 0.14f);
                    break;
                default:
                    CoverColor = FLinearColor(0.08f, 0.22f, 0.07f);
                    break;
                }

                FActorSpawnParameters SpawnParameters;
                SpawnParameters.Owner = this;
                SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                if (ATableObstable *Obstacle = GetWorld()->SpawnActor<ATableObstable>(ObstacleClass, Pos, FRotator::ZeroRotator, SpawnParameters))
                {
                    Obstacle->InitializeObstacle(CellIdx, CoverColor, CoverIdx);
                    Obstacles.Add(CellIdx, Obstacle);
                }

                ++CoverIdx;
            }
        }
    }
}

void AAWArenaRenderer::TriggerMuzzleFlash(AAWTankActor *Tank)
{
    CombatEffects->TriggerMuzzleFlash(Tank);
}

void AAWArenaRenderer::SetFinalEffects(const std::array<FAWRobotEffects, 2> &FinalEffects)
{
    CombatEffects->SetFinalEffects(FinalEffects);
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
        if (Tank->IsPlanProjection())
            continue;
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

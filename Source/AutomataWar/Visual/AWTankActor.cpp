/**
 * @file AWTankActor.cpp
 * @brief Implementation of the level-authored tank presentation actor.
 */

#include "AWTankActor.h"
#include "AWVisualTypes.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"

AAWTankActor::AAWTankActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(SceneRoot);
    SceneRoot->SetMobility(EComponentMobility::Movable);

    TankMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TankMesh"));
    TankMesh->SetupAttachment(SceneRoot);
    TankMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    CannonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CannonMesh"));
    CannonMesh->SetupAttachment(SceneRoot);
    CannonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    ActiveIndicator = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ActiveIndicator"));
    ActiveIndicator->SetupAttachment(SceneRoot);
    ActiveIndicator->SetRelativeLocation(FVector(0.f, 0.f, -46.f));
    ActiveIndicator->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ActiveIndicator->SetCastShadow(false);
    ActiveIndicator->SetVisibility(false);
}

void AAWTankActor::OnConstruction(const FTransform &Transform)
{
    Super::OnConstruction(Transform);
    ApplyVisualConfiguration();
}

void AAWTankActor::BeginPlay()
{
    Super::BeginPlay();

    InitializeTransformState();
    ApplyVisualConfiguration();
    BuildActiveIndicator();
    ApplyTankMaterial();
    ApplyIndicatorMaterial();
}

void AAWTankActor::InitializeTransformState()
{
    AuthoredTransform = GetActorTransform();
    TargetLocation = GetActorLocation();
    TargetRotation = GetActorRotation();
}

void AAWTankActor::ApplyTankMaterial()
{
    const TCHAR *MaterialPath = bPlanProjection ? AWVisualAssets::M_Effect : AWVisualAssets::M_Robot;
    if (UMaterialInterface *RobotMaterial = LoadObject<UMaterialInterface>(nullptr, MaterialPath))
    {
        DynamicMaterial = UMaterialInstanceDynamic::Create(RobotMaterial, this);
        if (bPlanProjection)
        {
            DynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), PlayerColor * 2.5f);
            DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), 0.26f);
        }
        else
        {
            DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), PlayerColor);
            DynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), PlayerColor * 0.45f);
            DynamicMaterial->SetScalarParameterValue(TEXT("Metallic"), 0.28f);
            DynamicMaterial->SetScalarParameterValue(TEXT("Roughness"), 0.52f);
        }
        for (UMeshComponent *Mesh : {static_cast<UMeshComponent *>(TankMesh), static_cast<UMeshComponent *>(CannonMesh)})
        {
            const int32 MaterialSlots = FMath::Max(Mesh->GetNumMaterials(), 1);
            for (int32 Slot = 0; Slot < MaterialSlots; ++Slot)
                Mesh->SetMaterial(Slot, DynamicMaterial);
            if (bPlanProjection)
            {
                Mesh->SetCastShadow(false);
                Mesh->SetReceivesDecals(false);
                Mesh->SetTranslucentSortPriority(20);
            }
        }
    }
}

void AAWTankActor::ApplyIndicatorMaterial()
{
    if (UMaterialInterface *EffectMaterial = LoadObject<UMaterialInterface>(nullptr, AWVisualAssets::M_Effect))
    {
        IndicatorMaterial = UMaterialInstanceDynamic::Create(EffectMaterial, this);
        IndicatorMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), PlayerColor * 6.f);
        IndicatorMaterial->SetScalarParameterValue(TEXT("Opacity"), 0.85f);
        ActiveIndicator->SetMaterial(0, IndicatorMaterial);
    }
}

void AAWTankActor::ConfigureAsPlanProjection(const AAWTankActor &SourceTank)
{
    RobotIndex = SourceTank.RobotIndex;
    TankAsset = SourceTank.TankAsset;
    CannonAsset = SourceTank.CannonAsset;
    MeshTransform = SourceTank.MeshTransform;
    CannonTransform = SourceTank.CannonTransform;
    PlayerColor = SourceTank.PlayerColor;
    bPlanProjection = true;
}

void AAWTankActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bHasTarget)
        return;

    const FVector Location = FMath::VInterpTo(GetActorLocation(), TargetLocation, DeltaTime, AWVisualConfig::InterpSpeed);
    const FRotator Rotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, AWVisualConfig::InterpSpeed);
    SetActorLocationAndRotation(Location, Rotation);

    if (ActiveIndicator->IsVisible())
    {
        const float Pulse = 1.f + FMath::Sin(GetWorld()->GetTimeSeconds() * 5.f) * 0.06f;
        ActiveIndicator->SetRelativeScale3D(FVector(Pulse, Pulse, 1.f));
    }
}

void AAWTankActor::SetTargetTransform(const FVector &Location, const FRotator &Rotation)
{
    TargetLocation = Location;
    TargetRotation = Rotation;

    if (!bHasTarget)
    {
        SetActorLocationAndRotation(TargetLocation, TargetRotation);
        bHasTarget = true;
    }
}

void AAWTankActor::ResetVisual()
{
    SetActorTransform(AuthoredTransform);
    TargetLocation = AuthoredTransform.GetLocation();
    TargetRotation = AuthoredTransform.Rotator();
    bHasTarget = false;
    SetActiveIndicator(false);
}

void AAWTankActor::SetActiveIndicator(bool bActive)
{
    ActiveIndicator->SetVisibility(bActive, true);
}

USceneComponent *AAWTankActor::GetMuzzleComponent() const
{
    return CannonMesh;
}

FName AAWTankActor::GetMuzzleSocketName() const
{
    static const FName MuzzleSocket(TEXT("Muzzle"));
    return MuzzleSocket;
}

FTransform AAWTankActor::GetMuzzleTransform() const
{
    return CannonMesh && CannonMesh->DoesSocketExist(GetMuzzleSocketName())
               ? CannonMesh->GetSocketTransform(GetMuzzleSocketName())
               : CannonMesh->GetComponentTransform();
}

void AAWTankActor::BuildActiveIndicator()
{
    constexpr int32 Segments = 40;
    constexpr float InnerRadius = 57.f;
    constexpr float OuterRadius = 66.f;
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FColor> Colors;
    Vertices.Reserve((Segments + 1) * 2);

    for (int32 Segment = 0; Segment <= Segments; ++Segment)
    {
        const float Angle = 2.f * PI * static_cast<float>(Segment) / static_cast<float>(Segments);
        const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
        Vertices.Add(Direction * InnerRadius);
        Vertices.Add(Direction * OuterRadius);
        Normals.Append({FVector::UpVector, FVector::UpVector});
        UVs.Append({FVector2D(0.f, static_cast<float>(Segment) / Segments),
                    FVector2D(1.f, static_cast<float>(Segment) / Segments)});
        Colors.Append({PlayerColor.ToFColor(true), PlayerColor.ToFColor(true)});

        if (Segment < Segments)
        {
            const int32 Base = Segment * 2;
            Triangles.Append({Base, Base + 3, Base + 1, Base, Base + 2, Base + 3});
        }
    }

    ActiveIndicator->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors,
                                       TArray<FProcMeshTangent>(), false);
}

void AAWTankActor::ApplyVisualConfiguration()
{
    TankMesh->SetStaticMesh(TankAsset);
    TankMesh->SetRelativeTransform(MeshTransform);
    CannonMesh->SetSkeletalMesh(CannonAsset);
    CannonMesh->SetRelativeTransform(CannonTransform);
}

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
}

void AAWTankActor::OnConstruction(const FTransform &Transform)
{
    Super::OnConstruction(Transform);
    ApplyVisualConfiguration();
}

void AAWTankActor::BeginPlay()
{
    Super::BeginPlay();

    AuthoredTransform = GetActorTransform();
    TargetLocation = GetActorLocation();
    TargetRotation = GetActorRotation();
    ApplyVisualConfiguration();

    if (UMaterialInterface *RobotMaterial = LoadObject<UMaterialInterface>(nullptr, AWVisualAssets::M_Robot))
    {
        DynamicMaterial = UMaterialInstanceDynamic::Create(RobotMaterial, this);
        DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), PlayerColor);
        DynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), PlayerColor * 2.f);
        for (UMeshComponent *Mesh : {static_cast<UMeshComponent *>(TankMesh), static_cast<UMeshComponent *>(CannonMesh)})
        {
            const int32 MaterialSlots = FMath::Max(Mesh->GetNumMaterials(), 1);
            for (int32 Slot = 0; Slot < MaterialSlots; ++Slot)
                Mesh->SetMaterial(Slot, DynamicMaterial);
        }
    }
}

void AAWTankActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bHasTarget)
        return;

    const FVector Location = FMath::VInterpTo(GetActorLocation(), TargetLocation, DeltaTime, AWVisualConfig::InterpSpeed);
    const FRotator Rotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, AWVisualConfig::InterpSpeed);
    SetActorLocationAndRotation(Location, Rotation);
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

void AAWTankActor::ApplyVisualConfiguration()
{
    TankMesh->SetStaticMesh(TankAsset);
    TankMesh->SetRelativeTransform(MeshTransform);
    CannonMesh->SetSkeletalMesh(CannonAsset);
    CannonMesh->SetRelativeTransform(CannonTransform);
}

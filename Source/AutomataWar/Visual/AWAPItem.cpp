#include "AWAPItem.h"
#include "AWVisualTypes.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

AAWAPItem::AAWAPItem()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(SceneRoot);
    SceneRoot->SetMobility(EComponentMobility::Movable);

    ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
    ItemMesh->SetupAttachment(SceneRoot);
    ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ItemMesh->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
    ItemMesh->SetRelativeScale3D(FVector(0.34f, 0.34f, 0.07f));
    ItemMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));

    PickupTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("PickupTrigger"));
    PickupTrigger->SetupAttachment(SceneRoot);
    PickupTrigger->InitSphereRadius(42.f);
    PickupTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    PickupTrigger->SetCollisionResponseToAllChannels(ECR_Overlap);

    ItemLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ItemLight"));
    ItemLight->SetupAttachment(SceneRoot);
    ItemLight->SetIntensity(1800.f);
    ItemLight->SetAttenuationRadius(150.f);
    ItemLight->SetLightColor(FLinearColor(0.35f, 1.f, 0.28f));
}

void AAWAPItem::BeginPlay()
{
    Super::BeginPlay();
    SpawnTransform = GetActorTransform();

    if (UMaterialInterface *Material = LoadObject<UMaterialInterface>(nullptr, AWVisualAssets::M_Effect))
    {
        UMaterialInstanceDynamic *DynamicMaterial = UMaterialInstanceDynamic::Create(Material, this);
        DynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(0.22f, 5.f, 0.4f, 1.f));
        DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), 0.9f);
        ItemMesh->SetMaterial(0, DynamicMaterial);
    }
}

void AAWAPItem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    RunningTime += DeltaTime;

    if (bAnimatingCollection)
    {
        CollectionAlpha = FMath::Min(1.f, CollectionAlpha + DeltaTime / CollectionDuration);
        const float EasedAlpha = FMath::InterpEaseOut(0.f, 1.f, CollectionAlpha, 3.f);
        SetActorLocation(SpawnTransform.GetLocation() + FVector(0.f, 0.f, 115.f * EasedAlpha));
        SetActorRotation(FRotator(0.f, RunningTime * RotationSpeed * 2.f, 0.f));
        SetActorScale3D(FVector(FMath::Max(0.01f, 1.f - EasedAlpha)));
        if (CollectionAlpha >= 1.f)
        {
            bAnimatingCollection = false;
            SetActorHiddenInGame(true);
        }
        return;
    }

    if (!bCollected)
    {
        const float BobOffset = FMath::Sin(RunningTime * BobSpeed) * BobHeight;
        SetActorLocation(SpawnTransform.GetLocation() + FVector(0.f, 0.f, BobOffset));
        SetActorRotation(FRotator(0.f, RunningTime * RotationSpeed, 0.f));
    }
}

void AAWAPItem::InitializeItem(int32 InCellIndex, int32 InRewardValue)
{
    CellIndex = InCellIndex;
    RewardValue = InRewardValue;
}

void AAWAPItem::SetCollected(bool bInCollected, bool bPlayEffects)
{
    if (bCollected == bInCollected)
        return;

    bCollected = bInCollected;
    PickupTrigger->SetCollisionEnabled(bCollected ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);
    ItemLight->SetVisibility(!bCollected);

    if (bCollected && bPlayEffects)
    {
        CollectionAlpha = 0.f;
        bAnimatingCollection = true;
        PlayPickupEffects();
        return;
    }

    bAnimatingCollection = false;
    SetActorHiddenInGame(bCollected);
    SetActorTransform(SpawnTransform);
}

void AAWAPItem::PlayPickupEffects() const
{
    if (USoundBase *Sound = LoadObject<USoundBase>(nullptr, AWVisualAssets::SFX_ActionPointPickup))
        UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation(), 0.9f, 1.35f);
    if (UNiagaraSystem *Effect = LoadObject<UNiagaraSystem>(nullptr, AWVisualAssets::NS_ActionPointPickup))
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Effect, GetActorLocation(), FRotator::ZeroRotator,
                                                       FVector(0.7f), true, true);
}
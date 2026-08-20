#include "AWItem.h"
#include "AWVisualTypes.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

AAWItem::AAWItem()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(SceneRoot);
    SceneRoot->SetMobility(EComponentMobility::Movable);
    SceneRoot->bEditableWhenInherited = true;

    ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
    ItemMesh->SetupAttachment(SceneRoot);
    ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ItemMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
    ItemMesh->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
    ItemMesh->SetRelativeScale3D(FVector(0.34f, 0.34f, 0.07f));
    ItemMesh->bEditableWhenInherited = true;

    PickupTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("PickupTrigger"));
    PickupTrigger->SetupAttachment(SceneRoot);
    PickupTrigger->InitSphereRadius(42.f);
    PickupTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PickupTrigger->bEditableWhenInherited = true;

    ItemLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ItemLight"));
    ItemLight->SetupAttachment(SceneRoot);
    ItemLight->SetIntensity(160.f);
    ItemLight->SetAttenuationRadius(72.f);
    ItemLight->bEditableWhenInherited = true;

    PickupEffectPath = AWVisualAssets::NS_ActionPointPickup;
}

void AAWItem::BeginPlay()
{
    Super::BeginPlay();
    SpawnTransform = GetActorTransform();
    ItemLight->SetLightColor(ItemColor);
}

void AAWItem::Tick(float DeltaTime)
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

void AAWItem::InitializeItem(int32 InCellIndex, int32 InValue)
{
    CellIndex = InCellIndex;
    Value = InValue;
}

void AAWItem::SetCollected(bool bInCollected, bool bPlayEffects)
{
    if (bCollected == bInCollected)
        return;

    bCollected = bInCollected;
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

void AAWItem::PlayPickupEffects() const
{
    if (USoundBase *Sound = LoadObject<USoundBase>(nullptr, AWVisualAssets::SFX_ActionPointPickup))
        UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation(), 0.9f, PickupSoundPitch);
    if (UNiagaraSystem *Effect = LoadObject<UNiagaraSystem>(nullptr, *PickupEffectPath))
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Effect, GetActorLocation(), FRotator::ZeroRotator,
                                                       FVector(0.7f), true, true);
}
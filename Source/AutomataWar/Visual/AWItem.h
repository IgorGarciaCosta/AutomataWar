#pragma once

/**
 * @file AWItem.h
 * @brief Shared presentation behavior and concrete visual power-up actors.
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AWItem.generated.h"

class UPointLightComponent;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;

/**
 * Presentation-only base for deterministic arena pickups.
 *
 * The simulation owns collection and effects. This actor mirrors replay events,
 * animates collection, and supplies common mesh/light/VFX behavior without
 * participating in collision or authoritative gameplay.
 */
UCLASS(Abstract, Blueprintable)
class AUTOMATAWAR_API AAWItem : public AActor
{
    GENERATED_BODY()

public:
    /** Create the shared mesh, trigger, and light component hierarchy. */
    AAWItem();

    /** Cache the authored spawn transform and apply the item's visual identity. */
    virtual void BeginPlay() override;
    /** Animate idle rotation/bobbing or the one-shot collection transition. */
    virtual void Tick(float DeltaTime) override;

    /** Associate this visual with a canonical grid cell and optional numeric value. */
    virtual void InitializeItem(int32 InCellIndex, int32 InValue = 0);

    /** Mirror replay collection state and optionally play one-shot feedback. */
    void SetCollected(bool bInCollected, bool bPlayEffects);

    /** Return whether this item is hidden because its replay event has occurred. */
    bool IsCollected() const { return bCollected; }
    /** Return the canonical flattened grid index represented by this actor. */
    int32 GetCellIndex() const { return CellIndex; }

protected:
    /** Spawn the configured pickup Niagara system and shared collection sound. */
    virtual void PlayPickupEffects() const;

    UPROPERTY(VisibleAnywhere, Category = "Item")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = "Item")
    TObjectPtr<UStaticMeshComponent> ItemMesh;

    UPROPERTY(VisibleAnywhere, Category = "Item")
    TObjectPtr<USphereComponent> PickupTrigger;

    UPROPERTY(VisibleAnywhere, Category = "Item")
    TObjectPtr<UPointLightComponent> ItemLight;

    UPROPERTY(EditDefaultsOnly, Category = "Item|Visual")
    FLinearColor ItemColor = FLinearColor(0.35f, 1.f, 0.28f);

    UPROPERTY(EditDefaultsOnly, Category = "Item|Visual")
    FString PickupEffectPath;

    UPROPERTY(EditDefaultsOnly, Category = "Item|Visual")
    float PickupSoundPitch = 1.f;

private:
    UPROPERTY(EditDefaultsOnly, Category = "Item|Animation")
    float RotationSpeed = 150.f;

    UPROPERTY(EditDefaultsOnly, Category = "Item|Animation")
    float BobHeight = 10.f;

    UPROPERTY(EditDefaultsOnly, Category = "Item|Animation")
    float BobSpeed = 2.5f;

    UPROPERTY(EditDefaultsOnly, Category = "Item|Animation")
    float CollectionDuration = 0.42f;

    int32 CellIndex = INDEX_NONE;
    int32 Value = 0;
    FTransform SpawnTransform;
    float RunningTime = 0.f;
    float CollectionAlpha = 0.f;
    bool bCollected = false;
    bool bAnimatingCollection = false;
};

/** Orange ammunition pickup that grants two rounds of bonus shot damage. */
UCLASS(Blueprintable)
class AUTOMATAWAR_API AAWExtraAmmoItem : public AAWItem
{
    GENERATED_BODY()

public:
    /** Configure the ammunition pickup's warm color and compact crate silhouette. */
    AAWExtraAmmoItem();
};

/** Cyan shield pickup that grants two rounds of incoming damage reduction. */
UCLASS(Blueprintable)
class AUTOMATAWAR_API AAWShieldItem : public AAWItem
{
    GENERATED_BODY()

public:
    /** Configure the shield pickup's cool color, sphere silhouette, and shield VFX. */
    AAWShieldItem();
};

/** Amber accelerator pickup that doubles movement distance for two rounds. */
UCLASS(Blueprintable)
class AUTOMATAWAR_API AAWAcceleratorItem : public AAWItem
{
    GENERATED_BODY()

public:
    /** Configure the accelerator pickup's amber color and directional cone silhouette. */
    AAWAcceleratorItem();
};
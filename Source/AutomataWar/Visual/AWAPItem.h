#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AWAPItem.generated.h"

class UPointLightComponent;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;

/** Presentation actor for one deterministic action-point pickup. */
UCLASS(Blueprintable)
class AUTOMATAWAR_API AAWAPItem : public AActor
{
    GENERATED_BODY()

public:
    AAWAPItem();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    void InitializeItem(int32 InCellIndex, int32 InRewardValue);
    void SetCollected(bool bInCollected, bool bPlayEffects);

    bool IsCollected() const { return bCollected; }
    int32 GetCellIndex() const { return CellIndex; }

private:
    void PlayPickupEffects() const;

    UPROPERTY(VisibleAnywhere, Category = "Action Points")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = "Action Points")
    TObjectPtr<UStaticMeshComponent> ItemMesh;

    UPROPERTY(VisibleAnywhere, Category = "Action Points")
    TObjectPtr<USphereComponent> PickupTrigger;

    UPROPERTY(VisibleAnywhere, Category = "Action Points")
    TObjectPtr<UPointLightComponent> ItemLight;

    UPROPERTY(EditDefaultsOnly, Category = "Action Points")
    float RotationSpeed = 150.f;

    UPROPERTY(EditDefaultsOnly, Category = "Action Points")
    float BobHeight = 10.f;

    UPROPERTY(EditDefaultsOnly, Category = "Action Points")
    float BobSpeed = 2.5f;

    UPROPERTY(EditDefaultsOnly, Category = "Action Points")
    float CollectionDuration = 0.42f;

    int32 CellIndex = INDEX_NONE;
    int32 RewardValue = 0;
    FTransform SpawnTransform;
    float RunningTime = 0.f;
    float CollectionAlpha = 0.f;
    bool bCollected = false;
    bool bAnimatingCollection = false;
};
#pragma once

/**
 * @file AWTankActor.h
 * @brief Level-authored presentation actor for one simulated tank.
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AWTankActor.generated.h"

class UMaterialInstanceDynamic;
class USceneComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * Visual representation of one deterministic simulation robot.
 *
 * Instances are placed in the arena map and driven by immutable simulation
 * snapshots. Gameplay collision and authoritative state remain in Core.
 */
UCLASS(Blueprintable)
class AUTOMATAWAR_API AAWTankActor : public AActor
{
    GENERATED_BODY()

public:
    AAWTankActor();

    virtual void OnConstruction(const FTransform &Transform) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    /** Move smoothly toward the transform represented by the latest snapshot. */
    void SetTargetTransform(const FVector &Location, const FRotator &Rotation);

    /** Restore the transform authored in the arena map. */
    void ResetVisual();

    /** Return the simulation robot slot represented by this actor. */
    int32 GetRobotIndex() const { return RobotIndex; }

    /** Return the component and socket used as the visual firing origin. */
    USceneComponent *GetMuzzleComponent() const;
    FName GetMuzzleSocketName() const;
    FTransform GetMuzzleTransform() const;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tank")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tank")
    TObjectPtr<UStaticMeshComponent> TankMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tank")
    TObjectPtr<USkeletalMeshComponent> CannonMesh;

    /** Simulation slot represented by this level instance. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tank", meta = (ClampMin = "0", ClampMax = "1"))
    int32 RobotIndex = 0;

    /** Mesh assigned to this tank instance in the arena map. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tank")
    TObjectPtr<UStaticMesh> TankAsset;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tank")
    TObjectPtr<USkeletalMesh> CannonAsset;

    /** Imported-mesh alignment relative to the actor origin. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tank")
    FTransform MeshTransform = FTransform::Identity;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tank")
    FTransform CannonTransform = FTransform::Identity;

    /** Player color applied to the mesh. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tank")
    FLinearColor PlayerColor = FLinearColor(0.f, 0.8f, 1.f);

private:
    void ApplyVisualConfiguration();

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

    FTransform AuthoredTransform;
    FVector TargetLocation = FVector::ZeroVector;
    FRotator TargetRotation = FRotator::ZeroRotator;
    bool bHasTarget = false;
};
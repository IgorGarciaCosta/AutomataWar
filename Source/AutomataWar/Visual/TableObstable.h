#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TableObstable.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UNiagaraSystem;
class USceneComponent;
class UStaticMeshComponent;
class UTableObstableHealthWidget;
class UWidgetComponent;

/** Runtime presentation actor for one destructible cover cell. */
UCLASS(Blueprintable)
class AUTOMATAWAR_API ATableObstable : public AActor
{
    GENERATED_BODY()

public:
    ATableObstable();

    virtual void OnConstruction(const FTransform &Transform) override;

    /** Configure the grid identity and deterministic visual variant selected by the arena renderer. */
    void InitializeObstacle(int32 InCellIndex, const FLinearColor &InColor, int32 InVisualVariant);

    /** Apply replay-visible health when a projectile arrives or navigation restores a snapshot. */
    void SetHealth(int32 NewHealth);

    /** Restore the initial visible state. */
    void ResetHealth();

    int32 GetCellIndex() const { return CellIndex; }
    int32 GetHealth() const { return CurrentHealth; }
    int32 GetMaxHealth() const { return MaxHealth; }

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Obstacle")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Obstacle")
    TObjectPtr<UStaticMeshComponent> ObstacleMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Obstacle")
    TObjectPtr<UWidgetComponent> HealthWidget;

    /** Widget Blueprint rendered by HealthWidget. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Obstacle|UI")
    TSubclassOf<UTableObstableHealthWidget> HealthWidgetClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Obstacle|Visual")
    TObjectPtr<UMaterialInterface> ObstacleMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Obstacle|Visual")
    TObjectPtr<UNiagaraSystem> ExplosionEffect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Obstacle", meta = (ClampMin = "1"))
    int32 MaxHealth;

private:
    void RefreshHealthWidget();
    void Explode();

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

    int32 CellIndex = INDEX_NONE;
    int32 CurrentHealth = 0;
    bool bDestroyed = false;
};
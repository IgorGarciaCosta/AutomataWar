#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AutomataWar/Core/Sim/AutomataSimulation.h"
#include "AWAPItemSpawner.generated.h"

class AAWItem;
class USceneComponent;

/** Spawns all replay-visible pickup actors from canonical item grid cells. */
UCLASS(Blueprintable)
class AUTOMATAWAR_API AAWAPItemSpawner : public AActor
{
    GENERATED_BODY()

public:
    AAWAPItemSpawner();

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    void InitializeItems(const Automata::SimConfig &Config, const TArray<Automata::CellType> &Grid,
                         const TArray<Automata::SimEvent> &Events);
    void SetReplayStep(int32 Step);

private:
    struct FItemState
    {
        TWeakObjectPtr<AAWItem> Item;
        int32 CollectedStep = MAX_int32;
        int32 RewardValue = 0;
    };

    void DestroyItems();

    UPROPERTY(VisibleAnywhere, Category = "Action Points")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(EditDefaultsOnly, Category = "Action Points")
    TSubclassOf<AAWItem> ItemClass;

    UPROPERTY(EditDefaultsOnly, Category = "Items")
    TSubclassOf<AAWItem> ExtraAmmoItemClass;

    UPROPERTY(EditDefaultsOnly, Category = "Items")
    TSubclassOf<AAWItem> ShieldItemClass;

    UPROPERTY(EditDefaultsOnly, Category = "Items")
    TSubclassOf<AAWItem> AcceleratorItemClass;

    UPROPERTY(EditDefaultsOnly, Category = "Action Points")
    float ItemHeight = 42.f;

    TMap<int32, FItemState> Items;
    int32 LastReplayStep = INDEX_NONE;
};
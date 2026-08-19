#include "AWAPItemSpawner.h"
#include "AWItem.h"
#include "AWVisualTypes.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

AAWAPItemSpawner::AAWAPItemSpawner()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(SceneRoot);
    ItemClass = AAWItem::StaticClass();
    ExtraAmmoItemClass = AAWItem::StaticClass();
    ShieldItemClass = AAWItem::StaticClass();
    AcceleratorItemClass = AAWItem::StaticClass();

    static ConstructorHelpers::FClassFinder<AAWItem> ActionPointBlueprint(TEXT("/Game/Blueprints/Items/BP_ActionPointItem"));
    static ConstructorHelpers::FClassFinder<AAWItem> ExtraAmmoBlueprint(TEXT("/Game/Blueprints/Items/BP_ExtraAmmoItem"));
    static ConstructorHelpers::FClassFinder<AAWItem> ShieldBlueprint(TEXT("/Game/Blueprints/Items/BP_ShieldItem"));
    static ConstructorHelpers::FClassFinder<AAWItem> AcceleratorBlueprint(TEXT("/Game/Blueprints/Items/BP_AcceleratorItem"));
    if (ActionPointBlueprint.Succeeded())
        ItemClass = ActionPointBlueprint.Class;
    if (ExtraAmmoBlueprint.Succeeded())
        ExtraAmmoItemClass = ExtraAmmoBlueprint.Class;
    if (ShieldBlueprint.Succeeded())
        ShieldItemClass = ShieldBlueprint.Class;
    if (AcceleratorBlueprint.Succeeded())
        AcceleratorItemClass = AcceleratorBlueprint.Class;
}

void AAWAPItemSpawner::BeginPlay()
{
    Super::BeginPlay();

    if (SpawnSeed == 0)
    {
        const uint64 RandomSeed = static_cast<uint64>(FMath::Rand()) ^ (static_cast<uint64>(FMath::Rand()) << 32);
        SpawnSeed = static_cast<int64>(RandomSeed & MAX_int64);
        if (SpawnSeed == 0)
            SpawnSeed = 1;
    }

    Automata::SimConfig Config;
    Config.seed = static_cast<uint64>(SpawnSeed);
    const TArray<EAWCommand> EmptyCommands;
    Automata::Simulation Preview;
    Preview.RunMatch(EmptyCommands, EmptyCommands, Config);

    TArray<Automata::CellType> Grid;
    Grid.Reserve(Preview.GetGrid().size());
    for (Automata::CellType Cell : Preview.GetGrid())
        Grid.Add(Cell);
    InitializeItems(Config, Grid, {});
}

void AAWAPItemSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    DestroyItems();
    Super::EndPlay(EndPlayReason);
}

void AAWAPItemSpawner::InitializeItems(const Automata::SimConfig &Config, const TArray<Automata::CellType> &Grid,
                                       const TArray<Automata::SimEvent> &Events)
{
    DestroyItems();
    if (!ItemClass || Config.gridWidth <= 0)
        return;

    TMap<int32, TPair<int32, int32>> Collections;
    for (const Automata::SimEvent &Event : Events)
        if (Event.type == Automata::EventType::ActionPointsCollected ||
            Event.type == Automata::EventType::ExtraAmmoCollected ||
            Event.type == Automata::EventType::ShieldCollected ||
            Event.type == Automata::EventType::AcceleratorCollected)
            Collections.Add(Event.paramA, {Event.step, Event.paramB});

    for (int32 CellIndex = 0; CellIndex < Grid.Num(); ++CellIndex)
    {
        TSubclassOf<AAWItem> SpawnClass;
        switch (Grid[CellIndex])
        {
        case Automata::CellType::ActionPointItem:
            SpawnClass = ItemClass;
            break;
        case Automata::CellType::ExtraAmmoItem:
            SpawnClass = ExtraAmmoItemClass;
            break;
        case Automata::CellType::ShieldItem:
            SpawnClass = ShieldItemClass;
            break;
        case Automata::CellType::AcceleratorItem:
            SpawnClass = AcceleratorItemClass;
            break;
        default:
            continue;
        }

        const int32 X = CellIndex % Config.gridWidth;
        const int32 Y = CellIndex / Config.gridWidth;
        const FVector Location = GetActorLocation() + FVector(
                                                          X * AWVisualConfig::CellSize + AWVisualConfig::CellSize * 0.5f,
                                                          Y * AWVisualConfig::CellSize + AWVisualConfig::CellSize * 0.5f,
                                                          ItemHeight);

        FActorSpawnParameters SpawnParameters;
        SpawnParameters.Owner = this;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AAWItem *Item = GetWorld()->SpawnActor<AAWItem>(SpawnClass, Location, FRotator::ZeroRotator, SpawnParameters);
        if (!Item)
            continue;

        FItemState State;
        State.Item = Item;
        if (const TPair<int32, int32> *Collection = Collections.Find(CellIndex))
        {
            State.CollectedStep = Collection->Key;
            State.RewardValue = Collection->Value;
        }
        Item->InitializeItem(CellIndex, State.RewardValue);
        Items.Add(CellIndex, State);
    }

    LastReplayStep = INDEX_NONE;
}

void AAWAPItemSpawner::SetReplayStep(int32 Step)
{
    const bool bMovingForward = LastReplayStep == INDEX_NONE || Step >= LastReplayStep;
    for (TPair<int32, FItemState> &Entry : Items)
    {
        if (AAWItem *Item = Entry.Value.Item.Get())
        {
            const bool bShouldBeCollected = Entry.Value.CollectedStep <= Step;
            const bool bPlayEffects = bMovingForward && bShouldBeCollected && !Item->IsCollected();
            Item->SetCollected(bShouldBeCollected, bPlayEffects);
        }
    }
    LastReplayStep = Step;
}

void AAWAPItemSpawner::DestroyItems()
{
    for (const TPair<int32, FItemState> &Entry : Items)
        if (AAWItem *Item = Entry.Value.Item.Get())
            Item->Destroy();
    Items.Reset();
    LastReplayStep = INDEX_NONE;
}
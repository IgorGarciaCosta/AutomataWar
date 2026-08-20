#include "TableObstable.h"

#include "AWVisualTypes.h"
#include "AutomataWar/Core/AutomataRules.h"
#include "AutomataWar/UI/TableObstableHealthWidget.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

ATableObstable::ATableObstable()
{
    PrimaryActorTick.bCanEverTick = false;
    MaxHealth = Automata::ObstacleMaxHealth;
    CurrentHealth = MaxHealth;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(SceneRoot);

    ObstacleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ObstacleMesh"));
    ObstacleMesh->SetupAttachment(SceneRoot);
    ObstacleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ObstacleMesh->SetRelativeLocation(FVector(0.f, 0.f, 20.f));
    ObstacleMesh->SetRelativeScale3D(FVector(0.85f, 0.85f, 0.4f));
    ObstacleMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));

    HealthWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthWidget"));
    HealthWidget->SetupAttachment(SceneRoot);
    HealthWidget->SetRelativeLocation(FVector(0.f, 0.f, 70.f));
    HealthWidget->SetWidgetSpace(EWidgetSpace::Screen);
    HealthWidget->SetDrawSize(FVector2D(90.f, 10.f));
    HealthWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    ObstacleMaterial = LoadObject<UMaterialInterface>(nullptr, AWVisualAssets::M_Cover);
    ExplosionEffect = LoadObject<UNiagaraSystem>(nullptr, AWVisualAssets::NS_Destruction);
}

void ATableObstable::OnConstruction(const FTransform &Transform)
{
    Super::OnConstruction(Transform);
    HealthWidget->SetWidgetClass(HealthWidgetClass);
}

void ATableObstable::BeginPlay()
{
    Super::BeginPlay();
    HealthWidget->SetWidgetClass(HealthWidgetClass);
    RefreshHealthWidget();
}

void ATableObstable::InitializeObstacle(int32 InCellIndex, const FLinearColor &InColor, int32 InVisualVariant)
{
    CellIndex = InCellIndex;

    const int32 VisualVariant = FMath::Abs(InVisualVariant) % 4;
    const FVector VariantScales[] = {
        FVector(0.82f, 0.82f, 0.56f),
        FVector(0.88f, 0.48f, 0.52f),
        FVector(0.48f, 0.88f, 0.52f),
        FVector(0.62f, 0.62f, 0.72f)};
    const FVector &Scale = VariantScales[VisualVariant];
    ObstacleMesh->SetRelativeScale3D(Scale);
    ObstacleMesh->SetRelativeLocation(FVector(0.f, 0.f, Scale.Z * 50.f));
    HealthWidget->SetRelativeLocation(FVector(0.f, 0.f, Scale.Z * 100.f + 20.f));

    if (ObstacleMaterial)
    {
        ObstacleMesh->SetMaterial(0, ObstacleMaterial);
        DynamicMaterial = ObstacleMesh->CreateDynamicMaterialInstance(0);
        if (DynamicMaterial)
            DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), InColor);
    }

    ResetHealth();
}

void ATableObstable::SetHealth(int32 NewHealth)
{
    const int32 ClampedHealth = FMath::Clamp(NewHealth, 0, MaxHealth);
    const bool bWasAlive = CurrentHealth > 0;
    CurrentHealth = ClampedHealth;

    if (bDestroyed && CurrentHealth > 0)
    {
        bDestroyed = false;
        ObstacleMesh->SetVisibility(true);
        HealthWidget->SetVisibility(true);
    }

    RefreshHealthWidget();
    if (bWasAlive && CurrentHealth == 0)
        Explode();
}

void ATableObstable::ResetHealth()
{
    bDestroyed = false;
    CurrentHealth = MaxHealth;
    ObstacleMesh->SetVisibility(true);
    HealthWidget->SetVisibility(true);
    RefreshHealthWidget();
}

void ATableObstable::RefreshHealthWidget()
{
    HealthWidget->InitWidget();
    if (UTableObstableHealthWidget *Widget = Cast<UTableObstableHealthWidget>(HealthWidget->GetUserWidgetObject()))
        Widget->SetHealthPercent(MaxHealth > 0 ? static_cast<float>(CurrentHealth) / MaxHealth : 0.f);
}

void ATableObstable::Explode()
{
    bDestroyed = true;
    if (ExplosionEffect)
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());

    ObstacleMesh->SetVisibility(false);
    HealthWidget->SetVisibility(false);
}
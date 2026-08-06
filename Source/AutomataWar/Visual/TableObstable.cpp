#include "TableObstable.h"

#include "AWVisualTypes.h"
#include "AutomataWar/Core/AutomataRules.h"
#include "AutomataWar/UI/TableObstableHealthWidget.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
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

    HealthWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthWidget"));
    HealthWidget->SetupAttachment(SceneRoot);
    HealthWidget->SetRelativeLocation(FVector(0.f, 0.f, 70.f));
    HealthWidget->SetWidgetSpace(EWidgetSpace::Screen);
    HealthWidget->SetDrawSize(FVector2D(90.f, 10.f));
    HealthWidget->SetWidgetClass(UTableObstableHealthWidget::StaticClass());
    HealthWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ATableObstable::BeginPlay()
{
    Super::BeginPlay();
    RefreshHealthWidget();
}

void ATableObstable::InitializeObstacle(int32 InCellIndex, UStaticMesh *InMesh, const FVector &InScale, const FLinearColor &InColor)
{
    CellIndex = InCellIndex;
    ObstacleMesh->SetStaticMesh(InMesh);
    ObstacleMesh->SetRelativeScale3D(InScale);

    if (UMaterialInterface *CoverMaterial = LoadObject<UMaterialInterface>(nullptr, AWVisualAssets::M_Cover))
    {
        ObstacleMesh->SetMaterial(0, CoverMaterial);
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
    if (UNiagaraSystem *Explosion = LoadObject<UNiagaraSystem>(nullptr, AWVisualAssets::NS_Destruction))
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Explosion, GetActorLocation());

    ObstacleMesh->SetVisibility(false);
    HealthWidget->SetVisibility(false);
}
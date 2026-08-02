/**
 * @file AWArenaRenderer.cpp
 * @brief Implementation of the presentation-only arena renderer.
 */

#include "AWArenaRenderer.h"
#include "AWVisualTypes.h"
#include "AutomataWar/UI/AWUITypes.h"
#include "ProceduralMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Sound/SoundBase.h"

AAWArenaRenderer::AAWArenaRenderer()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Root->SetMobility(EComponentMobility::Static);

	FloorMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("FloorMesh"));
	FloorMesh->SetupAttachment(Root);
	FloorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FloorMesh->bUseComplexAsSimpleCollision = false;

	RobotRoot0 = CreateDefaultSubobject<USceneComponent>(TEXT("RobotRoot0"));
	RobotRoot0->SetupAttachment(Root);

	RobotRoot1 = CreateDefaultSubobject<USceneComponent>(TEXT("RobotRoot1"));
	RobotRoot1->SetupAttachment(Root);
}

void AAWArenaRenderer::BeginPlay()
{
	Super::BeginPlay();
	BuildRobotVisuals();
}

void AAWArenaRenderer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Smooth interpolation toward target positions
	const float Alpha = FMath::Clamp(DeltaTime * AWVisualConfig::InterpSpeed, 0.f, 1.f);

	if (RobotRoot0)
	{
		FVector Cur = RobotRoot0->GetComponentLocation();
		RobotRoot0->SetWorldLocation(FMath::Lerp(Cur, TargetPos0, Alpha));
		RobotRoot0->SetWorldRotation(FMath::Lerp(RobotRoot0->GetComponentRotation(), TargetRot0, Alpha));
	}
	if (RobotRoot1)
	{
		FVector Cur = RobotRoot1->GetComponentLocation();
		RobotRoot1->SetWorldLocation(FMath::Lerp(Cur, TargetPos1, Alpha));
		RobotRoot1->SetWorldRotation(FMath::Lerp(RobotRoot1->GetComponentRotation(), TargetRot1, Alpha));
	}
}

void AAWArenaRenderer::InitializeArena(const Automata::SimConfig& Config, const TArray<Automata::CellType>& Grid)
{
	GridWidth = Config.gridWidth;
	GridHeight = Config.gridHeight;
	BuildFloorGrid(GridWidth, GridHeight);
	SpawnCoverVisuals(GridWidth, GridHeight, Grid);
}

void AAWArenaRenderer::SetSnapshot(const Automata::TickSnapshot& Snapshot)
{
	CurrentSnapshot = Snapshot;
	TargetPos0 = GridToWorld(Snapshot.robots[0].x, Snapshot.robots[0].y);
	TargetPos1 = GridToWorld(Snapshot.robots[1].x, Snapshot.robots[1].y);
	TargetRot0 = DirToRotation(Snapshot.robots[0].facing);
	TargetRot1 = DirToRotation(Snapshot.robots[1].facing);
}

void AAWArenaRenderer::ProcessEvents(const TArray<Automata::SimEvent>& Events, int32 FromTick, int32 ToTick)
{
	for (const Automata::SimEvent& Evt : Events)
	{
		if (Evt.tick < FromTick || Evt.tick > ToTick) continue;

		const auto& Robot = CurrentSnapshot.robots[Evt.robot];
		FVector Pos = GridToWorld(Robot.x, Robot.y);

		switch (Evt.type)
		{
		case Automata::EventType::Fire:
			TriggerMuzzleFlash(Pos);
			PlaySFX(AWVisualAssets::SFX_Fire, Pos);
			break;
		case Automata::EventType::Hit:
			TriggerImpact(Pos);
			PlaySFX(AWVisualAssets::SFX_Impact, Pos);
			break;
		case Automata::EventType::ShieldActivate:
			TriggerShieldBubble(Evt.robot);
			PlaySFX(AWVisualAssets::SFX_Shield, Pos);
			break;
		case Automata::EventType::Move:
			PlaySFX(AWVisualAssets::SFX_Move, Pos);
			break;
		default:
			break;
		}

		// Check for destruction (HP dropped to 0)
		if (Evt.type == Automata::EventType::Hit)
		{
			int32 TargetRobot = 1 - Evt.robot; // Hit event is on the shooter; target is the other
			if (CurrentSnapshot.robots[TargetRobot].hp <= 0)
			{
				FVector DeathPos = GridToWorld(CurrentSnapshot.robots[TargetRobot].x, CurrentSnapshot.robots[TargetRobot].y);
				TriggerDestruction(DeathPos);
				PlaySFX(AWVisualAssets::SFX_Destroy, DeathPos);
			}
		}
	}
}

void AAWArenaRenderer::ResetVisuals()
{
	if (RobotRoot0) RobotRoot0->SetWorldLocation(FVector::ZeroVector);
	if (RobotRoot1) RobotRoot1->SetWorldLocation(FVector::ZeroVector);
	TargetPos0 = FVector::ZeroVector;
	TargetPos1 = FVector::ZeroVector;
}

void AAWArenaRenderer::BuildFloorGrid(int32 Width, int32 Height)
{
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> Colors;

	const float CellSize = AWVisualConfig::CellSize;

	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			int32 BaseIdx = Vertices.Num();
			float X0 = X * CellSize;
			float Y0 = Y * CellSize;

			Vertices.Add(FVector(X0, Y0, AWVisualConfig::FloorZ));
			Vertices.Add(FVector(X0 + CellSize, Y0, AWVisualConfig::FloorZ));
			Vertices.Add(FVector(X0 + CellSize, Y0 + CellSize, AWVisualConfig::FloorZ));
			Vertices.Add(FVector(X0, Y0 + CellSize, AWVisualConfig::FloorZ));

			Triangles.Add(BaseIdx); Triangles.Add(BaseIdx + 1); Triangles.Add(BaseIdx + 2);
			Triangles.Add(BaseIdx); Triangles.Add(BaseIdx + 2); Triangles.Add(BaseIdx + 3);

			for (int32 i = 0; i < 4; ++i) Normals.Add(FVector::UpVector);
			UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0));
			UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));

			// Checkerboard pattern: dark neutral with luminous grid lines
			bool bDark = ((X + Y) % 2 == 0);
			FColor CellColor = bDark ? FColor(8, 10, 14) : FColor(12, 15, 20);
			for (int32 i = 0; i < 4; ++i) Colors.Add(CellColor);
		}
	}

	FloorMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, TArray<FProcMeshTangent>(), false);
}

void AAWArenaRenderer::SpawnCoverVisuals(int32 Width, int32 Height, const TArray<Automata::CellType>& Grid)
{
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!CubeMesh) return;

	int32 CoverIdx = 0;
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			int32 CellIdx = Y * Width + X;
			if (CellIdx >= Grid.Num()) break;

			if (Grid[CellIdx] == Automata::CellType::Cover || Grid[CellIdx] == Automata::CellType::Wall)
			{
				FVector Pos = GridToWorld(X, Y) + FVector(0, 0, 40.f);

				// Variant selection from index for visual variety
				float ScaleZ = (CoverIdx % 3 == 0) ? 0.8f : (CoverIdx % 3 == 1) ? 1.0f : 0.6f;
				FLinearColor CoverColor;
				switch (CoverIdx % 3)
				{
				case 0: CoverColor = FLinearColor(0.15f, 0.12f, 0.08f); break;
				case 1: CoverColor = FLinearColor(0.08f, 0.12f, 0.15f); break;
				default: CoverColor = FLinearColor(0.12f, 0.08f, 0.12f); break;
				}

				UStaticMeshComponent* Block = NewObject<UStaticMeshComponent>(this);
				Block->SetupAttachment(GetRootComponent());
				Block->SetStaticMesh(CubeMesh);
				Block->SetWorldLocation(Pos);
				Block->SetWorldScale3D(FVector(0.9f, 0.9f, ScaleZ));
				Block->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Block->RegisterComponent();

				UMaterialInstanceDynamic* Mat = Block->CreateDynamicMaterialInstance(0);
				if (Mat)
				{
					Mat->SetVectorParameterValue(TEXT("BaseColor"), CoverColor);
				}

				++CoverIdx;
			}
		}
	}
}

void AAWArenaRenderer::BuildRobotVisuals()
{
	// Robot 0: tracked/angular chassis (cyan) — uses cube primitives
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh && RobotRoot0)
	{
		// Body
		UStaticMeshComponent* Body = NewObject<UStaticMeshComponent>(this);
		Body->SetupAttachment(RobotRoot0);
		Body->SetStaticMesh(CubeMesh);
		Body->SetRelativeLocation(FVector(0, 0, 25));
		Body->SetRelativeScale3D(FVector(0.8f, 1.2f, 0.4f));
		Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Body->RegisterComponent();

		RobotMat0 = Body->CreateDynamicMaterialInstance(0);
		if (RobotMat0)
		{
			RobotMat0->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.0f, 0.3f, 0.4f));
			RobotMat0->SetVectorParameterValue(TEXT("EmissiveColor"), AWUIColors::AccentCyan * 2.f);
		}

		// Turret/barrel
		UStaticMeshComponent* Turret = NewObject<UStaticMeshComponent>(this);
		Turret->SetupAttachment(RobotRoot0);
		Turret->SetStaticMesh(CubeMesh);
		Turret->SetRelativeLocation(FVector(40, 0, 35));
		Turret->SetRelativeScale3D(FVector(0.6f, 0.2f, 0.15f));
		Turret->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Turret->RegisterComponent();

		if (UMaterialInstanceDynamic* TM = Turret->CreateDynamicMaterialInstance(0))
		{
			TM->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.05f, 0.2f, 0.25f));
		}
	}

	// Robot 1: tripod/hover chassis (coral) — taller, narrower
	if (CubeMesh && RobotRoot1)
	{
		UStaticMesh* CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
		UStaticMesh* BodyMesh = CylinderMesh ? CylinderMesh : CubeMesh;

		// Body (tall narrow)
		UStaticMeshComponent* Body = NewObject<UStaticMeshComponent>(this);
		Body->SetupAttachment(RobotRoot1);
		Body->SetStaticMesh(BodyMesh);
		Body->SetRelativeLocation(FVector(0, 0, 40));
		Body->SetRelativeScale3D(FVector(0.4f, 0.4f, 0.8f));
		Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Body->RegisterComponent();

		RobotMat1 = Body->CreateDynamicMaterialInstance(0);
		if (RobotMat1)
		{
			RobotMat1->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.4f, 0.1f, 0.08f));
			RobotMat1->SetVectorParameterValue(TEXT("EmissiveColor"), AWUIColors::AccentCoral * 2.f);
		}

		// Turret/barrel
		UStaticMeshComponent* Turret = NewObject<UStaticMeshComponent>(this);
		Turret->SetupAttachment(RobotRoot1);
		Turret->SetStaticMesh(CubeMesh);
		Turret->SetRelativeLocation(FVector(35, 0, 55));
		Turret->SetRelativeScale3D(FVector(0.5f, 0.15f, 0.12f));
		Turret->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Turret->RegisterComponent();

		if (UMaterialInstanceDynamic* TM = Turret->CreateDynamicMaterialInstance(0))
		{
			TM->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.3f, 0.05f, 0.04f));
		}
	}
}

void AAWArenaRenderer::SpawnProjectileBolt(int32 OwnerIdx, FVector WorldPos, FVector Direction)
{
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!CubeMesh) return;

	UStaticMeshComponent* Bolt = NewObject<UStaticMeshComponent>(this);
	Bolt->SetupAttachment(GetRootComponent());
	Bolt->SetStaticMesh(CubeMesh);
	Bolt->SetWorldLocation(WorldPos + FVector(0, 0, AWVisualConfig::ProjectileZ));
	Bolt->SetWorldScale3D(FVector(0.3f, 0.08f, 0.08f));
	Bolt->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bolt->RegisterComponent();

	UMaterialInstanceDynamic* Mat = Bolt->CreateDynamicMaterialInstance(0);
	if (Mat)
	{
		FLinearColor Color = (OwnerIdx == 0) ? AWUIColors::AccentCyan : AWUIColors::AccentCoral;
		Mat->SetVectorParameterValue(TEXT("BaseColor"), Color);
		Mat->SetVectorParameterValue(TEXT("EmissiveColor"), Color * 5.f);
	}

	// Also add a point light for glow
	UPointLightComponent* Light = NewObject<UPointLightComponent>(Bolt);
	Light->SetupAttachment(Bolt);
	Light->SetIntensity(500.f);
	Light->SetAttenuationRadius(150.f);
	Light->SetLightColor((OwnerIdx == 0) ? FColor::Cyan : FColor(255, 90, 80));
	Light->RegisterComponent();
}

void AAWArenaRenderer::TriggerMuzzleFlash(FVector WorldPos)
{
	// Try Niagara system, fall back to point light flash
	UNiagaraSystem* NS = LoadObject<UNiagaraSystem>(nullptr, AWVisualAssets::NS_MuzzleFlash);
	if (NS)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), NS, WorldPos);
	}
	else
	{
		// Fallback: transient point light
		UPointLightComponent* Flash = NewObject<UPointLightComponent>(this);
		Flash->SetupAttachment(GetRootComponent());
		Flash->SetWorldLocation(WorldPos + FVector(0, 0, 50));
		Flash->SetIntensity(8000.f);
		Flash->SetAttenuationRadius(200.f);
		Flash->SetLightColor(FColor::Yellow);
		Flash->RegisterComponent();
		// Auto-destroy after brief duration handled by separate timer or next tick cleanup
	}
}

void AAWArenaRenderer::TriggerImpact(FVector WorldPos)
{
	UNiagaraSystem* NS = LoadObject<UNiagaraSystem>(nullptr, AWVisualAssets::NS_Impact);
	if (NS)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), NS, WorldPos);
	}
	else
	{
		UPointLightComponent* Flash = NewObject<UPointLightComponent>(this);
		Flash->SetupAttachment(GetRootComponent());
		Flash->SetWorldLocation(WorldPos);
		Flash->SetIntensity(5000.f);
		Flash->SetAttenuationRadius(150.f);
		Flash->SetLightColor(FColor::Orange);
		Flash->RegisterComponent();
	}
}

void AAWArenaRenderer::TriggerShieldBubble(int32 RobotIdx)
{
	USceneComponent* RobotRoot = (RobotIdx == 0) ? RobotRoot0.Get() : RobotRoot1.Get();
	if (!RobotRoot) return;

	FVector Pos = RobotRoot->GetComponentLocation();

	UNiagaraSystem* NS = LoadObject<UNiagaraSystem>(nullptr, AWVisualAssets::NS_Shield);
	if (NS)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), NS, Pos);
	}
	else
	{
		// Fallback: bright sphere-like glow
		UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
		if (SphereMesh)
		{
			UStaticMeshComponent* Shield = NewObject<UStaticMeshComponent>(this);
			Shield->SetupAttachment(GetRootComponent());
			Shield->SetStaticMesh(SphereMesh);
			Shield->SetWorldLocation(Pos);
			Shield->SetWorldScale3D(FVector(1.5f));
			Shield->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Shield->RegisterComponent();

			UMaterialInstanceDynamic* Mat = Shield->CreateDynamicMaterialInstance(0);
			if (Mat)
			{
				FLinearColor Color = (RobotIdx == 0) ? AWUIColors::AccentCyan : AWUIColors::AccentCoral;
				Mat->SetVectorParameterValue(TEXT("EmissiveColor"), Color * 3.f);
				Mat->SetScalarParameterValue(TEXT("Opacity"), 0.3f);
			}
		}
	}
}

void AAWArenaRenderer::TriggerDestruction(FVector WorldPos)
{
	UNiagaraSystem* NS = LoadObject<UNiagaraSystem>(nullptr, AWVisualAssets::NS_Destruction);
	if (NS)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), NS, WorldPos);
	}
	else
	{
		UPointLightComponent* Flash = NewObject<UPointLightComponent>(this);
		Flash->SetupAttachment(GetRootComponent());
		Flash->SetWorldLocation(WorldPos);
		Flash->SetIntensity(15000.f);
		Flash->SetAttenuationRadius(400.f);
		Flash->SetLightColor(FColor::Red);
		Flash->RegisterComponent();
	}
}

void AAWArenaRenderer::PlaySFX(const TCHAR* SoftPath, FVector Location)
{
	USoundBase* Sound = LoadObject<USoundBase>(nullptr, SoftPath);
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), Sound, Location);
	}
}

FVector AAWArenaRenderer::GridToWorld(int32 X, int32 Y) const
{
	return FVector(
		X * AWVisualConfig::CellSize + AWVisualConfig::CellSize * 0.5f,
		Y * AWVisualConfig::CellSize + AWVisualConfig::CellSize * 0.5f,
		AWVisualConfig::RobotZ);
}

FRotator AAWArenaRenderer::DirToRotation(Automata::Dir D) const
{
	switch (D)
	{
	case Automata::Dir::North: return FRotator(0, -90, 0);
	case Automata::Dir::East:  return FRotator(0, 0, 0);
	case Automata::Dir::South: return FRotator(0, 90, 0);
	case Automata::Dir::West:  return FRotator(0, 180, 0);
	default: return FRotator::ZeroRotator;
	}
}

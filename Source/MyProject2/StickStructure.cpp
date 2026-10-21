// Copyright Epic Games, Inc. All Rights Reserved.

#include "StickStructure.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	void Tint(UStaticMeshComponent* Part, UObject* Outer, const FLinearColor& Color)
	{
		if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
		{
			UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, Outer);
			Material->SetVectorParameterValue(TEXT("Color"), Color);
			Part->SetMaterial(0, Material);
		}
	}
}

AStickStructure::AStickStructure()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	// Basic shapes are 100 cm. The shaft stands on its foot at the actor's origin; the point tops it.
	Shaft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shaft"));
	RootComponent = Shaft;
	Shaft->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
	Shaft->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	Point = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Point"));
	Point->SetupAttachment(Shaft);
	Point->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone")));
	Point->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Flame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Flame"));
	Flame->SetupAttachment(Shaft);
	Flame->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone")));
	Flame->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Flame->SetCastShadow(false);
	Flame->SetVisibility(false);

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Shaft);
	Light->SetLightColor(FLinearColor(1.f, 0.6f, 0.25f));
	Light->SetAttenuationRadius(700.f);
	Light->SetCastShadows(false);
	Light->SetIntensity(0.f);
}

AStickStructure* AStickStructure::Plant(UWorld* World, const FVector& Ground, float Yaw)
{
	// The shaft (scaled) is the root, so its children are placed in world terms after spawning.
	const float ShaftLength = HeightCm - 20.f;
	AStickStructure* Stake = World ? World->SpawnActor<AStickStructure>(AStickStructure::StaticClass(),
		FTransform(FRotator(0.f, Yaw, 0.f), Ground + FVector(0.f, 0.f, ShaftLength * 0.5f))) : nullptr;
	if (!Stake)
	{
		return nullptr;
	}
	Stake->Shaft->SetWorldScale3D(FVector(RadiusCm / 50.f, RadiusCm / 50.f, ShaftLength / 100.f));
	Stake->Point->SetWorldScale3D(FVector(RadiusCm / 50.f, RadiusCm / 50.f, 0.2f));
	Stake->Point->SetWorldLocation(Ground + FVector(0.f, 0.f, ShaftLength + 10.f));
	Stake->Flame->SetWorldScale3D(FVector(0.12f, 0.12f, 0.3f));
	Stake->Flame->SetWorldLocation(Ground + FVector(0.f, 0.f, HeightCm + 12.f));
	Stake->Light->SetWorldLocation(Ground + FVector(0.f, 0.f, HeightCm + 25.f));
	Tint(Stake->Shaft, Stake, FLinearColor(0.3f, 0.2f, 0.1f));
	Tint(Stake->Point, Stake, FLinearColor(0.5f, 0.38f, 0.2f));
	Tint(Stake->Flame, Stake, FLinearColor(1.f, 0.45f, 0.05f));
	return Stake;
}

void AStickStructure::AddDamage(float Amount)
{
	Damage = FMath::Clamp(Damage + Amount, 0.f, 1.f);
	if (Damage >= 1.f)
	{
		Destroy();
	}
}

bool AStickStructure::ApplyHeat(float Seconds)
{
	if (bLit)
	{
		return true;
	}
	Heat += Seconds;
	if (Heat < IgniteSeconds)
	{
		return false;
	}
	bLit = true;
	BurnLeft = TorchSeconds;
	Flame->SetVisibility(true);
	Light->SetIntensity(2500.f);
	SetActorTickEnabled(true);
	return true;
}

void AStickStructure::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bLit)
	{
		return;
	}
	BurnLeft -= DeltaSeconds;
	if (BurnLeft <= 0.f)
	{
		// Burnt down to nothing.
		Destroy();
	}
}

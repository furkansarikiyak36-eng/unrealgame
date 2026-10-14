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
	constexpr int32 PartCount = 7;

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
	PrimaryActorTick.bCanEverTick = false;
	Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
	RootComponent = Pivot;
	for (int32 Index = 0; Index < PartCount; ++Index)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Part%d"), Index));
		Part->SetupAttachment(Pivot);
		Part->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Parts.Add(Part);
	}
	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Pivot);
	Light->SetLightColor(FLinearColor(1.f, 0.6f, 0.25f));
	Light->SetAttenuationRadius(700.f);
	Light->SetCastShadows(false);
	Light->SetIntensity(0.f);
}

int32 AStickStructure::SticksFor(EStickBuild Build)
{
	switch (Build)
	{
	case EStickBuild::Spikes: return 4;
	case EStickBuild::Fence:  return 6;
	default:                  return 2;
	}
}

FString AStickStructure::NameOf(EStickBuild Build)
{
	switch (Build)
	{
	case EStickBuild::Spikes: return TEXT("Çubuk diken");
	case EStickBuild::Fence:  return TEXT("Çubuk çit");
	default:                  return TEXT("Meşale");
	}
}

AStickStructure* AStickStructure::Build(UWorld* World, EStickBuild BuildKind, const FVector& Ground, float Yaw)
{
	AStickStructure* Built = World ? World->SpawnActor<AStickStructure>(AStickStructure::StaticClass(), FTransform(FRotator(0.f, Yaw, 0.f), Ground)) : nullptr;
	if (Built)
	{
		Built->Configure(BuildKind);
	}
	return Built;
}

void AStickStructure::Configure(EStickBuild NewKind)
{
	Kind = NewKind;
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	const FLinearColor Wood(0.3f, 0.2f, 0.1f);
	for (UStaticMeshComponent* Part : Parts)
	{
		Part->SetVisibility(false);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	auto Use = [&](int32 Index, UStaticMesh* Mesh, const FVector& Location, const FRotator& Rotation, const FVector& Scale, bool bSolid)
	{
		UStaticMeshComponent* Part = Parts[Index];
		Part->SetStaticMesh(Mesh);
		Part->SetRelativeLocation(Location);
		Part->SetRelativeRotation(Rotation);
		Part->SetRelativeScale3D(Scale);
		Part->SetVisibility(true);
		Part->SetCollisionEnabled(bSolid ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		Tint(Part, this, Wood);
	};
	switch (Kind)
	{
	case EStickBuild::Spikes:
		// Four sharpened sticks leaning out of the ground, 40 cm apart.
		for (int32 Index = 0; Index < 4; ++Index)
		{
			Use(Index, Cone, FVector(0.f, -60.f + 40.f * Index, 35.f), FRotator(-25.f, 0.f, 0.f), FVector(0.08f, 0.08f, 0.75f), true);
		}
		break;
	case EStickBuild::Fence:
		// Five stakes and two woven rails, two metres long, 1.2 m high.
		for (int32 Index = 0; Index < 5; ++Index)
		{
			Use(Index, Cylinder, FVector(0.f, -100.f + 50.f * Index, 60.f), FRotator::ZeroRotator, FVector(0.06f, 0.06f, 1.2f), true);
		}
		Use(5, Cube, FVector(0.f, 0.f, 45.f), FRotator::ZeroRotator, FVector(0.06f, 2.f, 0.12f), true);
		Use(6, Cube, FVector(0.f, 0.f, 95.f), FRotator::ZeroRotator, FVector(0.06f, 2.f, 0.12f), true);
		break;
	default:
		// A stake with a burning head.
		Use(0, Cylinder, FVector(0.f, 0.f, 70.f), FRotator::ZeroRotator, FVector(0.05f, 0.05f, 1.4f), true);
		Use(1, Cone, FVector(0.f, 0.f, 150.f), FRotator::ZeroRotator, FVector(0.12f, 0.12f, 0.25f), false);
		Tint(Parts[1], this, FLinearColor(1.f, 0.45f, 0.05f));
		Light->SetRelativeLocation(FVector(0.f, 0.f, 170.f));
		Light->SetIntensity(2500.f);
		break;
	}
}

void AStickStructure::AddDamage(float Amount)
{
	Damage = FMath::Clamp(Damage + Amount, 0.f, 1.f);
	if (Damage >= 1.f)
	{
		Destroy();
	}
}

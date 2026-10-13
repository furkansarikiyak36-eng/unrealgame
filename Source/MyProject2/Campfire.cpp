// Copyright Epic Games, Inc. All Rights Reserved.

#include "Campfire.h"
#include "CarvableLog.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Wood/WoodStructureGeometry.h"

namespace
{
	constexpr int32 StoneCount = 7;
	constexpr float StoneRingRadius = 48.f;
	constexpr int32 FlameCount = 3;
}

ACampfire::ACampfire()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	UStaticMesh* StoneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Greybox/Meshes/SM_GB_Stone.SM_GB_Stone"));
	for (int32 Index = 0; Index < StoneCount; ++Index)
	{
		UStaticMeshComponent* Stone = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Stone%d"), Index));
		Stone->SetupAttachment(Root);
		Stone->SetStaticMesh(StoneMesh);
		const float Angle = 2.f * PI * Index / StoneCount;
		Stone->SetRelativeLocation(FVector(FMath::Cos(Angle) * StoneRingRadius, FMath::Sin(Angle) * StoneRingRadius, 9.f));
		Stone->SetRelativeRotation(FRotator(0.f, Index * 51.f, 0.f));
		Stone->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Stones.Add(Stone);
	}

	// Flames and embers are the engine's basic shapes tinted through the basic shape material's Color.
	UStaticMesh* Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	for (int32 Index = 0; Index < FlameCount; ++Index)
	{
		UStaticMeshComponent* Flame = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Flame%d"), Index));
		Flame->SetupAttachment(Root);
		Flame->SetStaticMesh(Cone);
		const float Angle = 2.f * PI * Index / FlameCount;
		Flame->SetRelativeLocation(FVector(FMath::Cos(Angle) * 12.f, FMath::Sin(Angle) * 12.f, 10.f));
		Flame->SetRelativeScale3D(FVector(0.28f, 0.28f, 0.7f - 0.15f * Index));
		Flame->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Flame->SetCastShadow(false);
		Flames.Add(Flame);
	}
	Embers = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Embers"));
	Embers->SetupAttachment(Root);
	Embers->SetStaticMesh(Sphere);
	Embers->SetRelativeLocation(FVector(0.f, 0.f, 4.f));
	Embers->SetRelativeScale3D(FVector(0.75f, 0.75f, 0.12f));
	Embers->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Root);
	Light->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
	Light->SetLightColor(FLinearColor(1.f, 0.55f, 0.2f));
	Light->SetAttenuationRadius(900.f);
	Light->SetCastShadows(false);
	Light->SetIntensity(0.f);
}

float ACampfire::FuelSecondsOf(const ACarvableLog* Piece)
{
	// A whole log is what the shoulder carries at full load, less the slack that load allows for.
	const float WholeLog = ACarvableLog::GetShoulderCapacity() / 1.03f;
	return Piece ? Piece->GetVolume() / WholeLog * SecondsPerWholeLog : 0.f;
}

ACampfire* ACampfire::Build(UWorld* World, const FVector& Ground)
{
	ACampfire* Fire = World ? World->SpawnActor<ACampfire>(ACampfire::StaticClass(), FTransform(Ground)) : nullptr;
	if (Fire)
	{
		Fire->SetLit(false);
	}
	return Fire;
}

bool ACampfire::AddFuel(ACarvableLog* Piece)
{
	if (!Piece || Piece->IsAnchored() || FuelSeconds >= MaxFuelSeconds - 0.5f)
	{
		return false;
	}
	const bool bWasOut = !IsBurning();
	FuelSeconds = FMath::Min(FuelSeconds + FuelSecondsOf(Piece), MaxFuelSeconds);
	Piece->Destroy();
	if (bWasOut)
	{
		SetLit(true);
	}
	return true;
}

void ACampfire::SetLit(bool bLit)
{
	static const FLinearColor FlameColor(1.f, 0.45f, 0.05f);
	static const FLinearColor EmberLit(0.8f, 0.12f, 0.02f);
	static const FLinearColor EmberOut(0.04f, 0.035f, 0.03f);
	auto Tint = [this](UStaticMeshComponent* Part, const FLinearColor& Color)
	{
		if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
		{
			UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(Part->GetMaterial(0));
			if (!Material)
			{
				Material = UMaterialInstanceDynamic::Create(Base, this);
				Part->SetMaterial(0, Material);
			}
			Material->SetVectorParameterValue(TEXT("Color"), Color);
		}
	};
	for (UStaticMeshComponent* Flame : Flames)
	{
		Flame->SetVisibility(bLit);
		Tint(Flame, FlameColor);
	}
	Tint(Embers, bLit ? EmberLit : EmberOut);
	Light->SetIntensity(bLit ? 5000.f : 0.f);
}

void ACampfire::Burn(float Seconds)
{
	if (Seconds <= 0.f || !IsBurning())
	{
		return;
	}
	const float Burned = FMath::Min(Seconds, FuelSeconds);
	FuelSeconds -= Burned;

	// Wood in reach of the heat is scorched: the distance is taken to the surface of each log.
	for (TActorIterator<ACarvableLog> It(GetWorld()); It; ++It)
	{
		FVector Centre, Axis;
		float Radius, Half;
		if (It->IsCarried() || !ACarvableLog::GetLogAxis(*It, Centre, Axis, Radius, Half))
		{
			continue;
		}
		double T = 0.0;
		const double Distance = WoodStructure::Geo::DistToSegment(GetActorLocation() + FVector(0.f, 0.f, 30.f), Centre - Axis * Half, Centre + Axis * Half, T) - Radius;
		if (Distance <= HeatReachCm)
		{
			It->AddDamage(HeatDamagePerSecond * Burned);
		}
	}

	if (!IsBurning())
	{
		SetLit(false);
	}
}

void ACampfire::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!IsBurning())
	{
		return;
	}
	Burn(DeltaSeconds);
	if (!IsBurning())
	{
		return;
	}
	// A flicker: the light and the flames breathe, a small fire a little lower.
	FlickerTime += DeltaSeconds;
	const float Strength = FMath::Clamp(0.55f + FuelSeconds / 600.f * 0.45f, 0.55f, 1.f);
	const float Flicker = 0.85f + 0.15f * FMath::Sin(FlickerTime * 13.f) * FMath::Sin(FlickerTime * 7.3f + 1.f);
	Light->SetIntensity(5000.f * Strength * Flicker);
	for (int32 Index = 0; Index < Flames.Num(); ++Index)
	{
		const float Wobble = 1.f + 0.18f * FMath::Sin(FlickerTime * (9.f + Index * 4.f) + Index);
		Flames[Index]->SetRelativeScale3D(FVector(0.28f, 0.28f, (0.7f - 0.15f * Index) * Strength * Wobble));
	}
}

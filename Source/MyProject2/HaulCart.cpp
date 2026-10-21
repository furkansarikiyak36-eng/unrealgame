// Copyright Epic Games, Inc. All Rights Reserved.

#include "HaulCart.h"
#include "CarvableLog.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	constexpr int32 PartCount = 4;

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

AHaulCart::AHaulCart()
{
	PrimaryActorTick.bCanEverTick = true;
	Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
	RootComponent = Pivot;

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Bed = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bed"));
	Bed->SetupAttachment(Pivot);
	Bed->SetStaticMesh(Cube);
	Bed->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	for (int32 Index = 0; Index < PartCount; ++Index)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Part%d"), Index));
		Part->SetupAttachment(Pivot);
		Part->SetStaticMesh(Index < 2 ? Cylinder : Cube);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Parts.Add(Part);
	}
}

AHaulCart* AHaulCart::Spawn(UWorld* World, const FVector& Ground, float Yaw, EHaulKind Kind)
{
	AHaulCart* Cart = World ? World->SpawnActor<AHaulCart>(AHaulCart::StaticClass(), FTransform(FRotator(0.f, Yaw, 0.f), Ground)) : nullptr;
	if (Cart)
	{
		Cart->Configure(Kind);
	}
	return Cart;
}

void AHaulCart::SetKind(EHaulKind NewKind)
{
	if (Loaded.Num() > 0 || NewKind == Kind)
	{
		return;
	}
	Configure(NewKind);
}

FString AHaulCart::KindName(EHaulKind Kind)
{
	switch (Kind)
	{
	case EHaulKind::Cart: return TEXT("El arabası");
	case EHaulKind::Sled: return TEXT("Kızak");
	default:              return TEXT("Depolama sehpası");
	}
}

void AHaulCart::Configure(EHaulKind NewKind)
{
	Kind = NewKind;
	// Basic shapes are 100 cm. The bed is a flat board; the parts are wheels, runners or legs.
	FVector BedSize(200.f, 90.f, 12.f);
	switch (Kind)
	{
	case EHaulKind::Cart:
		BedTop = 45.f;
		BedSize = FVector(220.f, 90.f, 12.f);
		for (int32 Index = 0; Index < 2; ++Index)
		{
			Parts[Index]->SetRelativeLocation(FVector(-20.f, Index == 0 ? -55.f : 55.f, 22.f));
			Parts[Index]->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));
			Parts[Index]->SetRelativeScale3D(FVector(0.44f, 0.44f, 0.08f));
			Parts[Index]->SetVisibility(true);
		}
		for (int32 Index = 2; Index < PartCount; ++Index)
		{
			Parts[Index]->SetVisibility(false);
		}
		break;
	case EHaulKind::Sled:
		BedTop = 22.f;
		BedSize = FVector(300.f, 110.f, 10.f);
		for (int32 Index = 0; Index < 2; ++Index)
		{
			Parts[Index]->SetRelativeLocation(FVector(0.f, Index == 0 ? -45.f : 45.f, 8.f));
			Parts[Index]->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
			Parts[Index]->SetRelativeScale3D(FVector(0.16f, 0.16f, 3.2f));
			Parts[Index]->SetVisibility(true);
		}
		for (int32 Index = 2; Index < PartCount; ++Index)
		{
			Parts[Index]->SetVisibility(false);
		}
		break;
	default:
		BedTop = 62.f;
		BedSize = FVector(300.f, 120.f, 10.f);
		for (int32 Index = 0; Index < PartCount; ++Index)
		{
			Parts[Index]->SetRelativeLocation(FVector(Index % 2 == 0 ? -130.f : 130.f, Index < 2 ? -50.f : 50.f, 28.f));
			Parts[Index]->SetRelativeRotation(FRotator::ZeroRotator);
			Parts[Index]->SetRelativeScale3D(FVector(0.12f, 0.12f, 0.56f));
			Parts[Index]->SetVisibility(true);
		}
		break;
	}
	Bed->SetRelativeLocation(FVector(0.f, 0.f, BedTop - BedSize.Z * 0.5f));
	Bed->SetRelativeScale3D(BedSize / 100.f);
	Tint(Bed, this, FLinearColor(0.35f, 0.22f, 0.1f));
	for (UStaticMeshComponent* Part : Parts)
	{
		Tint(Part, this, FLinearColor(0.12f, 0.09f, 0.06f));
	}
}

float AHaulCart::GetCapacity() const
{
	const float Loads = Kind == EHaulKind::Cart ? 2.f : (Kind == EHaulKind::Sled ? 4.f : 8.f);
	return Loads * ACarvableLog::GetShoulderCapacity();
}

float AHaulCart::GetLoadVolume() const
{
	float Total = 0.f;
	for (const TWeakObjectPtr<ACarvableLog>& Piece : Loaded)
	{
		Total += Piece.IsValid() ? Piece->GetVolume() : 0.f;
	}
	return Total;
}

bool AHaulCart::CanLoad(const ACarvableLog* Piece) const
{
	return Piece && !Piece->IsAnchored() && !Loaded.Contains(Piece) && GetLoadVolume() + Piece->GetVolume() <= GetCapacity();
}

bool AHaulCart::Load(ACarvableLog* Piece)
{
	if (!CanLoad(Piece))
	{
		return false;
	}
	// Pieces lie along the bed two abreast and stack up in layers.
	const int32 Slot = Loaded.Num();
	const float Across = (Slot % 2 == 0 ? -1.f : 1.f) * (Piece->GetRadius() + 2.f);
	const float Height = BedTop + Piece->GetRadius() + (Slot / 2) * (Piece->GetRadius() * 2.f + 1.f);
	Piece->BeginCarry(Pivot, FTransform(FRotator::ZeroRotator, FVector(-Piece->GetCenterX(), Across, Height)));
	Loaded.Add(Piece);
	return true;
}

ACarvableLog* AHaulCart::Unload()
{
	while (Loaded.Num() > 0)
	{
		ACarvableLog* Piece = Loaded.Pop().Get();
		if (!Piece)
		{
			continue;
		}
		// Set down lengthwise beside the cart.
		const FVector Beside = GetActorLocation() + GetActorRightVector() * 170.f;
		Piece->PlaceAt(Piece->GetLayTransform(Beside, GetActorRotation().Yaw), false);
		return Piece;
	}
	return nullptr;
}

bool AHaulCart::Shove(const FVector& NewVelocity)
{
	if (Kind == EHaulKind::Rack)
	{
		return false;
	}
	Velocity = FVector(NewVelocity.X, NewVelocity.Y, 0.f);
	SetActorTickEnabled(true);
	return true;
}

void AHaulCart::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Velocity.SizeSquared() < 25.f)
	{
		Velocity = FVector::ZeroVector;
		return;
	}
	AddActorWorldOffset(Velocity * DeltaSeconds, true);
	// A cart rolls on, a sled skids to a stop sooner.
	Velocity *= FMath::Max(0.f, 1.f - (Kind == EHaulKind::Cart ? 1.2f : 2.5f) * DeltaSeconds);
}

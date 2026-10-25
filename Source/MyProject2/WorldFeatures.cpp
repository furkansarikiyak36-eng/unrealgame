// Copyright Epic Games, Inc. All Rights Reserved.

#include "WorldFeatures.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "WorldCore/RegionLogisticsSubsystem.h"

namespace
{
	UStaticMesh* BasicShape(const TCHAR* Name)
	{
		return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Name, Name));
	}

	void Tint(UStaticMeshComponent* Part, UObject* Outer, const FLinearColor& Color)
	{
		if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
		{
			UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, Outer);
			Material->SetVectorParameterValue(TEXT("Color"), Color);
			Part->SetMaterial(0, Material);
		}
	}

	int32 RopeCounter = 0;
}

// ---------------------------------------------------------------- river

ARiverStrip::ARiverStrip()
{
	PrimaryActorTick.bCanEverTick = false;
	Water = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Water"));
	RootComponent = Water;
	Water->SetStaticMesh(BasicShape(TEXT("Cube")));
	Water->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Water->SetCastShadow(false);
}

ARiverStrip* ARiverStrip::Spawn(UWorld* World, const FVector& Center, const FVector2D& HalfSize)
{
	ARiverStrip* River = World ? World->SpawnActor<ARiverStrip>(ARiverStrip::StaticClass(), FTransform(Center + FVector(0.f, 0.f, 3.f))) : nullptr;
	if (River)
	{
		River->Half = HalfSize;
		River->Water->SetWorldScale3D(FVector(HalfSize.X / 50.f, HalfSize.Y / 50.f, 0.06f));
		Tint(River->Water, River, FLinearColor(0.08f, 0.22f, 0.45f));
	}
	return River;
}

bool ARiverStrip::Contains(const FVector& Feet) const
{
	const FVector Local = GetActorTransform().InverseTransformPositionNoScale(Feet);
	return FMath::Abs(Local.X) <= Half.X && FMath::Abs(Local.Y) <= Half.Y && Local.Z <= 30.f && Local.Z >= -150.f;
}

bool ARiverStrip::IsInWater(const UWorld* World, const FVector& Feet)
{
	for (TActorIterator<ARiverStrip> It(World); It; ++It)
	{
		if (It->Contains(Feet))
		{
			return true;
		}
	}
	return false;
}

// ---------------------------------------------------------------- rope

ARopeLine::ARopeLine()
{
	PrimaryActorTick.bCanEverTick = false;
	Line = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Line"));
	RootComponent = Line;
	Line->SetStaticMesh(BasicShape(TEXT("Cylinder")));
	Line->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

ARopeLine* ARopeLine::Span(UWorld* World, const FVector& A, const FVector& B)
{
	const float Length = FVector::Dist(A, B);
	if (!World || Length < MinLengthCm || Length > MaxLengthCm)
	{
		return nullptr;
	}
	// The cylinder is 100 cm along Z: turn it along the rope and stretch it.
	const FVector Direction = (B - A) / Length;
	const FQuat Rotation = FRotationMatrix::MakeFromZ(Direction).ToQuat();
	ARopeLine* Rope = World->SpawnActor<ARopeLine>(ARopeLine::StaticClass(), FTransform(Rotation, (A + B) * 0.5f));
	if (!Rope)
	{
		return nullptr;
	}
	Rope->EndA = A;
	Rope->EndB = B;
	Rope->Line->SetWorldScale3D(FVector(0.04f, 0.04f, Length / 100.f));
	Tint(Rope->Line, Rope, FLinearColor(0.55f, 0.45f, 0.25f));
	Rope->LinkKey = *FString::Printf(TEXT("Rope%d"), ++RopeCounter);
	if (URegionLogisticsSubsystem* Routes = World->GetSubsystem<URegionLogisticsSubsystem>())
	{
		Routes->RegisterInfrastructureSegment(Rope->LinkKey, Rope->GetNodeKey(false), A, Rope->GetNodeKey(true), B, ELogisticsLinkType::Ropeway, 120.f, 1.f, 0.1f, true, true);
	}
	return Rope;
}

FName ARopeLine::GetNodeKey(bool bEndB) const
{
	return *FString::Printf(TEXT("%s_%s"), *LinkKey.ToString(), bEndB ? TEXT("B") : TEXT("A"));
}

bool ARopeLine::EndNear(const FVector& Where, bool& bOutEndB) const
{
	const float ToA = FVector::Dist(Where, EndA);
	const float ToB = FVector::Dist(Where, EndB);
	bOutEndB = ToB < ToA;
	return FMath::Min(ToA, ToB) <= ReachCm;
}

int32 ARopeLine::CountAll(const UWorld* World)
{
	int32 Count = 0;
	for (TActorIterator<ARopeLine> It(World); It; ++It)
	{
		++Count;
	}
	return Count;
}

void ARopeLine::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UWorld* World = GetWorld(); World && !LinkKey.IsNone())
	{
		if (URegionLogisticsSubsystem* Routes = World->GetSubsystem<URegionLogisticsSubsystem>())
		{
			Routes->UnregisterInfrastructureSegment(LinkKey);
		}
	}
	Super::EndPlay(Reason);
}

// ---------------------------------------------------------------- remnant note

ARemnantNote::ARemnantNote()
{
	PrimaryActorTick.bCanEverTick = false;
	Stake = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Stake"));
	RootComponent = Stake;
	Stake->SetStaticMesh(BasicShape(TEXT("Cylinder")));
	Stake->SetRelativeScale3D(FVector(0.06f, 0.06f, 1.1f));
	Stake->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Paper = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Paper"));
	Paper->SetupAttachment(Stake);
	Paper->SetStaticMesh(BasicShape(TEXT("Cube")));
	Paper->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
}

ARemnantNote* ARemnantNote::Place(UWorld* World, const FVector& Ground, const FString& Title, const FString& Text)
{
	ARemnantNote* Note = World ? World->SpawnActor<ARemnantNote>(ARemnantNote::StaticClass(), FTransform(Ground + FVector(0.f, 0.f, 55.f))) : nullptr;
	if (Note)
	{
		Note->Title = Title;
		Note->Text = Text;
		// The note hangs at chest height on the stake; scale is the stake's, so undo it for the sheet.
		Note->Paper->SetWorldScale3D(FVector(0.03f, 0.3f, 0.4f));
		Note->Paper->SetWorldLocation(Note->GetActorLocation() + FVector(5.f, 0.f, 45.f));
		Tint(Note->Stake, Note, FLinearColor(0.25f, 0.17f, 0.08f));
		Tint(Note->Paper, Note, FLinearColor(0.9f, 0.85f, 0.65f));
	}
	return Note;
}

// ---------------------------------------------------------------- cliff

ACliffBlock::ACliffBlock()
{
	PrimaryActorTick.bCanEverTick = false;
	Rock = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rock"));
	RootComponent = Rock;
	Rock->SetStaticMesh(BasicShape(TEXT("Cube")));
	Rock->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
}

ACliffBlock* ACliffBlock::Spawn(UWorld* World, const FVector& GroundCenter, const FVector& Size)
{
	ACliffBlock* Cliff = World ? World->SpawnActor<ACliffBlock>(ACliffBlock::StaticClass(), FTransform(GroundCenter + FVector(0.f, 0.f, Size.Z * 0.5f))) : nullptr;
	if (Cliff)
	{
		Cliff->Rock->SetWorldScale3D(Size / 100.f);
		Cliff->TopZ = GroundCenter.Z + Size.Z;
		Tint(Cliff->Rock, Cliff, FLinearColor(0.32f, 0.31f, 0.3f));
	}
	return Cliff;
}

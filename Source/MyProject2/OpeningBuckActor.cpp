// Copyright Epic Games, Inc. All Rights Reserved.

#include "OpeningBuckActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "ForestTypes.h"

AOpeningBuckActor::AOpeningBuckActor()
{
	PrimaryActorTick.bCanEverTick = false;

	FrameMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrameMeshComponent"));
	FrameMeshComponent->SetMobility(EComponentMobility::Movable);
	FrameMeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	FrameMeshComponent->SetGenerateOverlapEvents(true);
	RootComponent = FrameMeshComponent;

	OpeningType = EOpeningType::Window;
	ClearWidth = 90.0f;           // 90 cm window opening
	ClearHeight = 120.0f;         // 120 cm window opening
	bWallIsGreenWood = true;
	SettlingRatePerFoot = 0.0625f; // 3/4 inch per foot (~6.25%)
	bHasKeywaySpline = true;
	JambThickness = 5.0f;         // 5 cm 2x lumber jamb

	SelfWeight = 45.0f;           // Frame weight ~45 kg
	CompressiveCapacityKN = 40.0f;
	StructuralHealth = 120.0f;
}

void AOpeningBuckActor::BeginPlay()
{
	Super::BeginPlay();

	if (FrameMeshComponent && !FrameMeshComponent->GetStaticMesh())
	{
		// Default frame mesh from plank or beam
		if (UStaticMesh* Mesh = ForestItems::GetMesh(EForestItem::Plank))
		{
			FrameMeshComponent->SetStaticMesh(Mesh);
		}
	}
}

void AOpeningBuckActor::InitializeBuck(EOpeningType InType, float WidthCm, float HeightCm, bool bIsGreenLogWall)
{
	OpeningType = InType;
	ClearWidth = WidthCm;
	ClearHeight = HeightCm;
	bWallIsGreenWood = bIsGreenLogWall;

	// Scale mesh to fit opening dimensions
	const FVector FrameScale(ClearWidth / 100.0f, 0.4f, ClearHeight / 100.0f);
	FrameMeshComponent->SetWorldScale3D(FrameScale);
}

float AOpeningBuckActor::CalculateSettlingAllowance() const
{
	// Alaska Log Building Guide Section 8:
	// Green logs settle 3/4 inch per foot of wall height (6.25%).
	// Dry/seasoned logs settle 1/4 inch per foot.
	const float WallHeightFeet = ClearHeight / 30.48f;
	const float RateInches = bWallIsGreenWood ? 0.75f : 0.25f;
	const float AllowanceInches = WallHeightFeet * RateInches;

	// Convert inches to cm (1 inch = 2.54 cm)
	return AllowanceInches * 2.54f;
}

float AOpeningBuckActor::GetLogRoughOpeningHeight() const
{
	// LRO Height = CRO + 2 * Jamb Thickness + Settling Clearance above header
	return ClearHeight + (JambThickness * 2.0f) + CalculateSettlingAllowance();
}

float AOpeningBuckActor::GetLogRoughOpeningWidth() const
{
	// LRO Width = CRO + 2 * Jamb Thickness + clearance for spline keyways (~2.5 cm)
	return ClearWidth + (JambThickness * 2.0f) + 2.5f;
}

void AOpeningBuckActor::TriggerChaosCollapse(const FVector& Impulse)
{
	if (bIsCollapsed)
	{
		return;
	}

	Super::TriggerChaosCollapse(Impulse);

	if (FrameMeshComponent)
	{
		FrameMeshComponent->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
		FrameMeshComponent->SetSimulatePhysics(true);
		FrameMeshComponent->SetEnableGravity(true);

		if (!Impulse.IsNearlyZero())
		{
			FrameMeshComponent->AddImpulse(Impulse, NAME_None, true);
		}
	}
}

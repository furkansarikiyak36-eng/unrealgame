// Copyright Epic Games, Inc. All Rights Reserved.

#include "LogConstructionActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"

ALogConstructionActor::ALogConstructionActor()
{
	PrimaryActorTick.bCanEverTick = false;

	LogMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LogMeshComponent"));
	LogMeshComponent->SetMobility(EComponentMobility::Movable);
	LogMeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	LogMeshComponent->SetGenerateOverlapEvents(true);
	RootComponent = LogMeshComponent;

	ItemType = EForestItem::Log;
	MoistureState = ELogMoistureState::Green;
	MoistureContent = 0.38f; // 38% green moisture
	LogDiameter = 30.0f;    // 30 cm (~12 inches)
	LogLength = 390.0f;     // ~3.9 meters
	UnderscribeQuality = EUnderscribeQuality::Ideal;
	bIsChinked = false;
	bIsPlastered = false;
	MaxStructuralHealth = 100.0f;
	StructuralHealth = 100.0f;
	SupportedNormalLoad = 500.0f;
	bIsCollapsed = false;

	// Inherited structural integrity properties
	SelfWeight = 160.0f;            // 160 kg per log
	CompressiveCapacityKN = 120.0f; // 120 kN vertical load capacity
	bIsGrounded = false;
}

void ALogConstructionActor::BeginPlay()
{
	Super::BeginPlay();

	if (LogMeshComponent && !LogMeshComponent->GetStaticMesh())
	{
		InitializeLog(ItemType, MoistureState);
	}
}

void ALogConstructionActor::InitializeLog(EForestItem InItemType, ELogMoistureState InMoistureState)
{
	ItemType = InItemType;
	MoistureState = InMoistureState;

	if (MoistureState == ELogMoistureState::Green)
	{
		MoistureContent = 0.38f;
	}
	else if (MoistureState == ELogMoistureState::Seasoned)
	{
		MoistureContent = 0.15f;
	}
	else
	{
		MoistureContent = 0.09f;
	}

	if (UStaticMesh* Mesh = ForestItems::GetMesh(ItemType))
	{
		LogMeshComponent->SetStaticMesh(Mesh);
		LogMeshComponent->SetWorldScale3D(ForestItems::GetItemScale(ItemType));
	}
}

float ALogConstructionActor::ApplyStructuralDamage(float DamageAmount, const FVector& HitLocation, const FVector& HitDirection)
{
	if (bIsCollapsed)
	{
		return 0.0f;
	}

	StructuralHealth = FMath::Clamp(StructuralHealth - DamageAmount, 0.0f, MaxStructuralHealth);

	if (StructuralHealth <= 0.0f)
	{
		const FVector Impulse = HitDirection.GetSafeNormal() * 450.0f + FVector(0.0f, 0.0f, 150.0f);
		TriggerChaosCollapse(Impulse);
	}

	return StructuralHealth;
}

void ALogConstructionActor::TriggerChaosCollapse(const FVector& Impulse)
{
	if (bIsCollapsed)
	{
		return;
	}

	// Trigger cascade failure on upper logs in the StructuralIntegritySubsystem
	Super::TriggerChaosCollapse(Impulse);

	if (LogMeshComponent)
	{
		LogMeshComponent->SetMobility(EComponentMobility::Movable);
		LogMeshComponent->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
		LogMeshComponent->SetSimulatePhysics(true);
		LogMeshComponent->SetEnableGravity(true);

		if (!Impulse.IsNearlyZero())
		{
			LogMeshComponent->AddImpulse(Impulse, NAME_None, true);
		}

		// Chaos log rolling and tumbling
		const FVector AngularImpulse(
			FMath::RandRange(-60.0f, 60.0f),
			FMath::RandRange(-60.0f, 60.0f),
			FMath::RandRange(-60.0f, 60.0f)
		);
		LogMeshComponent->AddAngularImpulseInDegrees(AngularImpulse, NAME_None, true);
	}
}

void ALogConstructionActor::ApplyChinking()
{
	bIsChinked = true;
	CompressiveCapacityKN += 20.0f;
	StructuralHealth = FMath::Clamp(StructuralHealth + 40.0f, 0.0f, MaxStructuralHealth + 50.0f);
	UpdateVisualColor();
}

void ALogConstructionActor::ApplyInteriorPlaster()
{
	bIsPlastered = true;
	StructuralHealth = FMath::Clamp(StructuralHealth + 30.0f, 0.0f, MaxStructuralHealth + 50.0f);
	UpdateVisualColor();
}

void ALogConstructionActor::UpdateVisualColor()
{
	if (!LogMeshComponent)
	{
		return;
	}

	for (int32 MatIdx = 0; MatIdx < LogMeshComponent->GetNumMaterials(); ++MatIdx)
	{
		UMaterialInstanceDynamic* DynMat = Cast<UMaterialInstanceDynamic>(LogMeshComponent->GetMaterial(MatIdx));
		if (!DynMat)
		{
			DynMat = LogMeshComponent->CreateAndSetMaterialInstanceDynamic(MatIdx);
		}
		if (DynMat)
		{
			// Değer arttıkça renk değişimi:
			// 1. Ham kütük: Doğal standart ahşap
			// 2. Chinking (+R-değeri, yalıtımlı): Açık kum/harç tonu
			// 3. Plaster (en yüksek R-değeri ve yangın direnci): Beyaz alçı sıvası!
			FLinearColor LogColor = FLinearColor(1.0f, 1.0f, 1.0f);
			if (bIsPlastered)
			{
				LogColor = FLinearColor(0.95f, 0.95f, 0.92f); // Beyaz alçı sıva
			}
			else if (bIsChinked)
			{
				LogColor = FLinearColor(0.82f, 0.74f, 0.62f); // Açık harç dolgulu kütük tonu
			}

			DynMat->SetVectorParameterValue(TEXT("BaseColorTint"), LogColor);
			DynMat->SetVectorParameterValue(TEXT("Color"), LogColor);
			DynMat->SetVectorParameterValue(TEXT("Tint"), LogColor);
		}
	}
}

float ALogConstructionActor::GetRequiredSettlingSpace() const
{
	// Alaska Log Building Guide Chapter 8:
	// Green logs settle 3/4 inch per vertical foot of log wall (~6%).
	// Dry logs settle 1/4 inch per foot (~2%).
	const float WallHeightFeet = (LogDiameter * 4.0f) / 30.48f;
	const float RateInches = (MoistureState == ELogMoistureState::Green) ? 0.75f : 0.25f;
	return WallHeightFeet * RateInches * 2.54f; // cm
}

float ALogConstructionActor::GetEffectiveRValue() const
{
	// BEES Standard: ~1.0 to 1.25 per inch of softwood log thickness.
	const float DiameterInches = LogDiameter / 2.54f;
	float BaseR = DiameterInches * 1.15f;

	// Without chinking, air infiltration drops R-value drastically by ~40%
	if (!bIsChinked)
	{
		BaseR *= 0.60f;
	}

	// Interior plaster adds thermal mass and air sealing (+1.5 R)
	if (bIsPlastered)
	{
		BaseR += 1.5f;
	}

	return BaseR;
}

float ALogConstructionActor::CalculateShearCapacity() const
{
	// Kessler & Hahney LBN 32 formula:
	// V = (0.2 * N) + ShearInterlock
	float ShearInterlock = 12.0f; // kN base lateral notch interlock

	if (UnderscribeQuality == EUnderscribeQuality::Ideal)
	{
		ShearInterlock *= 1.35f;
	}
	else if (UnderscribeQuality == EUnderscribeQuality::UnderScribed_TooLoose)
	{
		ShearInterlock *= 0.65f;
	}

	if (bIsChinked)
	{
		ShearInterlock += 3.5f; // Chinking adds continuous lateral bed friction
	}

	return (0.2f * (SupportedNormalLoad / 1000.0f)) + ShearInterlock;
}

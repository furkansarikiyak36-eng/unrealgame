// Copyright Epic Games, Inc. All Rights Reserved.

#include "StoneMasonryActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"

AStoneMasonryActor::AStoneMasonryActor()
{
	PrimaryActorTick.bCanEverTick = false;

	StoneMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StoneMeshComponent"));
	StoneMeshComponent->SetMobility(EComponentMobility::Movable);
	StoneMeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	StoneMeshComponent->SetGenerateOverlapEvents(true);
	RootComponent = StoneMeshComponent;

	MasonryType = EMasonryType::DryStone;
	bHasThroughStones = true;
	bHasHearting = true;
	BatterRatio = 0.125f; // 1:8 slope
	bHasCrossingJoints = true;
	CompressiveStrengthMPa = 25.0f; // High natural stone compression resistance
	StructuralHealth = 200.0f;
	bIsCollapsed = false;

	// Inherited structural integrity properties
	SelfWeight = 250.0f;            // 250 kg stone block
	CompressiveCapacityKN = 2500.0f; // Extremely high compression load capacity
	bIsGrounded = true;             // Foundation stones start grounded
}

void AStoneMasonryActor::BeginPlay()
{
	Super::BeginPlay();

	if (StoneMeshComponent && !StoneMeshComponent->GetStaticMesh())
	{
		InitializeStone(MasonryType, bHasThroughStones, bHasHearting);
	}
}

void AStoneMasonryActor::InitializeStone(EMasonryType InType, bool bWithThroughStones, bool bWithHearting)
{
	MasonryType = InType;
	bHasThroughStones = bWithThroughStones;
	bHasHearting = bWithHearting;

	if (UStaticMesh* Mesh = ForestItems::GetMesh(EForestItem::StoneBlock))
	{
		StoneMeshComponent->SetStaticMesh(Mesh);
		StoneMeshComponent->SetWorldScale3D(ForestItems::GetItemScale(EForestItem::StoneBlock));
	}
}

UStaticMesh* AStoneMasonryActor::GetStoneMeshForSave() const
{
	return StoneMeshComponent ? StoneMeshComponent->GetStaticMesh() : nullptr;
}

bool AStoneMasonryActor::RestorePersistentState(UStaticMesh* SavedMesh, EMasonryType SavedType,
	bool bSavedThroughStones, bool bSavedHearting, float SavedBatterRatio, bool bSavedCrossingJoints,
	float SavedCompressiveStrengthMPa, float SavedStructuralHealth, float SavedSelfWeight,
	float SavedCompressiveCapacityKN, bool bSavedGrounded, FString& OutError)
{
	OutError.Reset();
	if (!SavedMesh
		|| (SavedType != EMasonryType::DryStone && SavedType != EMasonryType::MortaredStone)
		|| !FMath::IsFinite(SavedBatterRatio) || SavedBatterRatio < 0.0f || SavedBatterRatio > 1.0f
		|| !FMath::IsFinite(SavedCompressiveStrengthMPa) || SavedCompressiveStrengthMPa <= 0.0f
		|| !FMath::IsFinite(SavedStructuralHealth) || SavedStructuralHealth <= 0.0f || SavedStructuralHealth > 300.0f
		|| !FMath::IsFinite(SavedSelfWeight) || SavedSelfWeight <= 0.0f
		|| !FMath::IsFinite(SavedCompressiveCapacityKN) || SavedCompressiveCapacityKN <= 0.0f)
	{
		OutError = TEXT("Stone masonry persistence record contains an invalid mesh, masonry type or structural value.");
		return false;
	}

	if (!StoneMeshComponent)
	{
		OutError = TEXT("Stone masonry actor has no mesh component.");
		return false;
	}

	MasonryType = SavedType;
	bHasThroughStones = bSavedThroughStones;
	bHasHearting = bSavedHearting;
	BatterRatio = SavedBatterRatio;
	bHasCrossingJoints = bSavedCrossingJoints;
	CompressiveStrengthMPa = SavedCompressiveStrengthMPa;
	StructuralHealth = SavedStructuralHealth;
	SelfWeight = SavedSelfWeight;
	CompressiveCapacityKN = SavedCompressiveCapacityKN;
	bIsGrounded = bSavedGrounded;
	bIsCollapsed = false;

	StoneMeshComponent->SetStaticMesh(SavedMesh);
	StoneMeshComponent->SetSimulatePhysics(false);
	StoneMeshComponent->SetEnableGravity(false);
	StoneMeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	UpdateVisualColor();
	return true;
}

void AStoneMasonryActor::ApplyMortar()
{
	MasonryType = EMasonryType::MortaredStone;
	CompressiveStrengthMPa += 10.0f;
	StructuralHealth += 50.0f;
	CompressiveCapacityKN += 1000.0f;
	UpdateVisualColor();
}

void AStoneMasonryActor::UpdateVisualColor()
{
	if (!StoneMeshComponent)
	{
		return;
	}

	for (int32 MatIdx = 0; MatIdx < StoneMeshComponent->GetNumMaterials(); ++MatIdx)
	{
		UMaterialInstanceDynamic* DynMat = Cast<UMaterialInstanceDynamic>(StoneMeshComponent->GetMaterial(MatIdx));
		if (!DynMat)
		{
			DynMat = StoneMeshComponent->CreateAndSetMaterialInstanceDynamic(MatIdx);
		}
		if (DynMat)
		{
			// Değer arttığında (harç uygulanıp basma kapasitesi 2500'den 3500 kN'a çıktığında):
			// Koyu doğal taştan açık derzli ve sağlam harç dolgulu taş tonuna geçer
			const FLinearColor StoneColor = (MasonryType == EMasonryType::MortaredStone)
				? FLinearColor(0.88f, 0.85f, 0.80f)
				: FLinearColor(0.65f, 0.65f, 0.65f);

			DynMat->SetVectorParameterValue(TEXT("BaseColorTint"), StoneColor);
			DynMat->SetVectorParameterValue(TEXT("Color"), StoneColor);
			DynMat->SetVectorParameterValue(TEXT("Tint"), StoneColor);
		}
	}
}

float AStoneMasonryActor::ApplyDamage(float DamageAmount, const FVector& HitDirection)
{
	if (bIsCollapsed)
	{
		return 0.0f;
	}

	StructuralHealth = FMath::Clamp(StructuralHealth - DamageAmount, 0.0f, 300.0f);

	if (StructuralHealth <= 0.0f)
	{
		const FVector Impulse = HitDirection.GetSafeNormal() * 300.0f + FVector(0.0f, 0.0f, 80.0f);
		TriggerChaosCollapse(Impulse);
	}

	return StructuralHealth;
}

void AStoneMasonryActor::TriggerChaosCollapse(const FVector& Impulse)
{
	if (bIsCollapsed)
	{
		return;
	}

	// Notify StructuralIntegritySubsystem for cascading upper structure failure
	Super::TriggerChaosCollapse(Impulse);

	if (StoneMeshComponent)
	{
		StoneMeshComponent->SetMobility(EComponentMobility::Movable);
		StoneMeshComponent->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
		StoneMeshComponent->SetSimulatePhysics(true);
		StoneMeshComponent->SetEnableGravity(true);

		if (!Impulse.IsNearlyZero())
		{
			StoneMeshComponent->AddImpulse(Impulse, NAME_None, true);
		}

		// Heavy stone tumbling
		const FVector AngularImpulse(
			FMath::RandRange(-40.0f, 40.0f),
			FMath::RandRange(-40.0f, 40.0f),
			FMath::RandRange(-40.0f, 40.0f)
		);
		StoneMeshComponent->AddAngularImpulseInDegrees(AngularImpulse, NAME_None, true);
	}
}

bool AStoneMasonryActor::ValidateDryStoneRules() const
{
	// Stonework 8 Principles: Must have Throughstones, Hearting, Batter slope and Crossing joints!
	if (MasonryType == EMasonryType::DryStone)
	{
		return bHasThroughStones && bHasHearting && (BatterRatio >= 0.10f) && bHasCrossingJoints;
	}
	return true;
}

float AStoneMasonryActor::GetCompressiveCapacity() const
{
	float CapacityKN = CompressiveStrengthMPa * 100.0f; // Scale to kN load capacity

	if (MasonryType == EMasonryType::DryStone)
	{
		if (!bHasHearting)
		{
			// Unhearted dry stone wall collapses inward under load
			CapacityKN *= 0.40f;
		}
		if (!bHasThroughStones)
		{
			// Missing throughstones allows face separation
			CapacityKN *= 0.50f;
		}
	}

	return CapacityKN;
}

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BuildingNodeActor.h"
#include "ForestTypes.h"
#include "StoneMasonryActor.generated.h"

class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EMasonryType : uint8
{
	DryStone UMETA(DisplayName = "Kuru Taş Duvar (Dry Stone Walling)"),
	MortaredStone UMETA(DisplayName = "Harçlı Taş Duvar (Mortared Masonry)")
};

/**
 * A stone foundation or wall actor implementing the 8 Dry Stone Walling principles
 * from StoneworkPDF24: Grading, Throughstones, Hearting, Batter, Coping, and Crossing Joints.
 * Serves as the rot-proof, high-compression base for log sill logs.
 */
UCLASS()
class MYPROJECT2_API AStoneMasonryActor : public ABuildingNodeActor
{
	GENERATED_BODY()

public:
	AStoneMasonryActor();

	/** Initializes stone foundation with specific bond and geometry */
	void InitializeStone(EMasonryType InType, bool bWithThroughStones = true, bool bWithHearting = true);

	/** Adds mortar to a dry stone wall, converting it to mortared masonry */
	UFUNCTION(BlueprintCallable, Category = "Stone Masonry|Crafting")
	void ApplyMortar();

	/** Updates material tint color as value increases (DryStone -> MortaredStone) */
	void UpdateVisualColor();

	/** Deals damage to foundation stone. If damaged enough, foundation crumbles and drops upper structure */
	virtual float ApplyDamage(float DamageAmount, const FVector& HitDirection) override;

	/** Triggers Chaos physics collapse of this stone block */
	virtual void TriggerChaosCollapse(const FVector& Impulse = FVector::ZeroVector) override;

	/** Checks if the wall satisfies the 8 Dry Stone principles */
	UFUNCTION(BlueprintPure, Category = "Stone Masonry|Engineering")
	bool ValidateDryStoneRules() const;

	/** Maximum vertical compressive load this foundation can carry (in kN) */
	UFUNCTION(BlueprintPure, Category = "Stone Masonry|Engineering")
	float GetCompressiveCapacity() const;

	// --- Getters ---
	EMasonryType GetMasonryType() const { return MasonryType; }
	bool HasThroughStones() const { return bHasThroughStones; }
	bool HasHearting() const { return bHasHearting; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> StoneMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Masonry Data")
	EMasonryType MasonryType;

	/** Throughstone ties passing completely through wall thickness */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Masonry Rules")
	bool bHasThroughStones;

	/** Interior core packed with small stone hearting (Stonework Principle 4) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Masonry Rules")
	bool bHasHearting;

	/** Batter slope ratio (Stonework Principle 7: 1:8 default, ~0.125) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Masonry Rules")
	float BatterRatio;

	/** Proper vertical joint staggering (Stonework Principle 6) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Masonry Rules")
	bool bHasCrossingJoints;

	/** Maximum compressive strength before stone fractures (MPa) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engineering")
	float CompressiveStrengthMPa;
};

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BuildingNodeActor.h"
#include "OpeningBuckActor.generated.h"

class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EOpeningType : uint8
{
	Window UMETA(DisplayName = "Pencere Açıklığı (Window Buck)"),
	Door UMETA(DisplayName = "Kapı Açıklığı (Door Buck)")
};

/**
 * Window and door buck framework based on Alaska Log Building Guide (Section 8).
 * Manages Log Rough Opening (LRO) vs Commercial Rough Opening (CRO),
 * 6% (3/4" per foot) vertical settling space allowance, keyway spline slip joints,
 * and 88-degree settling nailer plates.
 */
UCLASS()
class MYPROJECT2_API AOpeningBuckActor : public ABuildingNodeActor
{
	GENERATED_BODY()

public:
	AOpeningBuckActor();

	/** Initializes buck dimensions and opening type */
	void InitializeBuck(EOpeningType InType, float WidthCm, float HeightCm, bool bIsGreenLogWall = true);

	/** Calculates required vertical settling clearance space above the header (in cm) */
	UFUNCTION(BlueprintPure, Category = "Opening Buck|Engineering")
	float CalculateSettlingAllowance() const;

	/** Total LRO height required including settling head clearance */
	UFUNCTION(BlueprintPure, Category = "Opening Buck|Engineering")
	float GetLogRoughOpeningHeight() const;

	/** Width required for log opening including keyway splines */
	UFUNCTION(BlueprintPure, Category = "Opening Buck|Engineering")
	float GetLogRoughOpeningWidth() const;

	virtual void TriggerChaosCollapse(const FVector& Impulse = FVector::ZeroVector) override;

	// --- Getters ---
	EOpeningType GetOpeningType() const { return OpeningType; }
	float GetClearOpeningWidth() const { return ClearWidth; }
	float GetClearOpeningHeight() const { return ClearHeight; }
	bool HasKeywaySpline() const { return bHasKeywaySpline; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> FrameMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buck Data")
	EOpeningType OpeningType;

	/** Commercial Rough Opening (CRO) width in cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buck Dimensions")
	float ClearWidth;

	/** Commercial Rough Opening (CRO) height in cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buck Dimensions")
	float ClearHeight;

	/** True if wall logs are green wood subject to ~6% shrinkage/settling */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engineering|Settling")
	bool bWallIsGreenWood;

	/** Settling rate: default 0.0625 (3/4 inch per foot of wall height) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engineering|Settling")
	float SettlingRatePerFoot;

	/** Keyway vertical spline inserted in log ends to prevent wall buckling while permitting settling */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engineering|Keyway")
	bool bHasKeywaySpline;

	/** Thickness of the buck frame jambs (standard: 5 cm / 2 inch nominal lumber) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buck Dimensions")
	float JambThickness;
};

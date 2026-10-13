// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BuildingNodeActor.h"
#include "ForestTypes.h"
#include "LogConstructionActor.generated.h"

class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ELogMoistureState : uint8
{
	Green UMETA(DisplayName = "Yeşil / Yaş (Green)"),
	Seasoned UMETA(DisplayName = "Kurutulmuş (Seasoned)"),
	AirDry UMETA(DisplayName = "Hava Kurusu (Air Dry)")
};

UENUM(BlueprintType)
enum class EUnderscribeQuality : uint8
{
	UnderScribed_TooTight UMETA(DisplayName = "Aşırı Underscribe (Ezilme Riski)"),
	Ideal UMETA(DisplayName = "İdeal Underscribe (Kapanan Oluk & Sıkı Çentik)"),
	UnderScribed_TooLoose UMETA(DisplayName = "Yetersiz Underscribe (Gevşeme Riski)")
};

/**
 * A physical, structural log piece based on the 15 technical engineering guides.
 * Supports Underscribing (Chapter 8), 6% Settling rule (Alaska Guide),
 * Kessler shear resistance, and Chaos Physics structural collapse.
 */
UCLASS()
class MYPROJECT2_API ALogConstructionActor : public ABuildingNodeActor
{
	GENERATED_BODY()

public:
	ALogConstructionActor();

	/** Initializes the log with a specific type and mesh */
	void InitializeLog(EForestItem InItemType, ELogMoistureState InMoistureState = ELogMoistureState::Green);

	/** Applies damage from axe/saw. If health reaches 0, triggers structural failure */
	float ApplyStructuralDamage(float DamageAmount, const FVector& HitLocation, const FVector& HitDirection);

	/** Triggers dynamic Chaos RigidBody physics so the log falls, rolls and collides */
	virtual void TriggerChaosCollapse(const FVector& Impulse = FVector::ZeroVector) override;

	/** Applies chinking/daubing sealant between logs */
	UFUNCTION(BlueprintCallable, Category = "Log Construction|Crafting")
	void ApplyChinking();

	/** Applies interior plaster / lime daubing */
	UFUNCTION(BlueprintCallable, Category = "Log Construction|Crafting")
	void ApplyInteriorPlaster();

	/** Updates material tint color as value increases (Unchinked -> Chinked -> Plastered) */
	void UpdateVisualColor();

	/** Calculates required settling space based on the Alaska Guide 6% rule (3/4 inch per foot) */
	UFUNCTION(BlueprintPure, Category = "Log Construction|Engineering")
	float GetRequiredSettlingSpace() const;

	/** Effective thermal R-value based on BEES standard and chinking seal */
	UFUNCTION(BlueprintPure, Category = "Log Construction|Engineering")
	float GetEffectiveRValue() const;

	/** Lateral shear capacity based on Kessler & Hahney LBN 32 formula */
	UFUNCTION(BlueprintPure, Category = "Log Construction|Engineering")
	float CalculateShearCapacity() const;

	// --- Getters & Setters ---
	EForestItem GetItemType() const { return ItemType; }
	bool IsChinked() const { return bIsChinked; }
	bool IsPlastered() const { return bIsPlastered; }
	bool IsSimulatingPhysics() const { return bIsCollapsed; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LogMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log Data")
	EForestItem ItemType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log Data")
	ELogMoistureState MoistureState;

	/** Moisture content percentage (e.g. 0.35 for green log, 0.12 for seasoned) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engineering|Moisture")
	float MoistureContent;

	/** Log diameter in centimeters (standard: 30 cm / ~12 inches) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engineering|Dimensions")
	float LogDiameter;

	/** Log length in centimeters (standard: ~390 cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engineering|Dimensions")
	float LogLength;

	/** Underscribe quality rating (Chapter 8) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engineering|Underscribe")
	EUnderscribeQuality UnderscribeQuality;

	/** Whether the gaps have been sealed with chinking/daubing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engineering|Seals")
	bool bIsChinked;

	/** Whether interior plaster has been applied */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engineering|Seals")
	bool bIsPlastered;

	/** Maximum structural health before joint failure */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engineering|Health")
	float MaxStructuralHealth;

	/** Accumulated downward vertical load supported by this log (Newtons) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Engineering|Load")
	float SupportedNormalLoad;
};

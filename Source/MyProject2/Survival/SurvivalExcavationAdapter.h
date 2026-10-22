// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SurvivalExcavationAdapter.generated.h"

class USurvivalComponent;
class UUndergroundExposureComponent;
struct FSurvivalExposureSample;

UENUM(BlueprintType)
enum class ESurvivalExcavationAction : uint8
{
	ToolSwing,
	HeavyDig,
	PlaceShoring,
	MoveShoring,
	RepairShoring,
	CarryLoad
};

/** Connects existing player stats and exposure state without owning duplicate stats. */
UCLASS(ClassGroup = (Survival), meta = (BlueprintSpawnableComponent))
class MYPROJECT2_API USurvivalExcavationAdapter : public UActorComponent
{
	GENERATED_BODY()

public:
	USurvivalExcavationAdapter();
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Survival|Excavation")
	bool CanPayAction(ESurvivalExcavationAction Action, float BaseCost, float& OutEffectiveCost, FString& OutError) const;

	UFUNCTION(BlueprintCallable, Category = "Survival|Excavation")
	bool TryPayAction(ESurvivalExcavationAction Action, float BaseCost, float& OutEffectiveCost, FString& OutError);

	UFUNCTION(BlueprintCallable, Category = "Survival|Environment")
	bool SetEnvironmentSample(const FSurvivalExposureSample& Sample, FString& OutError);

	UFUNCTION(BlueprintPure, Category = "Survival|Excavation")
	float GetActionEnvironmentMultiplier() const;

	static bool CalculateEffectiveCost(
		ESurvivalExcavationAction Action,
		float BaseCost,
		float Wetness01,
		float BodyTemperatureC,
		bool bInWater,
		float& OutEffectiveCost);

private:
	void ResolveComponents();

	UPROPERTY(Transient)
	TObjectPtr<USurvivalComponent> Survival = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UUndergroundExposureComponent> Exposure = nullptr;
};

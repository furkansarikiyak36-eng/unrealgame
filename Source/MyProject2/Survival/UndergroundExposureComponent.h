// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UndergroundExposureComponent.generated.h"

class USurvivalComponent;

USTRUCT(BlueprintType)
struct FSurvivalExposureSample
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Survival|Environment")
	float AmbientTemperatureC = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Survival|Environment", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AmbientMoisture01 = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Survival|Environment", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LightLevel01 = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Survival|Environment")
	bool bIsUnderground = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Survival|Environment")
	bool bInWater = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Survival|Environment", meta = (ClampMin = "0.0"))
	float WaterDepthCm = 0.0f;

	bool IsValidSample(FString& OutError) const;
};

/** Versioned state for the Phase 8 unified SaveGame transaction. */
USTRUCT(BlueprintType)
struct FSurvivalExposureSnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Survival|Exposure")
	int32 SchemaVersion = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Survival|Exposure")
	FSurvivalExposureSample Environment;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Survival|Exposure")
	float BodyTemperatureC = 37.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Survival|Exposure")
	float Wetness01 = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Survival|Exposure")
	float ThermalStressSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Survival|Exposure")
	float DarknessExposureSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Survival|Exposure")
	float ContinuousWaterExposureSeconds = 0.0f;
};

UCLASS(ClassGroup = (Survival), meta = (BlueprintSpawnableComponent))
class MYPROJECT2_API UUndergroundExposureComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUndergroundExposureComponent();
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Survival|Environment")
	bool SetEnvironmentSample(const FSurvivalExposureSample& Sample, FString& OutError);

	UFUNCTION(BlueprintPure, Category = "Survival|Environment")
	FSurvivalExposureSample GetEnvironmentSample() const { return CurrentEnvironment; }

	UFUNCTION(BlueprintPure, Category = "Survival|Exposure")
	float GetBodyTemperatureC() const { return BodyTemperatureC; }

	UFUNCTION(BlueprintPure, Category = "Survival|Exposure")
	float GetWetness01() const { return Wetness01; }

	UFUNCTION(BlueprintPure, Category = "Survival|Exposure")
	float GetVisibilityFactor() const;

	UFUNCTION(BlueprintPure, Category = "Survival|Exposure")
	float GetStaminaCostMultiplier() const;

	UFUNCTION(BlueprintPure, Category = "Survival|Exposure")
	float GetContinuousWaterExposureSeconds() const { return ContinuousWaterExposureSeconds; }

	UFUNCTION(BlueprintPure, Category = "Survival|Exposure")
	bool IsUnderground() const { return CurrentEnvironment.bIsUnderground; }

	UFUNCTION(BlueprintPure, Category = "Survival|Exposure")
	bool IsInWater() const { return CurrentEnvironment.bInWater || CurrentEnvironment.WaterDepthCm > 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Survival|Exposure")
	FSurvivalExposureSnapshot CaptureSnapshot() const;

	/** State-only restore: does not advance time or apply catch-up damage. */
	UFUNCTION(BlueprintCallable, Category = "Survival|Exposure")
	bool RestoreSnapshot(const FSurvivalExposureSnapshot& Snapshot, FString& OutError);

	void RecordExertionHeat(float TemperatureRiseC);

	static float CalculateStaminaCostMultiplier(float InWetness01, float InBodyTemperatureC, bool bInWater);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival|Exposure")
	float WetnessAbsorptionPerSecond = 0.04f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival|Exposure")
	float WaterSoakPerSecond = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival|Exposure")
	float DryingPerSecond = 0.005f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival|Exposure")
	float TemperatureExchangeRate = 0.00012f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival|Exposure")
	float ThermalStressGraceSeconds = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival|Exposure")
	float ThermalHealthDamagePerSecond = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival|Exposure")
	float MaxSimulationStepSeconds = 0.25f;

private:
	void ResolveSurvivalComponent();

	UPROPERTY(Transient)
	TObjectPtr<USurvivalComponent> Survival = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, SaveGame, Category = "Survival|Exposure", meta = (AllowPrivateAccess = "true"))
	FSurvivalExposureSample CurrentEnvironment;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, SaveGame, Category = "Survival|Exposure", meta = (AllowPrivateAccess = "true"))
	float BodyTemperatureC = 37.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, SaveGame, Category = "Survival|Exposure", meta = (AllowPrivateAccess = "true"))
	float Wetness01 = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, SaveGame, Category = "Survival|Exposure", meta = (AllowPrivateAccess = "true"))
	float ThermalStressSeconds = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, SaveGame, Category = "Survival|Exposure", meta = (AllowPrivateAccess = "true"))
	float DarknessExposureSeconds = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, SaveGame, Category = "Survival|Exposure", meta = (AllowPrivateAccess = "true"))
	float ContinuousWaterExposureSeconds = 0.0f;
};

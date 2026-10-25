#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SimulationTypes.h"
#include "RegionSeason/RegionSeasonTypes.h"
#include "PlantDefinition.generated.h"

class UStaticMesh;

USTRUCT(BlueprintType)
struct FGDDStageThreshold
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phenology") EGrowthStage Stage = EGrowthStage::Seedling;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phenology", meta=(ClampMin="0")) float GDDRequired = 100.f;
};

UCLASS(BlueprintType)
class MYPROJECT2_API UPlantDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity") FName PlantID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity") FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phenology", meta=(ClampMin="-10", ClampMax="30")) float BaseTemperature = 8.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phenology", meta=(ClampMin="10", ClampMax="50")) float MaxTemperature = 35.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phenology", meta=(ClampMin="5", ClampMax="40")) float OptimalTemperatureC = 22.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phenology", meta=(ClampMin="0", ClampMax="24")) float MinimumDaylightHours = 8.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phenology", meta=(ClampMin="0", ClampMax="24")) float MaximumDaylightHours = 16.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phenology", meta=(TitleProperty="Stage")) TArray<FGDDStageThreshold> StageThresholds;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Water", meta=(ClampMin="0")) float WaterUseEfficiency = 5.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Water", meta=(ClampMin="0")) float BaseWaterDemandMmPerDay = 2.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Water", meta=(ClampMin="0")) float PeakWaterDemandMmPerDay = 5.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Water", meta=(ClampMin="0")) float WaterDemandStageFactor = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Yield", meta=(ClampMin="0", ClampMax="1")) float HarvestIndex = 0.4f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Yield", meta=(ClampMin="1")) float MaxBiomass = 2000.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Yield", meta=(ClampMin="0")) float BiomassPerGDD = 0.8f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Yield", meta=(ClampMin="0", ClampMax="1")) float MinimumQualityAfterStress = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stress|Temperature", meta=(ClampMin="-20", ClampMax="10")) float FrostDamageThresholdC = -2.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stress|Temperature", meta=(ClampMin="-30", ClampMax="0")) float FrostLethalThresholdC = -8.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stress|Temperature", meta=(ClampMin="20", ClampMax="60")) float HeatStressThresholdC = 30.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stress|Temperature", meta=(ClampMin="25", ClampMax="70")) float HeatLethalThresholdC = 42.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stress", meta=(ClampMin="1")) float StressDaysBeforeFailure = 18.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stress", meta=(ClampMin="0", ClampMax="1")) float DailyDiseaseSpread = 0.035f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stress", meta=(ClampMin="0", ClampMax="1")) float DailyDiseaseRecovery = 0.015f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stress", meta=(ClampMin="1")) float WeedBiomassForMaximumPressure = 500.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soil") float OptimalPH = 6.5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soil", meta=(ClampMin="0.1")) float PHTolerance = 1.5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Germination", meta=(ClampMin="0", ClampMax="1")) float BaseGerminationRate = 0.85f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Germination") float OptimalGerminationTemp = 20.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Nutrients", meta=(ClampMin="0")) float NitrogenDemand = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Nutrients", meta=(ClampMin="0")) float PhosphorusDemand = 50.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Nutrients", meta=(ClampMin="0")) float PotassiumDemand = 80.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Nutrients", meta=(ClampMin="0", ClampMax="1")) float NutrientUptakeFractionPerCropCycle = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Environment|Season", meta=(TitleProperty="Key")) TMap<EWorldSeason, float> SeasonSuitability;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Environment|Region", meta=(TitleProperty="Key")) TMap<EWorldRegionType, float> RegionSuitability;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual") TArray<TSoftObjectPtr<UStaticMesh>> StageMeshes;

    UFUNCTION(BlueprintCallable, Category="Validation") bool ValidateDefinition(FString& OutError) const;
    UFUNCTION(BlueprintPure, Category="GDD") EGrowthStage GetStageForGDD(float GDD) const;
    UFUNCTION(BlueprintPure, Category="GDD") float GetStageProgress(float GDD) const;
    UFUNCTION(BlueprintPure, Category="GDD") float GetTotalGDDToMaturity() const;
    UFUNCTION(BlueprintPure, Category="Environment") float GetSeasonSuitability(EWorldSeason Season) const;
    UFUNCTION(BlueprintPure, Category="Environment") float GetRegionSuitability(EWorldRegionType RegionType) const;
};
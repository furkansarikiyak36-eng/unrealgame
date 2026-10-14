#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SimulationTypes.h"
#include "PlantDefinition.generated.h"
class UStaticMesh;
USTRUCT(BlueprintType) struct FGDDStageThreshold { GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadOnly) EGrowthStage Stage=EGrowthStage::Seedling;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(ClampMin="0")) float GDDRequired=100.f;
};
UCLASS(BlueprintType)
class MYPROJECT2_API UPlantDefinition : public UPrimaryDataAsset { GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Identity") FName PlantID;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Identity") FText DisplayName;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="GDD") float BaseTemperature=8.f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="GDD") float MaxTemperature=35.f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="GDD") TArray<FGDDStageThreshold> StageThresholds;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Growth") float WaterUseEfficiency=5.f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Growth") float HarvestIndex=0.4f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Growth") float MaxBiomass=2000.f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Growth") float OptimalPH=6.5f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Growth") float PHTolerance=1.5f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Germination") float BaseGerminationRate=0.85f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Germination") float OptimalGerminationTemp=20.f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Nutrients") float NitrogenDemand=100.f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Nutrients") float PhosphorusDemand=50.f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Nutrients") float PotassiumDemand=80.f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Visual") TArray<TSoftObjectPtr<UStaticMesh>> StageMeshes;
 UFUNCTION(BlueprintCallable,Category="Validation") bool ValidateDefinition(FString& OutError) const;
 UFUNCTION(BlueprintPure,Category="GDD") EGrowthStage GetStageForGDD(float GDD) const;
 UFUNCTION(BlueprintPure,Category="GDD") float GetStageProgress(float GDD) const;
 UFUNCTION(BlueprintPure,Category="GDD") float GetTotalGDDToMaturity() const;
};

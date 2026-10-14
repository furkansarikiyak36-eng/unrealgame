#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SimulationTypes.h"
#include "PlantGrowthComponent.generated.h"
class UPlantDefinition;
UCLASS(ClassGroup=(Plant),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UPlantGrowthComponent:public UActorComponent{GENERATED_BODY()
public: UPlantGrowthComponent();
 UFUNCTION(BlueprintCallable,Category="Growth") void UpdateDaily(float TempMax,float TempMin,float DeltaTime=1.f);
 UFUNCTION(BlueprintCallable,Category="Growth") void SetDefinition(UPlantDefinition* NewDef);
 UFUNCTION(BlueprintPure,Category="Growth") UPlantDefinition* GetDefinition()const{return Definition;}
 UFUNCTION(BlueprintPure,Category="Growth") FPlantState GetState()const{return PlantState;}
 UFUNCTION(BlueprintPure,Category="Growth") EGrowthStage GetCurrentStage()const{return PlantState.Stage;}
 UFUNCTION(BlueprintCallable,Category="Growth") void ResetPlant();
 UFUNCTION(BlueprintCallable,Category="Growth") float HarvestBiomass();
 UFUNCTION(BlueprintCallable,Category="Growth") void SetStress(float WaterFactor,float NutrientFactor,float WeedFactor);
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Config") TObjectPtr<UPlantDefinition> Definition=nullptr;
 UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="State") FPlantState PlantState;
};

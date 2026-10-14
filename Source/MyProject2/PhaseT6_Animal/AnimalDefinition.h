#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SimulationTypes.h"
#include "AnimalDefinition.generated.h"
UCLASS(BlueprintType) class MYPROJECT2_API UAnimalDefinition:public UPrimaryDataAsset{GENERATED_BODY()
public: UPROPERTY(EditAnywhere,BlueprintReadOnly) FName AnimalID; UPROPERTY(EditAnywhere,BlueprintReadOnly) EAnimalSpecies Species=EAnimalSpecies::Cow; UPROPERTY(EditAnywhere,BlueprintReadOnly) FText DisplayName;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Growth") float W_infinity=600.f; UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Growth") float Gompertz_b=4.f; UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Growth") float Gompertz_k=.003f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Nutrition") float k_I=.12f; UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Nutrition") float MaintenanceEnergy=.5f; UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Nutrition") float GrowthEnergy=5.f; UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Nutrition") float MilkEnergy=3.f; UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Nutrition") float PregnancyEnergy=1.5f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Reproduction") float AgeMaturityDays=365.f; UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Reproduction") float GestationDays=280.f; UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Reproduction") float BaseConceptionRate=.7f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Production") float MaxMilkPerDay=30.f; UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Production") float LactationPeakDay=60.f; UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Production") float LactationLength=305.f; UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Production") float MaxEggsPerDay=1.f; UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Production") float MaxWoolPerDay=.02f;
 UFUNCTION(BlueprintPure,Category="Growth") float GompertzWeight(float AgeDays)const; UFUNCTION(BlueprintPure,Category="Growth") float GompertzGrowthRate(float AgeDays)const;};

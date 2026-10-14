#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SimulationTypes.h"
#include "AnimalGrowthComponent.generated.h"
class UAnimalDefinition;
UCLASS(ClassGroup=(Animal),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UAnimalGrowthComponent:public UActorComponent{GENERATED_BODY()
public: UAnimalGrowthComponent(); UFUNCTION(BlueprintCallable,Category="Growth") void UpdateDaily(float DeltaDays,float NutritionFactor,float HealthFactor,float EnvironmentFactor); UFUNCTION(BlueprintCallable,Category="Growth") void InitializeAge(float StartingAgeDays); UFUNCTION(BlueprintPure,Category="Growth") float GetAge()const{return AgeDays;} UFUNCTION(BlueprintPure,Category="Growth") float GetWeight()const{return BodyWeight;} UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<UAnimalDefinition> Definition=nullptr;
protected: UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float AgeDays=0.f; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float BodyWeight=40.f;};

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SoilNutrientComponent.generated.h"
UCLASS(ClassGroup=(Soil),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API USoilNutrientComponent:public UActorComponent{GENERATED_BODY()
public: USoilNutrientComponent(); UFUNCTION(BlueprintCallable,Category="Nutrients") void UpdateNitrogen(float AddedFertilizer,float ManureNitrogen,float Uptake,float Losses); UFUNCTION(BlueprintCallable,Category="Nutrients") void UpdatePhosphorus(float Added,float Uptake,float Losses); UFUNCTION(BlueprintCallable,Category="Nutrients") void UpdatePotassium(float Added,float Uptake,float Losses); UFUNCTION(BlueprintCallable,Category="Nutrients") void ApplyDailyLeaching(); UFUNCTION(BlueprintPure,Category="Nutrients") float ComputeSuitability(float Available,float Demand)const; UPROPERTY(EditAnywhere,BlueprintReadWrite) float ManureNitrogenEfficiency=.5f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float LeachingRate=.02f;};

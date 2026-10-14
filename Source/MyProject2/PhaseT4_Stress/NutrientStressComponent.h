#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NutrientStressComponent.generated.h"
UCLASS(ClassGroup=(Farming),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UNutrientStressComponent:public UActorComponent{GENERATED_BODY()
public:
 UNutrientStressComponent();
 UFUNCTION(BlueprintCallable,Category="NutrientStressComponent") float ComputeStress(float Available,float Demand) const;
 UFUNCTION(BlueprintCallable,Category="NutrientStressComponent") float ComputeCombinedStress(float N,float P,float K,float NDem,float PDem,float KDem) const;
};
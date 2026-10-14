#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WaterStressComponent.generated.h"
UCLASS(ClassGroup=(Farming),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UWaterStressComponent:public UActorComponent{GENERATED_BODY()
public:
 UWaterStressComponent();
 UFUNCTION(BlueprintCallable,Category="WaterStressComponent") float ComputeStress(float SoilWater,float Wilting,float Critical) const;
};
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "YieldComponent.generated.h"
UCLASS(ClassGroup=(Farming),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UYieldComponent:public UActorComponent{GENERATED_BODY()
public:
 UYieldComponent();
 UFUNCTION(BlueprintCallable,Category="YieldComponent") float ComputePotentialYield(float Biomass,float HarvestIndex) const;
 UFUNCTION(BlueprintCallable,Category="YieldComponent") float ComputeGameYield(float PotentialYield,float WaterMul,float NutrientMul,float WeedMul,float DiseaseMul,bool bWaterAlreadyApplied) const;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float BaseYield=400.f;
};
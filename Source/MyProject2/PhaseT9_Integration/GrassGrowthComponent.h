#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrassGrowthComponent.generated.h"
UCLASS(ClassGroup=(Farming),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UGrassGrowthComponent:public UActorComponent{GENERATED_BODY()
public:
 UGrassGrowthComponent();
 UFUNCTION(BlueprintCallable,Category="GrassGrowthComponent") float ComputeDailyGrowth(float Temperature,float Rainfall,float SoilWater) const;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float BaseGrowthRate=15.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float OptimalTemp=20.f;
};
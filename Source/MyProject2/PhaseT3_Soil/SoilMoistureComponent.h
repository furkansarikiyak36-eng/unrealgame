#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SoilMoistureComponent.generated.h"
UCLASS(ClassGroup=(Soil),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API USoilMoistureComponent:public UActorComponent{GENERATED_BODY()
public: USoilMoistureComponent(); UFUNCTION(BlueprintPure,Category="Moisture") float ComputeWaterStress(float Water,float Wilting,float Critical)const; UFUNCTION(BlueprintPure,Category="Moisture") float ComputeActualTranspiration(float Ks,float PotentialTranspiration)const; UFUNCTION(BlueprintPure,Category="Moisture") float ComputeEvaporation(float Temperature,float Humidity)const; UPROPERTY(EditAnywhere,BlueprintReadWrite) float BaseEvaporationRate=2.f;};

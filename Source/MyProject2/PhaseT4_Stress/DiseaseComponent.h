#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DiseaseComponent.generated.h"
UCLASS(ClassGroup=(Disease),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UDiseaseComponent:public UActorComponent{GENERATED_BODY()
public: UDiseaseComponent(); UFUNCTION(BlueprintCallable,Category="Disease") void UpdatePressure(float Humidity,float Temperature,float Crowding,float DeltaTime); UFUNCTION(BlueprintCallable,Category="Disease") void ApplyTreatment(float Amount); UFUNCTION(BlueprintPure,Category="Disease") float GetPressure()const{return DiseasePressure;} UFUNCTION(BlueprintPure,Category="Disease") float ComputeYieldMultiplier()const; UPROPERTY(EditAnywhere,BlueprintReadWrite) float BaseContagionRate=.02f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float MaxPressure=1.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float MinYieldMultiplier=.3f;
protected: UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float DiseasePressure=0.f;};

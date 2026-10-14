#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StickMoistureComponent.generated.h"
UCLASS(ClassGroup=(Stick),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UStickMoistureComponent:public UActorComponent{GENERATED_BODY() public: UStickMoistureComponent(); UFUNCTION(BlueprintCallable) void ApplyRain(float Intensity,float DeltaTime); UFUNCTION(BlueprintCallable) void Dry(float DeltaTime); UFUNCTION(BlueprintCallable) void SoakInWater(); UFUNCTION(BlueprintPure) float GetMoisture()const{return Moisture;} UFUNCTION(BlueprintPure) bool IsWet()const{return Moisture>.5f;} UPROPERTY(EditAnywhere,BlueprintReadWrite) float Moisture=0; UPROPERTY(EditAnywhere,BlueprintReadWrite) float DryRate=.02f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float WetBurnPenalty=.5f; protected: virtual void TickComponent(float,ELevelTick,FActorComponentTickFunction*)override;};

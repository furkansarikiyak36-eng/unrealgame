#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FireSpreadComponent.generated.h"
UCLASS(ClassGroup=(Fire),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UFireSpreadComponent:public UActorComponent
{GENERATED_BODY() public: UFireSpreadComponent(); UFUNCTION(BlueprintCallable) void Ignite(float InitialHeat); UFUNCTION(BlueprintCallable) void Extinguish(); UFUNCTION(BlueprintPure) bool IsBurning()const{return bBurning;} UPROPERTY(EditAnywhere,BlueprintReadWrite) float SpreadRadius=200.0f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float Heat=0.0f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float HeatGainPerSecond=10.0f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float BurnRatePerSecond=1.0f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float SpreadThreshold=50.0f; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) bool bBurning=false; protected: virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction) override; float SpreadCooldown=0.0f; };

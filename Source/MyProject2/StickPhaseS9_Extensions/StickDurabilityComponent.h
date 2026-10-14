#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StickDurabilityComponent.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStickBroke);
UCLASS(ClassGroup=(Stick),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UStickDurabilityComponent:public UActorComponent{GENERATED_BODY() public: UStickDurabilityComponent(); UFUNCTION(BlueprintCallable) void ApplyStress(float Amount); UFUNCTION(BlueprintCallable) void Repair(float Amount); UFUNCTION(BlueprintPure) float GetHealth()const{return Health;} UFUNCTION(BlueprintPure) bool IsBroken()const{return Health<=0;} UPROPERTY(BlueprintAssignable) FOnStickBroke OnStickBroke; UPROPERTY(EditAnywhere,BlueprintReadWrite) float MaxHealth=30; UPROPERTY(EditAnywhere,BlueprintReadWrite) float StressMultiplier=1; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float Health=30;};

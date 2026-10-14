#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TrapMechanismComponent.generated.h"
UENUM(BlueprintType) enum class ETrapTriggerType:uint8{Pressure,TripWire,Proximity,Manual}; DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTrapTriggered,AActor*,Victim);
UCLASS(ClassGroup=(Trap),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UTrapMechanismComponent:public UActorComponent{GENERATED_BODY() public: UTrapMechanismComponent(); UFUNCTION(BlueprintCallable) void Arm(); UFUNCTION(BlueprintCallable) void Disarm(); UFUNCTION(BlueprintCallable) bool Trigger(AActor* Victim); UFUNCTION(BlueprintPure) bool IsArmed()const{return bArmed;} UPROPERTY(BlueprintAssignable) FOnTrapTriggered OnTrapTriggered; UPROPERTY(EditAnywhere,BlueprintReadWrite) ETrapTriggerType TriggerType=ETrapTriggerType::Pressure; UPROPERTY(EditAnywhere,BlueprintReadWrite) float Cooldown=5; UPROPERTY(EditAnywhere,BlueprintReadWrite) float Damage=40; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) bool bArmed=true; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float LastTriggerTime=-100;};

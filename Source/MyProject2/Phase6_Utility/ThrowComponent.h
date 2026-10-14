#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ThrowComponent.generated.h"
class ALogActor;
UCLASS(ClassGroup=(Throw),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UThrowComponent:public UActorComponent
{GENERATED_BODY() public: UThrowComponent(); UFUNCTION(BlueprintCallable) void StartCharge(); UFUNCTION(BlueprintCallable) void ReleaseCharge(); UFUNCTION(BlueprintCallable) void SetTarget(ALogActor* Log); UPROPERTY(EditAnywhere,BlueprintReadWrite) float MaxChargeTime=2.0f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float MaxForce=1500.0f; protected: float ChargeTime=0.0f; bool bCharging=false; UPROPERTY() TObjectPtr<ALogActor> Target=nullptr; virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction)override; };

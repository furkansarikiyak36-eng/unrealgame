#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ConnectionDegradationComponent.generated.h"
UCLASS(ClassGroup=(Structure),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UConnectionDegradationComponent:public UActorComponent
{GENERATED_BODY() public: UConnectionDegradationComponent(); UFUNCTION(BlueprintCallable) void DegradeConnections(float Amount); UPROPERTY(EditAnywhere,BlueprintReadWrite) float DegradationRate=0.01f; protected: virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction)override; };

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LogJackingSystem.generated.h"
class ALogActor;
UCLASS(ClassGroup=(Log),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API ULogJackingSystem:public UActorComponent
{GENERATED_BODY() public: ULogJackingSystem(); UFUNCTION(BlueprintCallable) bool LiftOneEnd(ALogActor* Log,float Height); UFUNCTION(BlueprintCallable) bool PlaceSupportUnder(ALogActor* Log); UFUNCTION(BlueprintCallable) bool IsFullySupported(ALogActor* Log)const; UPROPERTY(EditAnywhere,BlueprintReadWrite) float MaxLiftHeight=60.0f; };

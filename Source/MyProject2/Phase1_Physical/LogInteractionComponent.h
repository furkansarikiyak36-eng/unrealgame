#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LogInteractionComponent.generated.h"
class ALogActor;
UCLASS(ClassGroup=(Log),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API ULogInteractionComponent:public UActorComponent
{ GENERATED_BODY() public: ULogInteractionComponent(); UFUNCTION(BlueprintCallable) bool TryInteract(ALogActor* Target) const; UFUNCTION(BlueprintCallable) void PushLog(ALogActor* Target,float Strength); UFUNCTION(BlueprintCallable) void KickLog(ALogActor* Target,float Strength); UPROPERTY(EditAnywhere,BlueprintReadWrite) float InteractRange=250.0f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float PushStrength=300.0f; };

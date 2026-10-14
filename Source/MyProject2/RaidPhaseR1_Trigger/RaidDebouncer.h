#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RaidDebouncer.generated.h"
UCLASS(ClassGroup=(Raid),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API URaidDebouncer : public UActorComponent { GENERATED_BODY() public: URaidDebouncer(); UFUNCTION(BlueprintCallable) bool TryAcquireEventLock(FName EventID,float Cooldown); UFUNCTION(BlueprintCallable) void ReleaseEventLock(FName EventID); UPROPERTY(EditAnywhere,BlueprintReadWrite) float DefaultCooldown=60.f; protected: UPROPERTY() TMap<FName,double> LastExecutionTimes; };

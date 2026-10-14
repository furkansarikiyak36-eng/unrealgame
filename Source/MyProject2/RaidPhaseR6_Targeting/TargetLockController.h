#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TargetLockController.generated.h"
UCLASS() class MYPROJECT2_API UTargetLockController : public UObject { GENERATED_BODY() public: UPROPERTY(EditAnywhere,BlueprintReadWrite) float LockDuration=5.f; UFUNCTION(BlueprintCallable) void LockTarget(AActor* Target,float Duration); UFUNCTION(BlueprintCallable) void ReleaseLock(); UFUNCTION(BlueprintPure) bool IsLocked() const; UFUNCTION(BlueprintPure) AActor* GetLockedTarget() const; protected: TWeakObjectPtr<AActor> LockedTarget; double LockEndTime=0.0; };

#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WaveExceptionHandler.generated.h"
UCLASS() class MYPROJECT2_API UWaveExceptionHandler : public UObject { GENERATED_BODY() public: UFUNCTION(BlueprintCallable) bool HandleStuckCapacity(float StuckDuration,float Threshold) const; UFUNCTION(BlueprintCallable) float HandleNoSpawnPosition(float CurrentRadius,float MaxRadius) const; UFUNCTION(BlueprintCallable) bool HandlePlayerDeath(int32 PlayerId) const; UFUNCTION(BlueprintCallable) void HandleServerPause(); UFUNCTION(BlueprintCallable) bool HandleCorruptWaveDefinition(FName WaveID) const; };

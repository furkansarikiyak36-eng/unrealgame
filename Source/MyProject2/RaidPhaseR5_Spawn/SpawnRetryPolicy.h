#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RaidTypes.h"
#include "SpawnRetryPolicy.generated.h"
UCLASS() class MYPROJECT2_API USpawnRetryPolicy : public UObject { GENERATED_BODY() public: UPROPERTY(EditAnywhere,BlueprintReadWrite) FRetryPolicy Policy; UFUNCTION(BlueprintPure) bool ShouldRetry(int32 AttemptCount) const; UFUNCTION(BlueprintPure) float GetNextAttemptDelay(int32 AttemptCount) const; };

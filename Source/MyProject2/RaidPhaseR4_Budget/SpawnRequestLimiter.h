#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SpawnRequestLimiter.generated.h"
UCLASS() class MYPROJECT2_API USpawnRequestLimiter : public UObject { GENERATED_BODY() public: UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 TickSpawnBudget=5; UFUNCTION(BlueprintPure) int32 ComputeSpawnRequest(int32 RemainingBudget,int32 FreeCapacity) const; };

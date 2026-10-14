#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SpawnLoadBalancer.generated.h"
UCLASS() class MYPROJECT2_API USpawnLoadBalancer : public UObject { GENERATED_BODY() public: UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 MaxSpawnsPerFrame=3; UPROPERTY(EditAnywhere,BlueprintReadWrite) float TargetDuration=10.f; UFUNCTION(BlueprintPure) int32 ComputeFrameSpawnBudget(int32 TotalCount,float DeltaTime) const; void SpreadSpawnsOverTime(int32 Count,float Duration); };

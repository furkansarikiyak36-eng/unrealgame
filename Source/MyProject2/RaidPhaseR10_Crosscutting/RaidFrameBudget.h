#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RaidFrameBudget.generated.h"
UCLASS() class MYPROJECT2_API URaidFrameBudget : public UObject { GENERATED_BODY() public: UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 MaxSpawnsPerFrame=3; UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 MaxPathfindsPerFrame=2; UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 MaxTargetUpdatesPerFrame=10; void ResetFrame(); bool TryConsumeSpawn(); bool TryConsumePathfind(); bool TryConsumeTargetUpdate(); protected: int32 SpawnsThisFrame=0; int32 PathfindsThisFrame=0; int32 TargetUpdatesThisFrame=0; };

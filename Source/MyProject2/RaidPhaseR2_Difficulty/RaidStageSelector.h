#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RaidStageSelector.generated.h"
class URaidStageTableAsset;
UCLASS() class MYPROJECT2_API URaidStageSelector : public UObject { GENERATED_BODY() public: UFUNCTION(BlueprintCallable) static int32 FindNearestStage(int32 Score,const URaidStageTableAsset* Table); UFUNCTION(BlueprintCallable) static int32 BinarySearchThreshold(int32 Score,const TArray<int32>& Thresholds); };

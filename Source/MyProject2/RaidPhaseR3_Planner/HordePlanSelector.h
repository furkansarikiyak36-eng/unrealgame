#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "HordePlanSelector.generated.h"
class UHordePlanAsset;
UCLASS() class MYPROJECT2_API UHordePlanSelector : public UObject { GENERATED_BODY() public: UFUNCTION(BlueprintCallable) static UHordePlanAsset* SelectPlanForStage(int32 StageIndex,const TArray<UHordePlanAsset*>& AvailablePlans); };

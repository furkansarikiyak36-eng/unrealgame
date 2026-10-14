#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WaveBudgetCondition.generated.h"
UCLASS() class MYPROJECT2_API UWaveBudgetCondition : public UObject { GENERATED_BODY() public: UFUNCTION(BlueprintPure) bool IsBudgetExhausted(int32 PlannedTotal,int32 SpawnedTotal) const; };

#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WaveDurationCondition.generated.h"
UCLASS() class MYPROJECT2_API UWaveDurationCondition : public UObject { GENERATED_BODY() public: UFUNCTION(BlueprintPure) bool IsDurationExpired(float StartTime,float CurrentTime,float Duration) const; };

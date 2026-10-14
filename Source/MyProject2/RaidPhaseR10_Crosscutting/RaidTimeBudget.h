#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RaidTimeBudget.generated.h"
UCLASS() class MYPROJECT2_API URaidTimeBudget : public UObject { GENERATED_BODY() public: UPROPERTY(EditAnywhere,BlueprintReadWrite) float MaxTickMilliseconds=2.f; void BeginTick(); UFUNCTION(BlueprintPure) bool HasTimeRemaining() const; UFUNCTION(BlueprintPure) float GetRemainingTime() const; protected: double TickStartTime=0.0; };

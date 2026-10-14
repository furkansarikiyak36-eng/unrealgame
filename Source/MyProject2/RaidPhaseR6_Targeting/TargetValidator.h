#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TargetValidator.generated.h"
UCLASS() class MYPROJECT2_API UTargetValidator : public UObject { GENERATED_BODY() public: UFUNCTION(BlueprintPure) bool IsTargetAlive(AActor* Target) const; UFUNCTION(BlueprintPure) bool IsTargetReachable(AActor* Target) const; UFUNCTION(BlueprintPure) bool ShouldSwitchTarget(AActor* Current,AActor* Candidate) const; };

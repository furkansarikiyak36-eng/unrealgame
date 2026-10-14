#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SpawnBudgetTracker.generated.h"
UCLASS() class MYPROJECT2_API USpawnBudgetTracker : public UObject { GENERATED_BODY() public: void Initialize(int32 PlannedTotal); void OnSpawned(int32 Count=1); void OnRemoved(int32 Count=1); UFUNCTION(BlueprintPure) int32 GetPlannedTotal() const{return PlannedTotal;} UFUNCTION(BlueprintPure) int32 GetSpawnedTotal() const{return SpawnedTotal;} UFUNCTION(BlueprintPure) int32 GetRemainingBudget() const{return FMath::Max(0,PlannedTotal-SpawnedTotal);} UFUNCTION(BlueprintPure) bool IsExhausted() const{return GetRemainingBudget()<=0;} void Reset(); protected: int32 PlannedTotal=0; int32 SpawnedTotal=0; };

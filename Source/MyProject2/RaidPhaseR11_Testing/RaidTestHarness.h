#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RaidTypes.h"
#include "RaidTestHarness.generated.h"
UCLASS() class MYPROJECT2_API URaidTestHarness : public UObject { GENERATED_BODY() public: UFUNCTION(BlueprintCallable) bool TestDifficultyScoring(); UFUNCTION(BlueprintCallable) bool TestPartyScore(); UFUNCTION(BlueprintCallable) bool TestStageSelection(); UFUNCTION(BlueprintCallable) bool TestWeightedPicker(); UFUNCTION(BlueprintCallable) bool TestAStarPathfinding(); UFUNCTION(BlueprintCallable) bool TestBudgetMath(); UFUNCTION(BlueprintCallable) bool TestSpawnDecision(); UFUNCTION(BlueprintCallable) bool TestRaidPlanValidation(); UFUNCTION(BlueprintCallable) bool TestRaidStageBudgetValidation(); UFUNCTION(BlueprintCallable) bool TestRaidLifecycleTransitions(); UFUNCTION(BlueprintCallable) bool TestStructureDamageSanitization(); UFUNCTION(BlueprintCallable) bool RunAllTests(FString& OutReport); };

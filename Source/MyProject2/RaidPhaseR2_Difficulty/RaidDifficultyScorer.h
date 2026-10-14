#pragma once
#include "CoreMinimal.h"
#include "RaidTypes.h"
#include "RaidDifficultyScorer.generated.h"
UCLASS() class MYPROJECT2_API URaidDifficultyScorer : public UObject { GENERATED_BODY() public: UFUNCTION(BlueprintCallable) static int32 ComputeSingleScore(int32 Level,float SurvivalTime,float DeathPenalty,float Multiplier); UFUNCTION(BlueprintCallable) static int32 ComputePartyScore(const TArray<int32>& SortedScores,float W0,float R); UFUNCTION(BlueprintCallable) static int32 ApplyMultiplierChain(int32 BaseScore,const TArray<FRaidMultiplier>& Chain); };

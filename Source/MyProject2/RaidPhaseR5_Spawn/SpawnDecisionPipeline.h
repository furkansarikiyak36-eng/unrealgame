#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RaidTypes.h"
#include "SpawnDecisionPipeline.generated.h"
class USpawnBudgetTracker; class USpawnCapacityCalculator; class USpawnRequestLimiter; class USpawnPositionFinder; class UPopulationCounter;
UCLASS() class MYPROJECT2_API USpawnDecisionPipeline : public UObject { GENERATED_BODY() public: bool ExecuteSpawnDecision(UWorld* World,const FSpawnRequest& Request,USpawnBudgetTracker* Budget,USpawnCapacityCalculator* Capacity,USpawnRequestLimiter* Limiter,USpawnPositionFinder* Finder,UPopulationCounter* Counter,TSubclassOf<AActor> EnemyClass); };

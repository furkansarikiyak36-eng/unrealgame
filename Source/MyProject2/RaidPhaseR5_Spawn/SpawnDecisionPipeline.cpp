#include "SpawnDecisionPipeline.h"
#include "RaidPhaseR4_Budget/SpawnBudgetTracker.h"
#include "RaidPhaseR4_Budget/SpawnCapacityCalculator.h"
#include "RaidPhaseR4_Budget/SpawnRequestLimiter.h"
#include "RaidPhaseR5_Spawn/SpawnPositionFinder.h"
#include "RaidPhaseR4_Budget/PopulationCounter.h"
#include "Engine/World.h"
bool USpawnDecisionPipeline::ExecuteSpawnDecision(UWorld* W,const FSpawnRequest&R,USpawnBudgetTracker*B,USpawnCapacityCalculator*C,USpawnRequestLimiter*L,USpawnPositionFinder*F,UPopulationCounter*P,TSubclassOf<AActor> Cls){if(!W||!B||!C||!L||!F||!P||!Cls||B->IsExhausted())return false;const int32 Free=C->ComputeFreeCapacity(P->GetLiveCount());if(L->ComputeSpawnRequest(B->GetRemainingBudget(),Free)<=0)return false;const FVector Pos=F->FindValidSpawnPosition(W,R.TargetLocation);if(Pos.IsZero()||!F->IsPositionValid(W,Pos))return false;FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;AActor* A=W->SpawnActor<AActor>(Cls,Pos,FRotator::ZeroRotator,Params);if(!A)return false;B->OnSpawned();P->OnEntitySpawned(A,R.EnemyID);return true;}

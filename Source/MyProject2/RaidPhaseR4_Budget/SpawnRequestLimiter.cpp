#include "SpawnRequestLimiter.h"
int32 USpawnRequestLimiter::ComputeSpawnRequest(int32 R,int32 F) const{return FMath::Max(0,FMath::Min3(R,F,FMath::Max(0,TickSpawnBudget)));}

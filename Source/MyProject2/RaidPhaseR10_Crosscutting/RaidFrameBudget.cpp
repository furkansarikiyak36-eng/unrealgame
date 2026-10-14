#include "RaidFrameBudget.h"
void URaidFrameBudget::ResetFrame(){SpawnsThisFrame=PathfindsThisFrame=TargetUpdatesThisFrame=0;}
bool URaidFrameBudget::TryConsumeSpawn(){if(SpawnsThisFrame>=FMath::Max(0,MaxSpawnsPerFrame))return false;++SpawnsThisFrame;return true;}
bool URaidFrameBudget::TryConsumePathfind(){if(PathfindsThisFrame>=FMath::Max(0,MaxPathfindsPerFrame))return false;++PathfindsThisFrame;return true;}
bool URaidFrameBudget::TryConsumeTargetUpdate(){if(TargetUpdatesThisFrame>=FMath::Max(0,MaxTargetUpdatesPerFrame))return false;++TargetUpdatesThisFrame;return true;}

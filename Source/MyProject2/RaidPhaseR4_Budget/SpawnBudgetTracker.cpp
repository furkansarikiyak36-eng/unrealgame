#include "SpawnBudgetTracker.h"
void USpawnBudgetTracker::Initialize(int32 N){PlannedTotal=FMath::Max(0,N);SpawnedTotal=0;}
void USpawnBudgetTracker::OnSpawned(int32 N){SpawnedTotal=FMath::Clamp(SpawnedTotal+FMath::Max(0,N),0,PlannedTotal);}
void USpawnBudgetTracker::OnRemoved(int32 Count){/* Event budget counts total successful spawns; removals must not refund it. */}
void USpawnBudgetTracker::Reset(){PlannedTotal=SpawnedTotal=0;}

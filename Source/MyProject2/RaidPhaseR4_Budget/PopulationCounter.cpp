#include "PopulationCounter.h"
void UPopulationCounter::OnEntitySpawned(AActor* A,FName G){if(A)LiveEntities.Add(A,G);}
void UPopulationCounter::OnEntityDied(AActor* A){if(A)LiveEntities.Remove(A);}
void UPopulationCounter::OnEntityRemoved(AActor* A){OnEntityDied(A);}
int32 UPopulationCounter::GetLiveCount() const{int32 N=0;for(const auto&P:LiveEntities)if(P.Key.IsValid())++N;return N;}
int32 UPopulationCounter::GetLiveCountByGroup(FName G) const{int32 N=0;for(const auto&P:LiveEntities)if(P.Value==G&&P.Key.IsValid())++N;return N;}
bool UPopulationCounter::IsAlive(AActor*A) const{return A&&LiveEntities.Contains(A);}

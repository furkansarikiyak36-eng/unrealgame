#include "SpawnCapacityCalculator.h"
int32 USpawnCapacityCalculator::ComputeEffectiveCapacity() const{return FMath::Max(0,FMath::Min3(WaveCapacity,GlobalCapacity,EventCapacity));}
int32 USpawnCapacityCalculator::ComputeFreeCapacity(int32 Live) const{return FMath::Max(0,ComputeEffectiveCapacity()-FMath::Max(0,Live));}
int32 USpawnCapacityCalculator::ComputePartyCapacity() const{return FMath::Max(0,FMath::Min(FMath::Max(0,PlayerCount)*FMath::Max(0,PerPlayerCap),FMath::Max(0,GlobalCapacity)));}

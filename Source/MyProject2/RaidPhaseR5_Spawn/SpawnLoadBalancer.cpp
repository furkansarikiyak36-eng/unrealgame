#include "SpawnLoadBalancer.h"
int32 USpawnLoadBalancer::ComputeFrameSpawnBudget(int32 N,float Dt) const{if(TargetDuration<=0.f)return FMath::Min(FMath::Max(0,N),FMath::Max(0,MaxSpawnsPerFrame));return FMath::Clamp(FMath::CeilToInt((float)FMath::Max(0,N)/TargetDuration*FMath::Max(0.f,Dt)),0,FMath::Max(0,MaxSpawnsPerFrame));}
void USpawnLoadBalancer::SpreadSpawnsOverTime(int32 N,float D){TargetDuration=FMath::Max(.1f,D);}

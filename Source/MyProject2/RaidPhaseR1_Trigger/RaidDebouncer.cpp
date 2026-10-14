#include "RaidDebouncer.h"
URaidDebouncer::URaidDebouncer(){PrimaryComponentTick.bCanEverTick=false;}
bool URaidDebouncer::TryAcquireEventLock(FName ID,float Cooldown){if(ID.IsNone())return false;const double Now=GetWorld()?GetWorld()->GetTimeSeconds():FPlatformTime::Seconds();if(double* Last=LastExecutionTimes.Find(ID)){if(Now-*Last<FMath::Max(0.f,Cooldown))return false;}LastExecutionTimes.Add(ID,Now);return true;}
void URaidDebouncer::ReleaseEventLock(FName ID){LastExecutionTimes.Remove(ID);}

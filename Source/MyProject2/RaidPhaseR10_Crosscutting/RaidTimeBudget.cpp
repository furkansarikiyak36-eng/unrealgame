#include "RaidTimeBudget.h"
void URaidTimeBudget::BeginTick(){TickStartTime=FPlatformTime::Seconds();}
bool URaidTimeBudget::HasTimeRemaining()const{return GetRemainingTime()>0.f;}
float URaidTimeBudget::GetRemainingTime()const{return FMath::Max(0.f,MaxTickMilliseconds-(float)((FPlatformTime::Seconds()-TickStartTime)*1000.0));}

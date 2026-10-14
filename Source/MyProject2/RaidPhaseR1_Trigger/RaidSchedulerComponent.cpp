#include "RaidSchedulerComponent.h"
URaidSchedulerComponent::URaidSchedulerComponent(){PrimaryComponentTick.bCanEverTick=false;}
void URaidSchedulerComponent::ScheduleNextEvent(float BaseTime,float InJitterMin,float InJitterMax){const float Lo=FMath::Min(InJitterMin,InJitterMax),Hi=FMath::Max(InJitterMin,InJitterMax);NextEventTime=FMath::Max(0.f,BaseTime+FMath::FRandRange(Lo,Hi));}
void URaidSchedulerComponent::ResetSchedule(){NextEventTime=0.f;}

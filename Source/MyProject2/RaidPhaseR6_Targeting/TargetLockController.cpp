#include "TargetLockController.h"
void UTargetLockController::LockTarget(AActor*T,float D){LockedTarget=T;LockEndTime=(GetWorld()?GetWorld()->GetTimeSeconds():FPlatformTime::Seconds())+FMath::Max(0.f,D);}
void UTargetLockController::ReleaseLock(){LockedTarget.Reset();LockEndTime=0.;}
bool UTargetLockController::IsLocked()const{return LockedTarget.IsValid()&&(GetWorld()?GetWorld()->GetTimeSeconds():FPlatformTime::Seconds())<LockEndTime;}
AActor* UTargetLockController::GetLockedTarget()const{return IsLocked()?LockedTarget.Get():nullptr;}

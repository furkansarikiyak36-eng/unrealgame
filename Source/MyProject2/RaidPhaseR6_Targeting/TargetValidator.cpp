#include "TargetValidator.h"
#include "GameFramework/Actor.h"
bool UTargetValidator::IsTargetAlive(AActor*T)const{return IsValid(T)&&!T->IsActorBeingDestroyed();}
bool UTargetValidator::IsTargetReachable(AActor*T)const{return IsTargetAlive(T);}
bool UTargetValidator::ShouldSwitchTarget(AActor*C,AActor*N)const{return IsTargetAlive(N)&&(!IsTargetAlive(C)||C!=N);}

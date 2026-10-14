#include "RaidLifecycleComponent.h"
URaidLifecycleComponent::URaidLifecycleComponent(){PrimaryComponentTick.bCanEverTick=false;}
void URaidLifecycleComponent::TransitionTo(ERaidLifecycleState S){CurrentPhase=S;if(OnLifecycleChanged.IsBound())OnLifecycleChanged.Broadcast(S);}
void URaidLifecycleComponent::BeginEvent(FRaidContext& C){C.bEventActive=true;C.Lifecycle=ERaidLifecycleState::Preparing;TransitionTo(C.Lifecycle);}
void URaidLifecycleComponent::EndEvent(FRaidContext& C){C.bEventActive=false;C.Lifecycle=ERaidLifecycleState::Completed;TransitionTo(C.Lifecycle);}
void URaidLifecycleComponent::CancelEvent(FRaidContext& C,FName Reason){C.bEventActive=false;C.Lifecycle=ERaidLifecycleState::Cancelled;TransitionTo(C.Lifecycle);}
void URaidLifecycleComponent::OnPlayerDeath(FRaidContext& C,int32 ID){for(FPlayerStateSnapshot& P:C.Players)if(P.PlayerId==ID){P.bIsAlive=false;++P.DeathCount;break;}}
void URaidLifecycleComponent::OnServerShutdown(FRaidContext& C){C.bEventActive=false;C.Lifecycle=ERaidLifecycleState::Cancelled;TransitionTo(C.Lifecycle);}

void URaidLifecycleComponent::SetPhase(FRaidContext& C,ERaidLifecycleState S)
{
    if(!C.bEventActive||S==ERaidLifecycleState::Idle||S==ERaidLifecycleState::Completed||S==ERaidLifecycleState::Cancelled)return;
    if(CurrentPhase==S&&C.Lifecycle==S)return;
    C.Lifecycle=S;
    TransitionTo(S);
}

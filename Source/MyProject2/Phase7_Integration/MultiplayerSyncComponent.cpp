#include "Phase7_Integration/MultiplayerSyncComponent.h"
#include "Phase1_Physical/LogActor.h"
UMultiplayerSyncComponent::UMultiplayerSyncComponent(){PrimaryComponentTick.bCanEverTick=false;SetIsReplicatedByDefault(true);}
void UMultiplayerSyncComponent::ServerSetState_Implementation(AActor* A,ELogState S){if(!GetOwner()||!GetOwner()->HasAuthority())return;if(ALogActor* L=Cast<ALogActor>(A))if(!L->SetLogState(S))return;MulticastOnStateChanged(A,S);}
void UMultiplayerSyncComponent::MulticastOnStateChanged_Implementation(AActor* A,ELogState S){if(GetOwner()&&GetOwner()->HasAuthority())return;if(ALogActor* L=Cast<ALogActor>(A))L->SetLogState(S);}

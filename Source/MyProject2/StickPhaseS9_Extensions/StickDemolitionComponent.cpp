#include "GameFramework/Actor.h"
#include "StickDemolitionComponent.h"
#include "StickPhaseS1_Core/StickActor.h"
#include "StickCore/StickConnectionGraph.h"
UStickDemolitionComponent::UStickDemolitionComponent(){PrimaryComponentTick.bCanEverTick=false;} bool UStickDemolitionComponent::Demolish(AStickActor*S){if(!S)return false;if(auto*G=S->FindComponentByClass<UStickConnectionGraph>()){const auto Ns=G->GetConnectedNodes();for(AActor*N:Ns)G->DisconnectFrom(N);}return S->SetStructuralState_Implementation(EStructuralNodeState::Loose);} int32 UStickDemolitionComponent::DemolishBundle(const TArray<AStickActor*>&A){int32 N=0;for(auto*S:A)if(Demolish(S))++N;return N;} int32 UStickDemolitionComponent::RecoverFromStructure(AActor*A){if(!A)return 0;int32 N=0;if(auto*G=A->FindComponentByClass<UStickConnectionGraph>())N+=FMath::RoundToInt(G->GetConnectedNodes().Num()*RecoveryRate);return N;}

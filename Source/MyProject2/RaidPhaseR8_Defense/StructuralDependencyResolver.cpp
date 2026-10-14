#include "StructuralDependencyResolver.h"
#include "Core/StructuralGraph.h"
#include "Core/StructuralNode.h"
TArray<AActor*> UStructuralDependencyResolver::GetDependentBlocks(AActor*B)const{if(UStructuralGraph*G=B?B->FindComponentByClass<UStructuralGraph>():nullptr)return G->SimulateWithout(B);return {};}
int32 UStructuralDependencyResolver::PropagateCollapse(AActor*B){int32 N=0;for(AActor*D:GetDependentBlocks(B)){if(!D)continue;if(UStructuralGraph*G=D->FindComponentByClass<UStructuralGraph>())G->DisconnectFrom(B);if(D->GetClass()->ImplementsInterface(UStructuralNode::StaticClass()))IStructuralNode::Execute_SetStructuralState(D,EStructuralNodeState::Falling);else D->SetActorLocation(D->GetActorLocation()-FVector(0,0,200));++N;}return N;}

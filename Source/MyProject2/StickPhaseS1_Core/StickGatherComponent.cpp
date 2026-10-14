#include "StickGatherComponent.h"
#include "StickActor.h"
#include "SaplingActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
UStickGatherComponent::UStickGatherComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool UStickGatherComponent::GatherFromGround(AStickActor* S){if(!S||!GetOwner()||FVector::Dist(GetOwner()->GetActorLocation(),S->GetActorLocation())>GatherRadius)return false;return S->SetStructuralState_Implementation(EStructuralNodeState::Carried);}
bool UStickGatherComponent::HarvestSapling(ASaplingActor* S,float Strength){if(!S||!GetOwner()||FVector::Dist(GetOwner()->GetActorLocation(),S->GetActorLocation())>GatherRadius)return false;return S->Harvest(Strength,GetOwner())>0;}
int32 UStickGatherComponent::GatherNearbySticks(float Radius){int32 N=0;for(TActorIterator<AStickActor> It(GetWorld());It&&N<AutoGatherBatch;++It)if(FVector::Dist(GetOwner()->GetActorLocation(),It->GetActorLocation())<=Radius&&GatherFromGround(*It))++N;return N;}

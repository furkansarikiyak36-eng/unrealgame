#include "EventPropagationComponent.h"
#include "WorldEventSubsystem.h"
#include "WorldCore/ChunkNeighborGraph.h"
#include "WorldCore/WorldStateSubsystem.h"
UEventPropagationComponent::UEventPropagationComponent(){PrimaryComponentTick.bCanEverTick=false;}
void UEventPropagationComponent::PropagateToNeighbors(const FWorldEvent& E){UWorld* W=GetWorld();UWorldEventSubsystem* S=W?W->GetSubsystem<UWorldEventSubsystem>():nullptr;if(!S)return;const float Depth=E.NumericPayload.FindRef(TEXT("PropagationDepth"));if(Depth>=FMath::Max(0,MaxPropagationDepth))return;for(const FIntPoint& N:UChunkNeighborGraph::GetNeighbors4(E.ChunkCoord)){FWorldEvent Child=E;Child.ChunkCoord=N;Child.Magnitude*=FMath::Clamp(PropagationAttenuation,0.0f,1.0f);Child.SourceTag=TEXT("Propagated");Child.NumericPayload.Add(TEXT("PropagationDepth"),Depth+1);S->BroadcastEvent(Child);}}

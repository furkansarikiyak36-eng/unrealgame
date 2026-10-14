#include "StickWeavingComponent.h"
#include "StickPhaseS1_Core/StickActor.h"
#include "StickCore/StickConnectionGraph.h"
UStickWeavingComponent::UStickWeavingComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool UStickWeavingComponent::WeaveHorizontal(AStickActor*A,AStickActor*B){if(!A||!B)return false;if(auto*G=A->FindComponentByClass<UStickConnectionGraph>())return G->ConnectWoven(B,TEXT("Weave"),TEXT("Cross"));return false;}
bool UStickWeavingComponent::WeaveCross(AStickActor*A,AStickActor*B){return WeaveHorizontal(A,B);}
bool UStickWeavingComponent::CreateWattlePanel(const TArray<AStickActor*>&V,const TArray<AStickActor*>&H){if(V.IsEmpty()||H.IsEmpty())return false;for(AStickActor*A:V)for(int32 i=0;i<FMath::Min(H.Num(),RowsPerPanel);++i)WeaveHorizontal(A,H[i]);return true;}

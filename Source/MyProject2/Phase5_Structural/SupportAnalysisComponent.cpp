#include "Phase5_Structural/SupportAnalysisComponent.h"
#include "Phase1_Physical/LogActor.h"
#include "Core/LogConnectionGraph.h"
#include "Core/LogDefinition.h"
USupportAnalysisComponent::USupportAnalysisComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool USupportAnalysisComponent::CanRemoveSupport(ALogActor* S)const{return S&&GetOrphansIfRemoved(S).IsEmpty();}
TArray<ALogActor*> USupportAnalysisComponent::GetOrphansIfRemoved(ALogActor* S)const{if(!S)return {};if(auto* G=S->FindComponentByClass<ULogConnectionGraph>())return G->SimulateWithout(S);return {};}
float USupportAnalysisComponent::ComputeLoad(ALogActor* L)const{if(!L)return 0;float Sum=0;if(auto* G=L->FindComponentByClass<ULogConnectionGraph>())for(ALogActor* C:G->GetConnectedLogs())if(C&&C->GetDefinition())Sum+=C->GetDefinition()->Mass;return Sum;}

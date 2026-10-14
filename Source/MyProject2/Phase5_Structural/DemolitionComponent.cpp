#include "Phase5_Structural/DemolitionComponent.h"
#include "Phase1_Physical/LogActor.h"
#include "Core/LogConnectionGraph.h"
UDemolitionComponent::UDemolitionComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool UDemolitionComponent::PartialDemolish(ALogActor* L){if(!L)return false;if(auto* G=L->FindComponentByClass<ULogConnectionGraph>()){const TArray<ALogActor*> N=G->GetConnectedLogs();for(ALogActor* C:N)G->DisconnectFrom(C);}return L->SetLogState(ELogState::Loose);}
bool UDemolitionComponent::FullDemolish(ALogActor* L){if(!PartialDemolish(L))return false;L->Destroy();return true;}

#include "Phase3_Cutting/DovetailJointComponent.h"
#include "Phase1_Physical/LogActor.h"
#include "Core/LogConnectionGraph.h"
UDovetailJointComponent::UDovetailJointComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool UDovetailJointComponent::PrepareDovetail(ALogActor* L){return L&&L->GetDefinition()!=nullptr;}
bool UDovetailJointComponent::LockDovetail(ALogActor* A,ALogActor* B){if(!A||!B)return false;if(ULogConnectionGraph* G=A->FindComponentByClass<ULogConnectionGraph>())return G->ConnectTo(B,TEXT("Dovetail"),TEXT("Dovetail"),true);return false;}

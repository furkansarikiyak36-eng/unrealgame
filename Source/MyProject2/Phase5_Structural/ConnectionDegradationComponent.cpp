#include "Phase5_Structural/ConnectionDegradationComponent.h"
#include "Phase1_Physical/LogActor.h"
#include "Core/LogConnectionGraph.h"
UConnectionDegradationComponent::UConnectionDegradationComponent(){PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickInterval=1.0f;}
void UConnectionDegradationComponent::DegradeConnections(float A){if(A<=0)return;if(ALogActor* L=Cast<ALogActor>(GetOwner()))if(auto* G=L->FindComponentByClass<ULogConnectionGraph>())for(FLogConnection& C:G->Connections)C.ConnectionStrength=FMath::Max(0.0f,C.ConnectionStrength-A);}
void UConnectionDegradationComponent::TickComponent(float D,ELevelTick T,FActorComponentTickFunction* F){Super::TickComponent(D,T,F);DegradeConnections(FMath::Max(0.0f,DegradationRate)*D);}

#include "Phase3_Cutting/CuttingComponent.h"
#include "Phase1_Physical/LogActor.h"
#include "Core/LogDefinition.h"
UCuttingComponent::UCuttingComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool UCuttingComponent::CutCrosswise(ALogActor* L,int32 I){if(!L||!L->GetDefinition()||I<0)return false;const float N=ModuleLength*(I+1);if(N>=L->GetCurrentLength())return false;L->ApplyCut(L->GetCurrentShape(),N);return true;}
bool UCuttingComponent::SplitLengthwise(ALogActor* L){if(!L||!L->GetDefinition())return false;L->ApplyCut(EWoodShape::Half,L->GetCurrentLength());return true;}
bool UCuttingComponent::SplitQuarter(ALogActor* L){if(!L||!L->GetDefinition())return false;L->ApplyCut(EWoodShape::Quarter,L->GetCurrentLength());return true;}
bool UCuttingComponent::FlattenEnd(ALogActor* L){return L&&L->GetDefinition()!=nullptr;}
bool UCuttingComponent::TaperEnd(ALogActor* L){if(!L||!L->GetDefinition())return false;L->ApplyCut(EWoodShape::Tapered,L->GetCurrentLength());return true;}
bool UCuttingComponent::ChamferEdge(ALogActor* L){return L&&L->GetDefinition()!=nullptr;}

#include "Phase3_Cutting/WoodworkingComponent.h"
#include "Phase1_Physical/LogActor.h"
UWoodworkingComponent::UWoodworkingComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool UWoodworkingComponent::MakePlank(ALogActor* L){if(!L||!L->GetDefinition())return false;L->ApplyCut(EWoodShape::Plank,L->GetCurrentLength());return true;}
bool UWoodworkingComponent::CarveNotch(ALogActor* L,FName N){return L&&!N.IsNone()&&L->GetDefinition()!=nullptr;}
bool UWoodworkingComponent::CarveGroove(ALogActor* L){return L&&L->GetDefinition()!=nullptr;}

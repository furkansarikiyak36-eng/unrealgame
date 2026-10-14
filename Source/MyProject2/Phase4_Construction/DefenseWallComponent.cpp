#include "Phase4_Construction/DefenseWallComponent.h"
#include "Phase1_Physical/LogActor.h"
#include "Core/LogConnectionGraph.h"
UDefenseWallComponent::UDefenseWallComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool UDefenseWallComponent::SharpenTop(ALogActor* L){if(!L||!L->GetDefinition())return false;L->ApplyCut(EWoodShape::Tapered,L->GetCurrentLength());return true;}
bool UDefenseWallComponent::AddBrace(ALogActor* W,ALogActor* B){if(!W||!B)return false;if(auto* G=W->FindComponentByClass<ULogConnectionGraph>())return G->ConnectTo(B,TEXT("Brace"),TEXT("Wall"),true);return false;}

#include "RepairToolComponent.h"
#include "Phase1_Physical/LogActor.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "StickPhaseS1_Core/StickActor.h"
URepairToolComponent::URepairToolComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool URepairToolComponent::RepairStructure(AActor*S,float A){if(!S||A<=0||!GetOwner()||FVector::Dist(GetOwner()->GetActorLocation(),S->GetActorLocation())>RepairRange)return false;return false;/* Generic structural repair needs a shared durability interface; avoids falsely reporting success. */}
bool URepairToolComponent::RepairLog(ALogActor*L,float A){return L&&RepairStructure(L,A);}
bool URepairToolComponent::RepairStone(AStoneActor*S,float A){if(!S||A<=0||!GetOwner()||FVector::Dist(GetOwner()->GetActorLocation(),S->GetActorLocation())>RepairRange)return false;/* StoneActor needs an explicit Repair method to clamp health correctly. */return false;}
bool URepairToolComponent::RepairStick(AStickActor*S,float A){if(!S||A<=0||!GetOwner()||FVector::Dist(GetOwner()->GetActorLocation(),S->GetActorLocation())>RepairRange)return false;/* StickDurabilityComponent repair should be the single source of truth. */return false;}

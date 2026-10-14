#include "HordePlanSelector.h"
#include "HordePlanAsset.h"
UHordePlanAsset* UHordePlanSelector::SelectPlanForStage(int32 I,const TArray<UHordePlanAsset*>& Plans){for(UHordePlanAsset* P:Plans)if(P&&P->StageIndex==I)return P;for(UHordePlanAsset* P:Plans)if(P)return P;return nullptr;}

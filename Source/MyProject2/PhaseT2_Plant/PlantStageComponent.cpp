#include "PlantStageComponent.h"
UPlantStageComponent::UPlantStageComponent(){PrimaryComponentTick.bCanEverTick=false;}
void UPlantStageComponent::CheckStageTransition(EGrowthStage NewStage){if(NewStage!=LastStage){const auto Old=LastStage;LastStage=NewStage;OnStageChanged.Broadcast(Old,NewStage);}}

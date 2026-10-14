#include "HarvestComponent.h"
#include "PhaseT2_Plant/PlantGrowthComponent.h"
#include "PhaseT2_Plant/PlantDefinition.h"
UHarvestComponent::UHarvestComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool UHarvestComponent::CanHarvest()const{if(const AActor* O=GetOwner())if(const UPlantGrowthComponent* G=O->FindComponentByClass<UPlantGrowthComponent>())return G->GetState().bIsHarvestable;return false;}
float UHarvestComponent::Harvest(){if(!CanHarvest())return 0.f;UPlantGrowthComponent* G=GetOwner()?GetOwner()->FindComponentByClass<UPlantGrowthComponent>():nullptr;if(!G)return 0.f;UPlantDefinition* D=G->GetDefinition();const float HI=D?D->HarvestIndex:HarvestIndex;const float Yield=G->HarvestBiomass()*FMath::Clamp(HI,0.f,1.f);G->ResetPlant();return Yield;}

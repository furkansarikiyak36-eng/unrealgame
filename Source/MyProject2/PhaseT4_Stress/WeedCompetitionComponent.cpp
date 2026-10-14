#include "WeedCompetitionComponent.h"
UWeedCompetitionComponent::UWeedCompetitionComponent(){PrimaryComponentTick.bCanEverTick=false;}
void UWeedCompetitionComponent::UpdateWeedBiomass(float G,float R){WeedBiomass=FMath::Clamp(WeedBiomass+FMath::Max(0.f,G)-FMath::Max(0.f,R),0.f,FMath::Max(0.f,MaxWeedBiomass));}
float UWeedCompetitionComponent::ComputeYieldMultiplier()const{return MaxWeedBiomass<=0.f?1.f:FMath::Max(MinYieldMultiplier,1.f-CompetitionAlpha*WeedBiomass/MaxWeedBiomass);}

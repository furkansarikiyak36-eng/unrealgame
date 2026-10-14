#include "BiomassComponent.h"
UBiomassComponent::UBiomassComponent(){PrimaryComponentTick.bCanEverTick=false;}
void UBiomassComponent::AccumulateBiomass(float DailyTranspiration,float WaterUseEfficiency,float StressMultiplier){Biomass=FMath::Clamp(Biomass+FMath::Max(0.f,DailyTranspiration)*FMath::Max(0.f,WaterUseEfficiency)*FMath::Clamp(StressMultiplier,0.f,1.f),0.f,MaxBiomass);}
float UBiomassComponent::GetBiomass() const{return Biomass;}
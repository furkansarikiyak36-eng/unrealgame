#include "WaterStressComponent.h"
UWaterStressComponent::UWaterStressComponent(){PrimaryComponentTick.bCanEverTick=false;}
float UWaterStressComponent::ComputeStress(float SoilWater,float Wilting,float Critical) const{return Critical<=Wilting?0.f:FMath::Clamp((SoilWater-Wilting)/(Critical-Wilting),0.f,1.f);}
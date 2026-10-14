#include "QualityComponent.h"
UQualityComponent::UQualityComponent(){PrimaryComponentTick.bCanEverTick=false;}
float UQualityComponent::ComputeQuality(float BaseQuality,float DaysPastMaturity,float DamageFactor) const{const float M=FMath::Clamp(1.f-FMath::Max(0.f,DaysPastMaturity)*DecayRatePerDay,.3f,1.f);return FMath::Clamp(BaseQuality,0.f,1.f)*M*FMath::Clamp(DamageFactor,0.f,1.f);}
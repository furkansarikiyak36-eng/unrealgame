#include "WoolProductionComponent.h"
#include "AnimalDefinition.h"
UWoolProductionComponent::UWoolProductionComponent(){PrimaryComponentTick.bCanEverTick=false;}
void UWoolProductionComponent::AccumulateDaily(float N,float H,float S){if(!Definition)return;DaysSinceShearing+=1.f;AccumulatedWool+=FMath::Max(0.f,Definition->MaxWoolPerDay)*FMath::Clamp(N,0.f,1.f)*FMath::Clamp(H,0.f,1.f)*FMath::Clamp(S,0.f,1.5f);}
float UWoolProductionComponent::ShearWool(){if(DaysSinceShearing<DaysBetweenShearing)return 0.f;const float X=AccumulatedWool;AccumulatedWool=0.f;DaysSinceShearing=0.f;return X;}

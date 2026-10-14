#include "AnimalGrowthComponent.h"
#include "AnimalDefinition.h"
UAnimalGrowthComponent::UAnimalGrowthComponent(){PrimaryComponentTick.bCanEverTick=false;}
void UAnimalGrowthComponent::InitializeAge(float A){AgeDays=FMath::Max(0.f,A);if(Definition)BodyWeight=Definition->GompertzWeight(AgeDays);}
void UAnimalGrowthComponent::UpdateDaily(float D,float N,float H,float E){if(!Definition||D<=0.f)return;AgeDays+=D;const float Target=Definition->GompertzWeight(AgeDays);const float Rate=Definition->GompertzGrowthRate(AgeDays)*FMath::Clamp(N,0.f,1.f)*FMath::Clamp(H,0.f,1.f)*FMath::Clamp(E,0.f,1.f);BodyWeight=FMath::Max(.5f,FMath::Min(BodyWeight+Rate*D,Target));}

#include "EggProductionComponent.h"
#include "AnimalDefinition.h"
UEggProductionComponent::UEggProductionComponent(){PrimaryComponentTick.bCanEverTick=false;}
float UEggProductionComponent::ComputeDailyEggs(float A,float N,float H,float S){if(!Definition)return 0.f;const float X=Definition->MaxEggsPerDay*FMath::Clamp(A,0.f,1.f)*FMath::Clamp(N,0.f,1.f)*FMath::Clamp(H,0.f,1.f)*FMath::Clamp(S,0.f,1.5f);EggAccumulator+=FMath::Max(0.f,X);return X;}
int32 UEggProductionComponent::CollectEggs(){const int32 N=FMath::FloorToInt(EggAccumulator);EggAccumulator-=N;return N;}

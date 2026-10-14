#include "NutrientStressComponent.h"
UNutrientStressComponent::UNutrientStressComponent(){PrimaryComponentTick.bCanEverTick=false;}
float UNutrientStressComponent::ComputeStress(float Available,float Demand) const{return Demand<=0.f?1.f:FMath::Clamp(Available/Demand,0.f,1.f);}
float UNutrientStressComponent::ComputeCombinedStress(float N,float P,float K,float NDem,float PDem,float KDem) const{return FMath::Min3(ComputeStress(N,NDem),ComputeStress(P,PDem),ComputeStress(K,KDem));}
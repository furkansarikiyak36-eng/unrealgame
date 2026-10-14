#include "MilkProductionComponent.h"
#include "AnimalDefinition.h"
UMilkProductionComponent::UMilkProductionComponent(){PrimaryComponentTick.bCanEverTick=false;}
float UMilkProductionComponent::ComputePotentialMilk(float D)const{if(!Definition||D<=0.f||D>Definition->LactationLength)return 0.f;const float Raw=LactationCurveA*FMath::Pow(D,LactationCurveB)*FMath::Exp(-LactationCurveC*D);return FMath::Clamp(Raw*Definition->MaxMilkPerDay,0.f,Definition->MaxMilkPerDay);}
float UMilkProductionComponent::ComputeActualMilk(float P,float E,float Protein,float H,float T){const float Potential=FMath::Max(0.f,P)*FMath::Clamp(E,0.f,1.f)*FMath::Clamp(Protein,0.f,1.f)*FMath::Clamp(H,0.f,1.f)*FMath::Clamp(T,0.f,1.f);const float Actual=FMath::Min(Potential,FMath::Max(0.f,MaxStoredMilk-StoredMilk));StoredMilk+=Actual;return Actual;}
float UMilkProductionComponent::Milking(float A){const float X=FMath::Min(FMath::Max(0.f,A),StoredMilk);StoredMilk-=X;return X;}

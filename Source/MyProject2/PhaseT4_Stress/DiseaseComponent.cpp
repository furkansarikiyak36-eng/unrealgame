#include "DiseaseComponent.h"
UDiseaseComponent::UDiseaseComponent(){PrimaryComponentTick.bCanEverTick=false;}
void UDiseaseComponent::UpdatePressure(float H,float T,float C,float Dt){const float HF=FMath::Clamp(H,0.f,1.f),TF=FMath::Clamp(1.f-FMath::Abs(T-22.f)/15.f,0.f,1.f),Crowd=FMath::Max(0.f,C);DiseasePressure=FMath::Clamp(DiseasePressure+BaseContagionRate*HF*TF*Crowd*FMath::Max(0.f,Dt),0.f,FMath::Max(0.f,MaxPressure));}
void UDiseaseComponent::ApplyTreatment(float A){DiseasePressure=FMath::Max(0.f,DiseasePressure-FMath::Max(0.f,A));}
float UDiseaseComponent::ComputeYieldMultiplier()const{return FMath::Max(MinYieldMultiplier,1.f-DiseasePressure);}

#include "AnimalEnergyComponent.h"
#include "AnimalDefinition.h"
UAnimalEnergyComponent::UAnimalEnergyComponent(){PrimaryComponentTick.bCanEverTick=false;}
float UAnimalEnergyComponent::ComputeRequirements(float W,bool G,bool L,bool P,float T)const{if(!Definition)return 0.f;float R=Definition->MaintenanceEnergy*FMath::Pow(FMath::Max(1.f,W),.75f);if(G)R+=Definition->GrowthEnergy*.1f;if(L)R+=Definition->MilkEnergy*10.f;if(P)R+=Definition->PregnancyEnergy;return FMath::Max(0.f,R+FMath::Abs(T)*.05f);}
void UAnimalEnergyComponent::UpdateEnergyBalance(float I,float R,float D){EnergyBalance=I-R;BodyConditionScore=FMath::Clamp(BodyConditionScore+k_B*EnergyBalance*FMath::Max(0.f,D),BCS_Min,BCS_Max);}

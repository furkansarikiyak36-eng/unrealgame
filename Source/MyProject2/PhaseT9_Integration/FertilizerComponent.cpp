#include "FertilizerComponent.h"
#include "PhaseT3_Soil/SoilCellComponent.h"
#include "PhaseT3_Soil/SoilNutrientComponent.h"
#include "ManureComponent.h"
UFertilizerComponent::UFertilizerComponent(){PrimaryComponentTick.bCanEverTick=false;}
void UFertilizerComponent::ApplyFertilizer(FName Type,float A){if(A<=0.f||!GetOwner())return;if(USoilNutrientComponent* N=GetOwner()->FindComponentByClass<USoilNutrientComponent>()){const float E=A*FMath::Clamp(SyntheticUptakeEfficiency,0.f,1.f);if(Type==TEXT("Nitrogen"))N->UpdateNitrogen(E,0.f,0.f,0.f);else if(Type==TEXT("Phosphorus"))N->UpdatePhosphorus(E,0.f,0.f);else if(Type==TEXT("Potassium"))N->UpdatePotassium(E,0.f,0.f);}}
void UFertilizerComponent::ApplyManure(float Amount,float NitrogenContent){if(Amount<=0.f||NitrogenContent<=0.f||!GetOwner())return;if(USoilNutrientComponent* N=GetOwner()->FindComponentByClass<USoilNutrientComponent>())N->UpdateNitrogen(0.f,NitrogenContent*FMath::Clamp(ManureUptakeEfficiency,0.f,1.f),0.f,0.f);if(UManureComponent* M=GetOwner()->FindComponentByClass<UManureComponent>())M->CollectManure(Amount);}

#include "AnimalNutritionComponent.h"
#include "AnimalDefinition.h"
UAnimalNutritionComponent::UAnimalNutritionComponent(){PrimaryComponentTick.bCanEverTick=false;}
float UAnimalNutritionComponent::ComputeIntakeCapacity(float W,float T,float H)const{return Definition?FMath::Max(0.f,Definition->k_I*FMath::Pow(FMath::Max(1.f,W),.75f)*FMath::Clamp(T,0.f,1.f)*FMath::Clamp(H,0.f,1.f)):0.f;}
float UAnimalNutritionComponent::ComputeActualIntake(float I,float A)const{return FMath::Min(FMath::Max(0.f,I),FMath::Max(0.f,A));}
float UAnimalNutritionComponent::ComputeEnergyIntake(float A,const FFeedItem& F)const{return FMath::Max(0.f,A)*FMath::Clamp(F.DryMatter,0.f,1.f)*FMath::Max(0.f,F.MetabolizableEnergy);}
float UAnimalNutritionComponent::ComputeProteinIntake(float A,const FFeedItem& F)const{return FMath::Max(0.f,A)*FMath::Clamp(F.DryMatter,0.f,1.f)*FMath::Clamp(F.CrudeProtein,0.f,1.f);}
float UAnimalNutritionComponent::ComputeSuitability(float I,float R)const{return R<=0.f?1.f:FMath::Clamp(I/R,0.f,1.f);}

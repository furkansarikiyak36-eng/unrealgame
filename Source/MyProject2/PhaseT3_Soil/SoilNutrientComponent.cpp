#include "SoilNutrientComponent.h"
#include "SoilCellComponent.h"
USoilNutrientComponent::USoilNutrientComponent(){PrimaryComponentTick.bCanEverTick=false;}
void USoilNutrientComponent::UpdateNitrogen(float F,float M,float U,float L){if(AActor* O=GetOwner())if(USoilCellComponent* S=O->FindComponentByClass<USoilCellComponent>()){auto& X=S->GetMutableSoilState();X.Nitrogen=FMath::Max(0.f,X.Nitrogen+F+M*ManureNitrogenEfficiency-U-L);}}
void USoilNutrientComponent::UpdatePhosphorus(float A,float U,float L){if(AActor* O=GetOwner())if(USoilCellComponent* S=O->FindComponentByClass<USoilCellComponent>()){auto& X=S->GetMutableSoilState();X.Phosphorus=FMath::Max(0.f,X.Phosphorus+A-U-L);}}
void USoilNutrientComponent::UpdatePotassium(float A,float U,float L){if(AActor* O=GetOwner())if(USoilCellComponent* S=O->FindComponentByClass<USoilCellComponent>()){auto& X=S->GetMutableSoilState();X.Potassium=FMath::Max(0.f,X.Potassium+A-U-L);}}
void USoilNutrientComponent::ApplyDailyLeaching(){if(AActor* O=GetOwner())if(USoilCellComponent* S=O->FindComponentByClass<USoilCellComponent>()){auto& X=S->GetMutableSoilState();X.Nitrogen=FMath::Max(0.f,X.Nitrogen*(1.f-FMath::Clamp(LeachingRate,0.f,1.f)));}}
float USoilNutrientComponent::ComputeSuitability(float A,float D)const{return D<=0.f?1.f:FMath::Clamp(A/D,0.f,1.f);}

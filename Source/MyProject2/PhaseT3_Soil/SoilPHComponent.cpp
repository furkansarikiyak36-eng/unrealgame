#include "SoilPHComponent.h"
#include "SoilCellComponent.h"
USoilPHComponent::USoilPHComponent(){PrimaryComponentTick.bCanEverTick=false;}
float USoilPHComponent::ComputePHSuitability(float Opt,float Tol)const{if(Tol<=SMALL_NUMBER)return 0.f;if(AActor* O=GetOwner())if(USoilCellComponent* S=O->FindComponentByClass<USoilCellComponent>())return FMath::Exp(-FMath::Pow(S->GetSoilState().pH-Opt,2.f)/(2.f*Tol*Tol));return 1.f;}
void USoilPHComponent::ApplyLime(float A){if(AActor* O=GetOwner())if(USoilCellComponent* S=O->FindComponentByClass<USoilCellComponent>())S->GetMutableSoilState().pH=FMath::Clamp(S->GetSoilState().pH+LimeEffectPerUnit*FMath::Max(0.f,A),4.f,9.f);}
void USoilPHComponent::ApplySulfur(float A){if(AActor* O=GetOwner())if(USoilCellComponent* S=O->FindComponentByClass<USoilCellComponent>())S->GetMutableSoilState().pH=FMath::Clamp(S->GetSoilState().pH+SulfurEffectPerUnit*FMath::Max(0.f,A),4.f,9.f);}

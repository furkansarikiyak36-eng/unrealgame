#include "SoilCellComponent.h"
USoilCellComponent::USoilCellComponent(){PrimaryComponentTick.bCanEverTick=false;}
void USoilCellComponent::UpdateDaily(float P,float I,float E,float T,float RO,float DP){SoilState.Water=FMath::Clamp(SoilState.Water+FMath::Max(0.f,P)+FMath::Max(0.f,I)-FMath::Max(0.f,RO)-FMath::Max(0.f,DP)-FMath::Max(0.f,E)-FMath::Max(0.f,T),0.f,FMath::Max(0.f,SoilState.WaterMax));}

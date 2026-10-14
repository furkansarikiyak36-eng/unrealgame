#include "SoilMoistureComponent.h"
USoilMoistureComponent::USoilMoistureComponent(){PrimaryComponentTick.bCanEverTick=false;}
float USoilMoistureComponent::ComputeWaterStress(float W,float Wilt,float Critical)const{return Critical<=Wilt?0.f:FMath::Clamp((W-Wilt)/(Critical-Wilt),0.f,1.f);}
float USoilMoistureComponent::ComputeActualTranspiration(float Ks,float Potential)const{return FMath::Max(0.f,FMath::Clamp(Ks,0.f,1.f)*FMath::Max(0.f,Potential));}
float USoilMoistureComponent::ComputeEvaporation(float T,float H)const{return BaseEvaporationRate*FMath::Clamp((T-5.f)/30.f,0.f,1.5f)*FMath::Clamp(1.f-H,0.f,1.f);}

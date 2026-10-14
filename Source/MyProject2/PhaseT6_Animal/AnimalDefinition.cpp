#include "AnimalDefinition.h"
float UAnimalDefinition::GompertzWeight(float A)const{const float Winf=FMath::Max(.01f,W_infinity);return Winf*FMath::Exp(-FMath::Max(0.f,Gompertz_b)*FMath::Exp(-FMath::Max(0.f,Gompertz_k)*FMath::Max(0.f,A)));}
float UAnimalDefinition::GompertzGrowthRate(float A)const{const float W=GompertzWeight(A);return FMath::Max(0.f,Gompertz_k)*W*FMath::Max(0.f,FMath::Loge(FMath::Max(1.f,W_infinity)/FMath::Max(.01f,W)));}

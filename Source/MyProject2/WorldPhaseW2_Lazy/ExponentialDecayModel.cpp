#include "ExponentialDecayModel.h"
float UExponentialDecayModel::ComputeDecay(float V,float K,float D){return FMath::Max(0.0f,V)*FMath::Exp(-FMath::Max(0.0f,K)*FMath::Max(0.0f,D));}
float UExponentialDecayModel::ComputeEffectiveRate(float K,float M,float T){const float MF=FMath::Clamp(M*2.0f,0.3f,2.0f);const float TF=FMath::Clamp((T+10.0f)/30.0f,0.2f,2.0f);return FMath::Max(0.0f,K)*MF*TF;}
float UExponentialDecayModel::ComputeLinearDecay(float V,float K,float D){return FMath::Max(0.0f,V-FMath::Max(0.0f,K)*FMath::Max(0.0f,D));}
float UExponentialDecayModel::ComputeLogisticDecay(float V,float Capacity,float Rate,float D){if(V<=0||Capacity<=0)return 0;const float E=FMath::Exp(-FMath::Max(0.0f,Rate)*FMath::Max(0.0f,D));const float Den=Capacity+V*(E-1);return Den>SMALL_NUMBER?Capacity*V*E/Den:0;}

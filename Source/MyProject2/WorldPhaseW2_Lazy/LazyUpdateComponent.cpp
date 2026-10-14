#include "LazyUpdateComponent.h"
#include "ExponentialDecayModel.h"
ULazyUpdateComponent::ULazyUpdateComponent(){PrimaryComponentTick.bCanEverTick=false;}
float ULazyUpdateComponent::GetDecayedValue(float V,float K,float Now,float Moisture,float Temp){if(bIsDirty)return V;const float D=Now-LastUpdateDay;if(D<=0)return V;return UExponentialDecayModel::ComputeDecay(V,UExponentialDecayModel::ComputeEffectiveRate(K,Moisture,Temp),D);}
void ULazyUpdateComponent::MarkDirty(){bIsDirty=true;}
void ULazyUpdateComponent::MarkClean(float Now){bIsDirty=false;LastUpdateDay=FMath::Max(0.0f,Now);}

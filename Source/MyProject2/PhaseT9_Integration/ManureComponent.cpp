#include "ManureComponent.h"
UManureComponent::UManureComponent(){PrimaryComponentTick.bCanEverTick=false;}
float UManureComponent::ComputeManureNitrogen(float NitrogenIntake,float NitrogenRetained,float NitrogenProducts) const{return FMath::Max(0.f,NitrogenIntake-NitrogenRetained-NitrogenProducts);}
void UManureComponent::AccumulateManure(float Amount,float NitrogenContent)
{
 if(!FMath::IsFinite(Amount)||!FMath::IsFinite(NitrogenContent)||!FMath::IsFinite(ManureStorageMax)||Amount<=0.f||NitrogenContent<0.f||ManureStorageMax<0.f)return;
 const float Accepted=FMath::Min(Amount,FMath::Max(0.f,ManureStorageMax-ManureAmount));
 ManureAmount+=Accepted;ManureNitrogen+=NitrogenContent*(Accepted/Amount);
}
float UManureComponent::CollectManure(float Amount){if(!FMath::IsFinite(Amount)||Amount<=0.f||!FMath::IsFinite(ManureAmount)||ManureAmount<=0.f)return 0.f;const float Taken=FMath::Min(Amount,ManureAmount);const float Ratio=ManureAmount>SMALL_NUMBER?Taken/ManureAmount:0.f;ManureAmount-=Taken;ManureNitrogen*=1.f-Ratio;return Taken;}
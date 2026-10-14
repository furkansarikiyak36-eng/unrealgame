#include "TorchComponent.h"
#include "Components/PointLightComponent.h"
UTorchComponent::UTorchComponent(){PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickInterval=.5f;}
void UTorchComponent::Ignite(float F){if(F>0&&CurrentFuel<=0)CurrentFuel=FMath::Min(F,MaxFuel);if(CurrentFuel>0)bIsLit=true;if(auto*L=GetOwner()?GetOwner()->FindComponentByClass<UPointLightComponent>():nullptr)L->SetIntensity(bIsLit?3000:0);}
void UTorchComponent::Extinguish(){bIsLit=false;if(auto*L=GetOwner()?GetOwner()->FindComponentByClass<UPointLightComponent>():nullptr)L->SetIntensity(0);}
void UTorchComponent::AddFuel(float A){if(A>0)CurrentFuel=FMath::Min(CurrentFuel+A,MaxFuel);}
void UTorchComponent::TickComponent(float D,ELevelTick T,FActorComponentTickFunction* F){Super::TickComponent(D,T,F);if(!bIsLit)return;CurrentFuel=FMath::Max(0.f,CurrentFuel-BurnRate*D);if(CurrentFuel<=0)Extinguish();if(auto*L=GetOwner()?GetOwner()->FindComponentByClass<UPointLightComponent>():nullptr)L->SetIntensity(3000.f*CurrentFuel/FMath::Max(MaxFuel,1.f));}

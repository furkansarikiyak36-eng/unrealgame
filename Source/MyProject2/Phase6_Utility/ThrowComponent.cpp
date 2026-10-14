#include "Phase6_Utility/ThrowComponent.h"
#include "Phase1_Physical/LogActor.h"
#include "Components/PrimitiveComponent.h"
UThrowComponent::UThrowComponent(){PrimaryComponentTick.bCanEverTick=true;}
void UThrowComponent::StartCharge(){ChargeTime=0;bCharging=true;}
void UThrowComponent::ReleaseCharge(){if(!bCharging||!Target||!GetOwner())return;const float A=FMath::Clamp(ChargeTime/FMath::Max(MaxChargeTime,0.01f),0.0f,1.0f);ALogActor* L=Target;Target=nullptr;bCharging=false;if(L->SetLogState(ELogState::Falling))if(UPrimitiveComponent* P=Cast<UPrimitiveComponent>(L->GetRootComponent()))P->AddImpulse(GetOwner()->GetActorForwardVector()*MaxForce*A,NAME_None,true);}
void UThrowComponent::SetTarget(ALogActor* L){Target=L;}
void UThrowComponent::TickComponent(float D,ELevelTick T,FActorComponentTickFunction* F){Super::TickComponent(D,T,F);if(bCharging)ChargeTime=FMath::Min(ChargeTime+D,FMath::Max(MaxChargeTime,0.01f));}

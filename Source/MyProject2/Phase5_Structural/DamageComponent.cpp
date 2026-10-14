#include "Phase5_Structural/DamageComponent.h"
UDamageComponent::UDamageComponent(){PrimaryComponentTick.bCanEverTick=false;}
void UDamageComponent::BeginPlay(){Super::BeginPlay();Health=FMath::Clamp(Health,0.0f,MaxHealth);}
void UDamageComponent::ApplyDamage(float A){if(A<=0)return;Health=FMath::Max(0.0f,Health-A);}

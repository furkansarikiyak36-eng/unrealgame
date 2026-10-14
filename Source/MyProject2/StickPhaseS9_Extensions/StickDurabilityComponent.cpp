#include "StickDurabilityComponent.h"
UStickDurabilityComponent::UStickDurabilityComponent(){PrimaryComponentTick.bCanEverTick=false;} void UStickDurabilityComponent::ApplyStress(float A){if(A<=0||Health<=0)return;Health=FMath::Max(0.f,Health-A*StressMultiplier);if(Health<=0)OnStickBroke.Broadcast();} void UStickDurabilityComponent::Repair(float A){if(A>0)Health=FMath::Min(Health+A,MaxHealth);}

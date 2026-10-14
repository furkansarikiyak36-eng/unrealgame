#include "AnimalHealthComponent.h"
UAnimalHealthComponent::UAnimalHealthComponent(){PrimaryComponentTick.bCanEverTick=false;}
void UAnimalHealthComponent::UpdateHealth(float R,float S,float D,float Dt){Health=FMath::Clamp(Health+(R-S-D)*FMath::Max(0.f,Dt),0.f,FMath::Max(0.f,MaxHealth));}
void UAnimalHealthComponent::ApplyTreatment(float A){Health=FMath::Clamp(Health+FMath::Max(0.f,A),0.f,FMath::Max(0.f,MaxHealth));}

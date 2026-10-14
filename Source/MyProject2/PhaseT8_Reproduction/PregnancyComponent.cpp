#include "PregnancyComponent.h"
#include "PhaseT6_Animal/AnimalDefinition.h"
UPregnancyComponent::UPregnancyComponent(){PrimaryComponentTick.bCanEverTick=false;}
void UPregnancyComponent::StartPregnancy(){if(!Definition||bIsPregnant)return;bIsPregnant=true;DaysRemaining=FMath::Max(1.f,Definition->GestationDays);}
int32 UPregnancyComponent::UpdatePregnancy(float D,int32 Capacity){if(!bIsPregnant)return 0;DaysRemaining-=FMath::Max(0.f,D);if(DaysRemaining>0.f)return 0;bIsPregnant=false;const int32 Born=FMath::Clamp(BaseOffspringCount,0,FMath::Max(0,Capacity));OnBirth.Broadcast(Born);return Born;}

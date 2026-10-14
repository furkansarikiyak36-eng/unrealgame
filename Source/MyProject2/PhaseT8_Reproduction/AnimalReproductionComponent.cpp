#include "AnimalReproductionComponent.h"
#include "Engine/World.h"
#include "PhaseT6_Animal/AnimalDefinition.h"
#include "PregnancyComponent.h"
#include "PhaseT6_Animal/AnimalActor.h"
#include "HerdManager.h"
#include "SimCore/DeterministicRandom.h"
UAnimalReproductionComponent::UAnimalReproductionComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool UAnimalReproductionComponent::IsEligible(float A,float H,float B,int32 C)const{return Definition&&A>=Definition->AgeMaturityDays&&H>=MinHealth&&B>=MinBCS&&C>0;}
float UAnimalReproductionComponent::ComputeConceptionProbability(float H,float B,float N)const{if(!Definition)return 0.f;return FMath::Clamp(Definition->BaseConceptionRate*FMath::Clamp(H/100.f,0.f,1.f)*FMath::Clamp((B-1.f)/4.f,0.f,1.f)*FMath::Clamp(N,0.f,1.f),0.f,1.f);}
void UAnimalReproductionComponent::UpdateReproduction(float Age,float H,float B,float N,float D,int32 Seed){if(!Definition||D<=0.f)return;int32 Capacity=0;if(AActor* Owner=GetOwner())if(UWorld* World=Owner->GetWorld())if(UHerdManager* Herd=World->GetSubsystem<UHerdManager>())Capacity=Herd->GetFreeCapacity();if(!IsEligible(Age,H,B,Capacity))return;UPregnancyComponent* P=GetOwner()?GetOwner()->FindComponentByClass<UPregnancyComponent>():nullptr;if(!P||P->IsPregnant())return;const float Rate=ComputeConceptionProbability(H,B,N);ReproductionProgress+=Rate*D;if(ReproductionProgress>=FMath::Max(.01f,ConceptionThreshold)){FDeterministicRandom R(uint32(Seed)^uint32(++RollSequence*7919u));if(R.Roll(Rate)){P->StartPregnancy();ReproductionProgress=0.f;}else ReproductionProgress=FMath::Min(ReproductionProgress,ConceptionThreshold*.99f);}}

#include "AnimalMarketComponent.h"
#include "PhaseT6_Animal/AnimalActor.h"
#include "PhaseT6_Animal/AnimalDefinition.h"
#include "HerdManager.h"
#include "Engine/World.h"
UAnimalMarketComponent::UAnimalMarketComponent(){PrimaryComponentTick.bCanEverTick=false;}
AAnimalActor* UAnimalMarketComponent::PurchaseAnimal(UAnimalDefinition* D,EAnimalSex S,float A){if(!D||!AnimalClass||!GetWorld()||!GetOwner())return nullptr;if(UHerdManager* H=GetWorld()->GetSubsystem<UHerdManager>();H&&H->GetFreeCapacity()<=0)return nullptr;FActorSpawnParameters P;P.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;auto* Animal=GetWorld()->SpawnActor<AAnimalActor>(AnimalClass,GetOwner()->GetActorLocation(),FRotator::ZeroRotator,P);if(Animal){Animal->Initialize(D,S,A);if(UHerdManager* H=GetWorld()->GetSubsystem<UHerdManager>())H->RegisterAnimal(Animal);}return Animal;}
float UAnimalMarketComponent::ComputeAnimalValue(AAnimalActor* A)const{return A?FMath::Max(0.f,A->State.BodyWeight)*2.f+FMath::Clamp(A->State.Health,0.f,100.f)*5.f:0.f;}
float UAnimalMarketComponent::SellAnimal(AAnimalActor* A){if(!A)return 0.f;const float V=ComputeAnimalValue(A);if(GetWorld())if(UHerdManager* H=GetWorld()->GetSubsystem<UHerdManager>())H->UnregisterAnimal(A);A->Destroy();return V;}

#include "HerdManager.h"
#include "PhaseT6_Animal/AnimalActor.h"
#include "BarnActor.h"
void UHerdManager::Initialize(FSubsystemCollectionBase& C){Super::Initialize(C);}
void UHerdManager::RegisterAnimal(AAnimalActor* A){if(A)Animals.AddUnique(TWeakObjectPtr<AAnimalActor>(A));}
void UHerdManager::UnregisterAnimal(AAnimalActor* A){if(A)Animals.Remove(TWeakObjectPtr<AAnimalActor>(A));}
void UHerdManager::RegisterBarn(ABarnActor* B){if(B)Barns.AddUnique(TWeakObjectPtr<ABarnActor>(B));}
int32 UHerdManager::GetTotalAnimalCount()const{int32 N=0;for(const auto& A:Animals)if(A.IsValid()&&!A->IsActorBeingDestroyed())++N;return N;}
int32 UHerdManager::GetFreeCapacity()const{int32 C=0;for(const auto& B:Barns)if(B.IsValid())C+=FMath::Max(0,B->Capacity);return FMath::Max(0,C-GetTotalAnimalCount());}
void UHerdManager::UpdateAllAnimals(){for(auto& A:Animals)if(A.IsValid())A->UpdateDaily();Animals.RemoveAll([](const TWeakObjectPtr<AAnimalActor>& A){return !A.IsValid();});}

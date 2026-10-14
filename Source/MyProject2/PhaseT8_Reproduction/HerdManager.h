#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HerdManager.generated.h"
class AAnimalActor; class ABarnActor;
UCLASS() class MYPROJECT2_API UHerdManager:public UWorldSubsystem{GENERATED_BODY()
public: virtual void Initialize(FSubsystemCollectionBase& Collection)override; UFUNCTION(BlueprintCallable,Category="Herd") void RegisterAnimal(AAnimalActor* Animal); UFUNCTION(BlueprintCallable,Category="Herd") void UnregisterAnimal(AAnimalActor* Animal); UFUNCTION(BlueprintCallable,Category="Herd") void RegisterBarn(ABarnActor* Barn); UFUNCTION(BlueprintPure,Category="Herd") int32 GetTotalAnimalCount()const; UFUNCTION(BlueprintPure,Category="Herd") int32 GetFreeCapacity()const; UFUNCTION(BlueprintCallable,Category="Herd") void UpdateAllAnimals();
protected: UPROPERTY() TArray<TWeakObjectPtr<AAnimalActor>> Animals; UPROPERTY() TArray<TWeakObjectPtr<ABarnActor>> Barns;};

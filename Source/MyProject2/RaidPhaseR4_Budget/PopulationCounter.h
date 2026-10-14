#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PopulationCounter.generated.h"
UCLASS() class MYPROJECT2_API UPopulationCounter : public UObject { GENERATED_BODY() public: void OnEntitySpawned(AActor* Entity,FName GroupID); void OnEntityDied(AActor* Entity); void OnEntityRemoved(AActor* Entity); UFUNCTION(BlueprintPure) int32 GetLiveCount() const; UFUNCTION(BlueprintPure) int32 GetLiveCountByGroup(FName GroupID) const; UFUNCTION(BlueprintPure) bool IsAlive(AActor* Entity) const; protected: TMap<TWeakObjectPtr<AActor>,FName> LiveEntities; };

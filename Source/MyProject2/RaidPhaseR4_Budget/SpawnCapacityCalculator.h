#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SpawnCapacityCalculator.generated.h"
UCLASS() class MYPROJECT2_API USpawnCapacityCalculator : public UObject { GENERATED_BODY() public: UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 WaveCapacity=8; UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 GlobalCapacity=20; UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 EventCapacity=15; UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 PlayerCount=1; UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 PerPlayerCap=5; UFUNCTION(BlueprintPure) int32 ComputeEffectiveCapacity() const; UFUNCTION(BlueprintPure) int32 ComputeFreeCapacity(int32 LiveCount) const; UFUNCTION(BlueprintPure) int32 ComputePartyCapacity() const; };

#pragma once
#include "CoreMinimal.h"
#include "RaidTypes.h"
#include "HordeWaveSequencer.generated.h"
class UHordePlanAsset;
UCLASS() class MYPROJECT2_API UHordeWaveSequencer : public UObject { GENERATED_BODY() public: UFUNCTION(BlueprintCallable) static TArray<FWavePlan> SequenceWaves(UHordePlanAsset* Plan); UFUNCTION(BlueprintCallable) static int32 GetWaveSpawnBudget(const FWavePlan& Wave); UFUNCTION(BlueprintCallable) static float GetWaveDuration(const FWavePlan& Wave); UFUNCTION(BlueprintCallable) static float GetIntermissionDuration(const FWavePlan& Wave); UFUNCTION(BlueprintCallable) static bool IsFinalWave(int32 WaveIndex,int32 TotalWaves); };

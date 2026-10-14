#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RaidInputNormalizer.generated.h"
UCLASS() class MYPROJECT2_API URaidInputNormalizer : public UObject { GENERATED_BODY() public: UFUNCTION(BlueprintPure) static float ClampPlayerLevel(int32 Level,int32 Min,int32 Max); UFUNCTION(BlueprintPure) static float NormalizeSurvivalTime(float RawTime,float MaxTime); UFUNCTION(BlueprintPure) static float NormalizeDeathPenalty(int32 DeathCount,float Factor); UFUNCTION(BlueprintPure) static float NormalizeWorldMultiplier(float Raw,float Min,float Max); UFUNCTION(BlueprintPure) static float NormalizeBiomeDifficulty(int32 Raw,int32 MaxBiome); };

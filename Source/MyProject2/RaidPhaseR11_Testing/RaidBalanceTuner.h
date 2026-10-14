#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RaidBalanceTuner.generated.h"
UCLASS() class MYPROJECT2_API URaidBalanceTuner : public UObject { GENERATED_BODY() public: UFUNCTION(BlueprintCallable) void TuneDifficultyMultiplier(float NewValue); UFUNCTION(BlueprintCallable) void TuneSpawnRate(float NewRate); UFUNCTION(BlueprintCallable) void TuneWaveDuration(float NewDuration); UFUNCTION(BlueprintCallable) void ExportSettings(FString& OutJson) const; UPROPERTY(EditAnywhere,BlueprintReadWrite) float DifficultyMultiplier=1.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float SpawnRateMultiplier=1.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float WaveDurationMultiplier=1.f; };

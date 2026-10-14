#include "RaidBalanceTuner.h"
void URaidBalanceTuner::TuneDifficultyMultiplier(float V){DifficultyMultiplier=FMath::Clamp(V,.1f,10.f);}
void URaidBalanceTuner::TuneSpawnRate(float V){SpawnRateMultiplier=FMath::Clamp(V,.1f,10.f);}
void URaidBalanceTuner::TuneWaveDuration(float V){WaveDurationMultiplier=FMath::Clamp(V,.1f,10.f);}
void URaidBalanceTuner::ExportSettings(FString& Out)const{Out=FString::Printf(TEXT("{\"DifficultyMultiplier\":%.2f,\"SpawnRateMultiplier\":%.2f,\"WaveDurationMultiplier\":%.2f}"),DifficultyMultiplier,SpawnRateMultiplier,WaveDurationMultiplier);}

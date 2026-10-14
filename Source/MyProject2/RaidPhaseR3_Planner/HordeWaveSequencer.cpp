#include "HordeWaveSequencer.h"
#include "HordePlanAsset.h"
TArray<FWavePlan> UHordeWaveSequencer::SequenceWaves(UHordePlanAsset* P){return P?P->Waves:TArray<FWavePlan>();}
int32 UHordeWaveSequencer::GetWaveSpawnBudget(const FWavePlan&W){return FMath::Max(0,W.TotalCount);}
float UHordeWaveSequencer::GetWaveDuration(const FWavePlan&W){return FMath::Max(0.f,W.Duration);}
float UHordeWaveSequencer::GetIntermissionDuration(const FWavePlan&W){return FMath::Max(0.f,W.Intermission);}
bool UHordeWaveSequencer::IsFinalWave(int32 I,int32 N){return N>0&&I>=N-1;}

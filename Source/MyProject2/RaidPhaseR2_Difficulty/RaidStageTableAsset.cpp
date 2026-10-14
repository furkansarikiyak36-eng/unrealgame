#include "RaidStageTableAsset.h"
bool URaidStageTableAsset::ValidateTable(FString& Err) const {if(Stages.IsEmpty()){Err=TEXT("Stages boş");return false;}TArray<FRaidStageConfig> Sorted=Stages;Sorted.Sort([](const auto&A,const auto&B){return A.Threshold<B.Threshold;});for(int32 I=1;I<Sorted.Num();++I)if(Sorted[I].Threshold<=Sorted[I-1].Threshold){Err=TEXT("Threshold değerleri benzersiz ve artan olmalı");return false;}return true;}
const FRaidStageConfig* URaidStageTableAsset::FindStageConfig(int32 I) const {for(const FRaidStageConfig& S:Stages)if(S.StageIndex==I)return &S;return nullptr;}

#include "RaidStageTableAsset.h"
bool URaidStageTableAsset::ValidateTable(FString& Err) const
{
    if(Stages.IsEmpty()||Stages.Num()>64){Err=TEXT("Stages boş veya 64 aşama sınırını aşıyor");return false;}
    if(!FMath::IsFinite(LevelMultiplier)||LevelMultiplier<0.f||LevelMultiplier>100.f
        ||!FMath::IsFinite(SurvivalTimeMultiplier)||SurvivalTimeMultiplier<0.f||SurvivalTimeMultiplier>10000.f
        ||!FMath::IsFinite(DeathPenaltyFactor)||DeathPenaltyFactor<0.f||DeathPenaltyFactor>10000.f
        ||!FMath::IsFinite(PartyWeightFirst)||PartyWeightFirst<0.f||PartyWeightFirst>100.f
        ||!FMath::IsFinite(PartyWeightDecay)||PartyWeightDecay<0.f||PartyWeightDecay>1.f)
    {Err=TEXT("Difficulty multipliers are outside supported ranges");return false;}
    TArray<FRaidStageConfig> Sorted=Stages;
    Sorted.Sort([](const auto&A,const auto&B){return A.Threshold<B.Threshold;});
    TSet<int32> StageIndexes;
    for(int32 I=0;I<Sorted.Num();++I)
    {
        const FRaidStageConfig& S=Sorted[I];
        if(I>0&&S.Threshold<=Sorted[I-1].Threshold){Err=TEXT("Threshold değerleri benzersiz ve artan olmalı");return false;}
        if(S.StageIndex<0||S.StageIndex>1024||StageIndexes.Contains(S.StageIndex)){Err=TEXT("StageIndex geçersiz veya yinelenmiş");return false;}
        StageIndexes.Add(S.StageIndex);
        if(S.Budget.MinEnemyLevel<1||S.Budget.MaxEnemyLevel<S.Budget.MinEnemyLevel||S.Budget.MaxEnemyLevel>1000
            ||S.Budget.ConcurrentPopulation<1||S.Budget.ConcurrentPopulation>256
            ||S.Budget.TotalBudget<1||S.Budget.TotalBudget>100000
            ||!FMath::IsFinite(S.Budget.RaidDuration)||S.Budget.RaidDuration<1.f||S.Budget.RaidDuration>86400.f)
        {Err=TEXT("Stage budget is outside supported ranges");return false;}
    }
    Err.Empty();return true;
}
const FRaidStageConfig* URaidStageTableAsset::FindStageConfig(int32 I) const {for(const FRaidStageConfig& S:Stages)if(S.StageIndex==I)return &S;return nullptr;}

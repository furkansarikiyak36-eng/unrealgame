#include "PlantDefinition.h"

bool UPlantDefinition::ValidateDefinition(FString& OutError) const
{
    if (PlantID.IsNone()) { OutError = TEXT("PlantID boş"); return false; }
    if (StageThresholds.IsEmpty()) { OutError = TEXT("StageThresholds boş"); return false; }
    if (BaseTemperature >= MaxTemperature) { OutError = TEXT("BaseTemperature >= MaxTemperature"); return false; }
    if (OptimalTemperatureC < BaseTemperature || OptimalTemperatureC > MaxTemperature) { OutError = TEXT("OptimalTemperatureC, BaseTemperature ve MaxTemperature aralığında olmalı"); return false; }
    if (MinimumDaylightHours > MaximumDaylightHours) { OutError = TEXT("MinimumDaylightHours > MaximumDaylightHours"); return false; }
    if (FrostLethalThresholdC >= FrostDamageThresholdC) { OutError = TEXT("FrostLethalThresholdC, FrostDamageThresholdC değerinden düşük olmalı"); return false; }
    if (HeatLethalThresholdC <= HeatStressThresholdC) { OutError = TEXT("HeatLethalThresholdC, HeatStressThresholdC değerinden yüksek olmalı"); return false; }
    if (MaxBiomass <= 0.f || StressDaysBeforeFailure <= 0.f) { OutError = TEXT("MaxBiomass ve StressDaysBeforeFailure sıfırdan büyük olmalı"); return false; }
    float Previous = -1.f;
    for (const FGDDStageThreshold& Stage : StageThresholds)
    {
        if (Stage.GDDRequired < 0.f || Stage.GDDRequired <= Previous) { OutError = TEXT("GDD eşikleri negatif veya artan değil"); return false; }
        Previous = Stage.GDDRequired;
    }
    for (const TPair<EWorldSeason, float>& Pair : SeasonSuitability)
        if (!FMath::IsFinite(Pair.Value) || Pair.Value < 0.f || Pair.Value > 1.f) { OutError = TEXT("SeasonSuitability değerleri 0..1 aralığında olmalı"); return false; }
    for (const TPair<EWorldRegionType, float>& Pair : RegionSuitability)
        if (!FMath::IsFinite(Pair.Value) || Pair.Value < 0.f || Pair.Value > 1.f) { OutError = TEXT("RegionSuitability değerleri 0..1 aralığında olmalı"); return false; }
    OutError.Reset();
    return true;
}

EGrowthStage UPlantDefinition::GetStageForGDD(float GDD) const
{
    EGrowthStage Result = EGrowthStage::Seed;
    for (const FGDDStageThreshold& Stage : StageThresholds)
    {
        if (GDD >= Stage.GDDRequired) Result = Stage.Stage;
        else break;
    }
    return Result;
}

float UPlantDefinition::GetStageProgress(float GDD) const
{
    if (StageThresholds.IsEmpty()) return 0.f;
    for (int32 Index = 0; Index < StageThresholds.Num(); ++Index)
    {
        if (GDD < StageThresholds[Index].GDDRequired)
        {
            const float Previous = Index ? StageThresholds[Index - 1].GDDRequired : 0.f;
            const float Range = StageThresholds[Index].GDDRequired - Previous;
            return Range > SMALL_NUMBER ? FMath::Clamp((GDD - Previous) / Range, 0.f, 1.f) : 1.f;
        }
    }
    return 1.f;
}

float UPlantDefinition::GetTotalGDDToMaturity() const
{
    for (int32 Index = StageThresholds.Num() - 1; Index >= 0; --Index)
        if (StageThresholds[Index].Stage == EGrowthStage::Maturity) return StageThresholds[Index].GDDRequired;
    return StageThresholds.IsEmpty() ? 0.f : StageThresholds.Last().GDDRequired;
}

float UPlantDefinition::GetSeasonSuitability(EWorldSeason Season) const
{
    const float* Value = SeasonSuitability.Find(Season);
    return Value ? FMath::Clamp(*Value, 0.f, 1.f) : 1.f;
}

float UPlantDefinition::GetRegionSuitability(EWorldRegionType RegionType) const
{
    const float* Value = RegionSuitability.Find(RegionType);
    return Value ? FMath::Clamp(*Value, 0.f, 1.f) : 1.f;
}
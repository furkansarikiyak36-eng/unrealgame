#include "PlantGrowthComponent.h"
#include "PlantDefinition.h"

UPlantGrowthComponent::UPlantGrowthComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UPlantGrowthComponent::SetDefinition(UPlantDefinition* NewDef)
{
    Definition = NewDef;
    ResetPlant();
}

void UPlantGrowthComponent::ResetPlant()
{
    const int32 PreviousSeeds = PlantState.SeedCount;
    PlantState = FPlantState();
    PlantState.SeedCount = PreviousSeeds;
    PlantState.bIsAlive = true;
    PlantState.Stage = EGrowthStage::Seed;
}

void UPlantGrowthComponent::SetStress(float WaterFactor, float NutrientFactor, float WeedFactor)
{
    PlantState.WaterStress = FMath::Clamp(WaterFactor, 0.f, 1.f);
    PlantState.NutrientStress = FMath::Clamp(NutrientFactor, 0.f, 1.f);
    PlantState.WeedPressure = FMath::Clamp(WeedFactor, 0.f, 1.f);
}

float UPlantGrowthComponent::EstimateHarvestYield() const
{
    if (!Definition || !PlantState.bIsAlive) return 0.f;
    const float Quality = FMath::Clamp(PlantState.QualityIndex, Definition->MinimumQualityAfterStress, 1.f);
    return FMath::Max(0.f, PlantState.Biomass * Definition->HarvestIndex * Quality);
}

float UPlantGrowthComponent::HarvestBiomass()
{
    const float Out = PlantState.Biomass;
    PlantState.Biomass = 0.f;
    PlantState.AccumulatedGDD = 0.f;
    PlantState.Stage = EGrowthStage::Seed;
    PlantState.StageProgress = 0.f;
    PlantState.bIsHarvestable = false;
    PlantState.bIsAlive = true;
    PlantState.DaysSincePlanting = 0.f;
    PlantState.CumulativeSevereStressDays = 0.f;
    PlantState.FrostDamage = 0.f;
    PlantState.HeatDamage = 0.f;
    PlantState.DiseaseSeverity = 0.f;
    PlantState.YieldPotential = 0.f;
    PlantState.QualityIndex = 1.f;
    PlantState.bHasFailed = false;
    return Out;
}

void UPlantGrowthComponent::UpdateDaily(float TempMax, float TempMin, float DeltaTime)
{
    if (!Definition || !PlantState.bIsAlive || DeltaTime <= 0.f) return;
    const float Average = (TempMax + TempMin) * 0.5f;
    const float GDD = FMath::Max(0.f, FMath::Min(Average, Definition->MaxTemperature) - Definition->BaseTemperature) * DeltaTime;
    const float WaterFactor = FMath::Clamp(PlantState.WaterStress, 0.f, 1.f);
    const float NutrientFactor = FMath::Clamp(PlantState.NutrientStress, 0.f, 1.f);
    const float WeedFactor = FMath::Max(0.3f, 1.f - FMath::Clamp(PlantState.WeedPressure, 0.f, 1.f));
    PlantState.AccumulatedGDD += GDD;
    PlantState.Stage = Definition->GetStageForGDD(PlantState.AccumulatedGDD);
    PlantState.StageProgress = Definition->GetStageProgress(PlantState.AccumulatedGDD);
    const float DailyWaterUse = Definition->BaseWaterDemandMmPerDay * WaterFactor * DeltaTime;
    const float Gain = DailyWaterUse * Definition->WaterUseEfficiency * NutrientFactor * WeedFactor;
    PlantState.Biomass = FMath::Clamp(PlantState.Biomass + Gain, 0.f, Definition->MaxBiomass);
    PlantState.bIsHarvestable = PlantState.Stage == EGrowthStage::Maturity;
}

void UPlantGrowthComponent::UpdateDailyAdvanced(
    float TempMax, float TempMin, float RainfallMm, float DaylightHours,
    EWorldSeason Season, EWorldRegionType RegionType, bool bHasKnownRegion,
    FSoilState& Soil, float DeltaDays)
{
    if (!Definition || !PlantState.bIsAlive || PlantState.bHasFailed || DeltaDays <= 0.f) return;

    const float Days = FMath::Clamp(DeltaDays, 0.f, 7.f);
    PlantState.DaysSincePlanting += Days;
    PlantState.WaterUptakeTodayMm = 0.f;
    PlantState.NitrogenUptakeToday = 0.f;
    PlantState.PhosphorusUptakeToday = 0.f;
    PlantState.PotassiumUptakeToday = 0.f;

    const float TMean = (TempMax + TempMin) * 0.5f;
    const float ThermalRange = FMath::Max(1.f, Definition->MaxTemperature - Definition->BaseTemperature);
    const float GDD = FMath::Clamp(TMean, Definition->BaseTemperature, Definition->MaxTemperature) - Definition->BaseTemperature;
    const float ColdFactor = FMath::Clamp((TMean - Definition->BaseTemperature + 4.f) / 4.f, 0.f, 1.f);
    const float HeatFactor = TempMax <= Definition->HeatStressThresholdC ? 1.f :
        FMath::Clamp((Definition->HeatLethalThresholdC - TempMax) /
        FMath::Max(1.f, Definition->HeatLethalThresholdC - Definition->HeatStressThresholdC), 0.f, 1.f);
    const float ThermalSuitability = FMath::Clamp(1.f - FMath::Abs(TMean - Definition->OptimalTemperatureC) / ThermalRange, 0.15f, 1.f);
    const float PhotoperiodRange = FMath::Max(1.f, Definition->MaximumDaylightHours - Definition->MinimumDaylightHours);
    const float PhotoperiodFactor = FMath::Clamp((DaylightHours - Definition->MinimumDaylightHours) / PhotoperiodRange, 0.2f, 1.f);
    const float SeasonFactor = Definition->GetSeasonSuitability(Season);
    const float RegionFactor = bHasKnownRegion ? Definition->GetRegionSuitability(RegionType) : 1.f;

    const float WaterRange = FMath::Max(1.f, Soil.WaterCritical - Soil.WaterWilting);
    const float WaterFactor = FMath::Clamp((Soil.Water - Soil.WaterWilting) / WaterRange, 0.f, 1.f);
    const float NitrogenFactor = Definition->NitrogenDemand <= 0.f ? 1.f : FMath::Clamp(Soil.Nitrogen / Definition->NitrogenDemand, 0.f, 1.f);
    const float PhosphorusFactor = Definition->PhosphorusDemand <= 0.f ? 1.f : FMath::Clamp(Soil.Phosphorus / Definition->PhosphorusDemand, 0.f, 1.f);
    const float PotassiumFactor = Definition->PotassiumDemand <= 0.f ? 1.f : FMath::Clamp(Soil.Potassium / Definition->PotassiumDemand, 0.f, 1.f);
    const float NutrientFactor = FMath::Min3(NitrogenFactor, PhosphorusFactor, PotassiumFactor);
    const float PHFactor = Definition->PHTolerance <= 0.f ? 1.f :
        FMath::Clamp(1.f - FMath::Abs(Soil.pH - Definition->OptimalPH) / Definition->PHTolerance, 0.f, 1.f);
    const float WeedFactor = FMath::Clamp(Soil.WeedBiomass / FMath::Max(1.f, Definition->WeedBiomassForMaximumPressure), 0.f, 1.f);
    const float SoilDisease = FMath::Clamp(Soil.DiseasePressure, 0.f, 1.f);

    PlantState.WaterStress = WaterFactor;
    PlantState.NutrientStress = NutrientFactor * PHFactor;
    PlantState.WeedPressure = WeedFactor;

    const float StageDemandProgress = FMath::Clamp(
        static_cast<float>(static_cast<uint8>(PlantState.Stage)) /
        static_cast<float>(static_cast<uint8>(EGrowthStage::Maturity)), 0.f, 1.f);
    const float PotentialWaterDemand = FMath::Lerp(Definition->BaseWaterDemandMmPerDay,
        Definition->PeakWaterDemandMmPerDay, StageDemandProgress)
        * FMath::Max(0.f, Definition->WaterDemandStageFactor) * Days;
    const float AvailableWater = FMath::Max(0.f, Soil.Water - Soil.WaterWilting);
    const float WaterUptake = FMath::Min(AvailableWater, PotentialWaterDemand * FMath::Clamp(0.25f + WaterFactor * 0.75f, 0.f, 1.f));
    Soil.Water = FMath::Clamp(Soil.Water - WaterUptake, 0.f, FMath::Max(0.f, Soil.WaterMax));
    PlantState.WaterUptakeTodayMm = WaterUptake;

    const float ExistingBiomassRatio = FMath::Clamp(PlantState.Biomass / FMath::Max(1.f, Definition->MaxBiomass), 0.f, 1.f);
    const float CropCycleGDD = FMath::Max(1.f, Definition->GetTotalGDDToMaturity());
    const float UptakeScale = FMath::Clamp(GDD / CropCycleGDD, 0.f, 0.1f)
        * Days * FMath::Clamp(Definition->NutrientUptakeFractionPerCropCycle, 0.f, 1.f);
    const float NRequest = Definition->NitrogenDemand * UptakeScale * (0.35f + ExistingBiomassRatio);
    const float PRequest = Definition->PhosphorusDemand * UptakeScale * (0.35f + ExistingBiomassRatio);
    const float KRequest = Definition->PotassiumDemand * UptakeScale * (0.35f + ExistingBiomassRatio);
    PlantState.NitrogenUptakeToday = FMath::Min(Soil.Nitrogen, FMath::Max(0.f, NRequest));
    PlantState.PhosphorusUptakeToday = FMath::Min(Soil.Phosphorus, FMath::Max(0.f, PRequest));
    PlantState.PotassiumUptakeToday = FMath::Min(Soil.Potassium, FMath::Max(0.f, KRequest));
    Soil.Nitrogen = FMath::Max(0.f, Soil.Nitrogen - PlantState.NitrogenUptakeToday);
    Soil.Phosphorus = FMath::Max(0.f, Soil.Phosphorus - PlantState.PhosphorusUptakeToday);
    Soil.Potassium = FMath::Max(0.f, Soil.Potassium - PlantState.PotassiumUptakeToday);

    const float WetnessRisk = FMath::Clamp((FMath::Max(0.f, RainfallMm) - 2.f) / 25.f, 0.f, 1.f);
    const float DiseaseGrowth = Definition->DailyDiseaseSpread * (0.35f + WetnessRisk * 0.65f) * (0.5f + SoilDisease);
    const float DiseaseRecovery = Definition->DailyDiseaseRecovery * (1.f - WetnessRisk) * (1.f - SoilDisease);
    PlantState.DiseaseSeverity = FMath::Clamp(PlantState.DiseaseSeverity + (DiseaseGrowth - DiseaseRecovery) * Days, 0.f, 1.f);

    if (TempMin < Definition->FrostDamageThresholdC)
    {
        const float FrostSeverity = FMath::Clamp((Definition->FrostDamageThresholdC - TempMin) /
            FMath::Max(1.f, Definition->FrostDamageThresholdC - Definition->FrostLethalThresholdC), 0.f, 1.f);
        PlantState.FrostDamage = FMath::Clamp(PlantState.FrostDamage + FrostSeverity * 0.18f * Days, 0.f, 1.f);
    }
    if (TempMax > Definition->HeatStressThresholdC)
    {
        const float HeatSeverity = FMath::Clamp((TempMax - Definition->HeatStressThresholdC) /
            FMath::Max(1.f, Definition->HeatLethalThresholdC - Definition->HeatStressThresholdC), 0.f, 1.5f);
        PlantState.HeatDamage = FMath::Clamp(PlantState.HeatDamage + HeatSeverity * 0.08f * Days, 0.f, 1.f);
    }

    const float CombinedSuitability = WaterFactor * FMath::Sqrt(FMath::Max(0.f, NutrientFactor * PHFactor))
        * (1.f - WeedFactor * 0.65f) * (1.f - PlantState.DiseaseSeverity * 0.6f)
        * ThermalSuitability * ColdFactor * HeatFactor * PhotoperiodFactor * SeasonFactor * RegionFactor
        * (1.f - PlantState.FrostDamage * 0.8f) * (1.f - PlantState.HeatDamage * 0.7f);
    const float SafeSuitability = FMath::Clamp(CombinedSuitability, 0.f, 1.f);
    const float EffectiveGDD = FMath::Max(0.f, GDD) * SafeSuitability * Days;
    PlantState.AccumulatedGDD += EffectiveGDD;

    const float BiomassGain = EffectiveGDD * Definition->BiomassPerGDD * FMath::Clamp(0.3f + NutrientFactor * 0.7f, 0.f, 1.f);
    PlantState.Biomass = FMath::Clamp(PlantState.Biomass + BiomassGain, 0.f, Definition->MaxBiomass);
    PlantState.Stage = Definition->GetStageForGDD(PlantState.AccumulatedGDD);
    PlantState.StageProgress = Definition->GetStageProgress(PlantState.AccumulatedGDD);
    PlantState.bIsHarvestable = PlantState.Stage == EGrowthStage::Maturity && !PlantState.bHasFailed;
    const float QualityPenalty = (1.f - SafeSuitability) * 0.035f + PlantState.DiseaseSeverity * 0.04f
        + PlantState.FrostDamage * 0.08f + PlantState.HeatDamage * 0.06f;
    PlantState.QualityIndex = FMath::Clamp(PlantState.QualityIndex - QualityPenalty * Days,
        Definition->MinimumQualityAfterStress, 1.f);
    PlantState.YieldPotential = EstimateHarvestYield();

    const bool bSevereStress = SafeSuitability < 0.22f || TempMin <= Definition->FrostLethalThresholdC
        || TempMax >= Definition->HeatLethalThresholdC;
    PlantState.CumulativeSevereStressDays = bSevereStress
        ? PlantState.CumulativeSevereStressDays + Days
        : FMath::Max(0.f, PlantState.CumulativeSevereStressDays - 0.5f * Days);

    if (TempMin <= Definition->FrostLethalThresholdC || TempMax >= Definition->HeatLethalThresholdC
        || PlantState.FrostDamage >= 0.98f || PlantState.HeatDamage >= 0.98f
        || PlantState.CumulativeSevereStressDays >= Definition->StressDaysBeforeFailure)
    {
        PlantState.bIsAlive = false;
        PlantState.bIsHarvestable = false;
        PlantState.bHasFailed = true;
        PlantState.Stage = EGrowthStage::Senescence;
        PlantState.StageProgress = 1.f;
    }
    else if (PlantState.Stage == EGrowthStage::Maturity)
    {
        PlantState.StageProgress = 1.f;
    }
}
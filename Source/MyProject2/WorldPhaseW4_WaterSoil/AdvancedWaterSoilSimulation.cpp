#include "AdvancedWaterSoilSimulation.h"
#include "WorldCore/ChunkState.h"
#include "WorldPhaseW3_Event/WorldEventSubsystem.h"
#include "Engine/World.h"

void UAdvancedWaterSoilSimulation::InitializeCellHydrology(FChunkCell& Cell) const
{
    FSoilHydrologyState& H = Cell.Hydrology;
    if (H.bInitialized) return;
    const float LegacyMoisture = FMath::Clamp(Cell.SoilMoisture, 0.f, 1.f);
    H.TopLayerWaterMm = LegacyMoisture * TopLayerFieldCapacityMm;
    H.RootLayerWaterMm = LegacyMoisture * RootLayerFieldCapacityMm;
    H.DeepLayerWaterMm = LegacyMoisture * DeepLayerFieldCapacityMm;
    H.SoilTemperatureC = 12.f;
    H.bInitialized = true;
}

float UAdvancedWaterSoilSimulation::GetCellTotalWaterMm(const FChunkCell& Cell) const
{
    const float Area = FMath::Max(0.01f, CellSizeMeters * CellSizeMeters);
    return Cell.Water.Volume / Area * 1000.f
        + Cell.Hydrology.TopLayerWaterMm + Cell.Hydrology.RootLayerWaterMm
        + Cell.Hydrology.DeepLayerWaterMm + Cell.Hydrology.SnowpackWaterEquivalentMm
        + Cell.Hydrology.GroundwaterWaterEquivalentMm;
}

float UAdvancedWaterSoilSimulation::GetCellSurfaceWaterMm(const FChunkCell& Cell) const
{
    const float Area = FMath::Max(0.01f, CellSizeMeters * CellSizeMeters);
    return FMath::Max(0.f, Cell.Water.Volume) / Area * 1000.f;
}

void UAdvancedWaterSoilSimulation::ApplyPrecipitation(UChunkState* Chunk, float PrecipitationMm, bool bSnow, float AirTemperatureC)
{
    if (!Chunk || PrecipitationMm <= 0.f || Chunk->Cells.IsEmpty()) return;
    const float SafeArea = FMath::Max(0.01f, CellSizeMeters * CellSizeMeters);
    for (FChunkCell& Cell : Chunk->Cells)
    {
        InitializeCellHydrology(Cell);
        if (bSnow && AirTemperatureC <= 1.0f)
        {
            Cell.Hydrology.SnowpackWaterEquivalentMm += PrecipitationMm;
        }
        else
        {
            Cell.Water.Volume = FMath::Max(0.f, Cell.Water.Volume) + PrecipitationMm * 0.001f * SafeArea;
            Cell.Water.Level = Cell.Water.Volume / SafeArea;
            Cell.bHasWater = Cell.Water.Volume > 0.001f;
        }
        Cell.bIsDirty = true;
    }
    Chunk->MarkDirty();
}

float UAdvancedWaterSoilSimulation::GetInfiltrationFactor(const FChunkCell& Cell) const
{
    const float Permeability = FMath::Clamp(Cell.Terrain.Permeability, 0.f, 1.f);
    const float FrozenFactor = Cell.Hydrology.SoilTemperatureC <= -1.f ? 0.08f
        : Cell.Hydrology.SoilTemperatureC < 2.f ? FMath::Lerp(0.08f, 1.f, (Cell.Hydrology.SoilTemperatureC + 1.f) / 3.f) : 1.f;
    const float TopSaturation = FMath::Clamp(Cell.Hydrology.TopLayerWaterMm / FMath::Max(1.f, TopLayerFieldCapacityMm), 0.f, 1.5f);
    const float SaturationFactor = FMath::Square(FMath::Clamp(1.f - TopSaturation, 0.05f, 1.f));
    return Permeability * FrozenFactor * SaturationFactor;
}

float UAdvancedWaterSoilSimulation::GetSlopeFactor(const UChunkState* Chunk, int32 Index, int32 Axis) const
{
    if (!Chunk || Axis <= 1 || !Chunk->Cells.IsValidIndex(Index)) return 0.f;
    const int32 X = Index % Axis, Y = Index / Axis;
    const float H = Chunk->Cells[Index].Terrain.Height;
    float MaxDifference = 0.f;
    const int32 Neighbors[4] = { X > 0 ? Index - 1 : INDEX_NONE, X + 1 < Axis ? Index + 1 : INDEX_NONE,
        Y > 0 ? Index - Axis : INDEX_NONE, Y + 1 < Axis ? Index + Axis : INDEX_NONE };
    for (const int32 N : Neighbors)
        if (N != INDEX_NONE && Chunk->Cells.IsValidIndex(N))
            MaxDifference = FMath::Max(MaxDifference, FMath::Abs(H - Chunk->Cells[N].Terrain.Height));
    return FMath::Clamp(MaxDifference / FMath::Max(0.1f, CellSizeMeters), 0.f, 1.f);
}

FHydrologyStepReport UAdvancedWaterSoilSimulation::SimulateChunk(UChunkState* Chunk, float DeltaDays, float AirTemperatureC, float AirHumidity01)
{
    FHydrologyStepReport Report;
    if (!Chunk || Chunk->Cells.IsEmpty() || DeltaDays <= 0.f) return Report;
    const float Days = FMath::Clamp(DeltaDays, 0.f, 1.f);
    const int32 Axis = FMath::RoundToInt(FMath::Sqrt(static_cast<float>(Chunk->Cells.Num())));
    if (Axis <= 0 || Axis * Axis != Chunk->Cells.Num()) return Report;

    const float Area = FMath::Max(0.01f, CellSizeMeters * CellSizeMeters);
    const float ThermalResponse = 1.f - FMath::Pow(1.f - FMath::Clamp(SoilThermalResponsePerDay, 0.f, 1.f), Days);
    TArray<float> SurfaceBefore;
    TArray<float> SurfaceDelta;
    SurfaceBefore.SetNum(Chunk->Cells.Num());
    SurfaceDelta.Init(0.f, Chunk->Cells.Num());

    for (FChunkCell& Cell : Chunk->Cells)
    {
        InitializeCellHydrology(Cell);
        Report.TotalWaterBeforeMm += GetCellTotalWaterMm(Cell);
        FSoilHydrologyState& H = Cell.Hydrology;
        H.SoilTemperatureC += (AirTemperatureC - H.SoilTemperatureC) * ThermalResponse;

        if (H.SnowpackWaterEquivalentMm > 0.f && AirTemperatureC > 0.f)
        {
            const float Melt = FMath::Min(H.SnowpackWaterEquivalentMm,
                FMath::Min(MaximumSnowMeltMmPerDay * Days, AirTemperatureC * SnowMeltMmPerDegreeDay * Days));
            H.SnowpackWaterEquivalentMm -= Melt;
            Cell.Water.Volume += Melt * 0.001f * Area;
            Report.SnowmeltMm += Melt;
        }
    }

    // Infiltration, evaporation, vertical drainage and nutrient leaching are local column processes.
    for (FChunkCell& Cell : Chunk->Cells)
    {
        FSoilHydrologyState& H = Cell.Hydrology;
        const float SurfaceMm = GetCellSurfaceWaterMm(Cell);
        const float KInfiltration = BaseInfiltrationMmPerDay * GetInfiltrationFactor(Cell);
        const float Infiltration = FMath::Min(SurfaceMm, KInfiltration * Days);
        const float InfiltrationM3 = Infiltration * 0.001f * Area;
        Cell.Water.Volume = FMath::Max(0.f, Cell.Water.Volume - InfiltrationM3);
        H.TopLayerWaterMm += Infiltration;
        H.CumulativeInfiltrationMm += Infiltration;
        Report.InfiltrationMm += Infiltration;

        const float VegetationCover = FMath::Clamp(Cell.Biomass.Biomass / 100.f, 0.f, 1.f);
        const float TemperatureFactor = FMath::Clamp((AirTemperatureC + 5.f) / 30.f, 0.f, 1.7f);
        const float AtmosphericDemand = BaseEvaporationMmPerDay * TemperatureFactor
            * FMath::Clamp(1.f - FMath::Clamp(AirHumidity01, 0.f, 1.f), 0.05f, 1.f) * (1.f - VegetationCover * 0.65f) * Days;
        const float SurfaceEvaporation = FMath::Min(GetCellSurfaceWaterMm(Cell), AtmosphericDemand * 0.65f);
        Cell.Water.Volume = FMath::Max(0.f, Cell.Water.Volume - SurfaceEvaporation * 0.001f * Area);
        const float SoilEvapDemand = FMath::Max(0.f, AtmosphericDemand - SurfaceEvaporation);
        const float TopAvailable = FMath::Max(0.f, H.TopLayerWaterMm - TopLayerWiltingMm);
        const float TopEvaporation = FMath::Min(TopAvailable, SoilEvapDemand);
        H.TopLayerWaterMm -= TopEvaporation;

        const float RootAvailable = FMath::Max(0.f, H.RootLayerWaterMm - RootLayerWiltingMm);
        const float PotentialTranspiration = (0.15f + VegetationCover * 2.2f) * TemperatureFactor * Days;
        const float Transpiration = FMath::Min(RootAvailable, PotentialTranspiration);
        H.RootLayerWaterMm -= Transpiration;
        const float ET = SurfaceEvaporation + TopEvaporation + Transpiration;
        H.CumulativeEvapotranspirationMm += ET;
        Report.EvapotranspirationMm += ET;

        const float TopExcess = FMath::Max(0.f, H.TopLayerWaterMm - TopLayerFieldCapacityMm);
        const float TopToRoot = FMath::Min(TopExcess, VerticalPercolationMmPerDay * Days
            * FMath::Clamp(Cell.Terrain.Permeability, 0.f, 1.f));
        H.TopLayerWaterMm -= TopToRoot;
        H.RootLayerWaterMm += TopToRoot;

        const float RootExcess = FMath::Max(0.f, H.RootLayerWaterMm - RootLayerFieldCapacityMm);
        const float RootToDeep = FMath::Min(RootExcess, VerticalPercolationMmPerDay * 0.65f * Days
            * FMath::Clamp(Cell.Terrain.Permeability, 0.f, 1.f));
        H.RootLayerWaterMm -= RootToDeep;
        H.DeepLayerWaterMm += RootToDeep;

        const float DeepExcess = FMath::Max(0.f, H.DeepLayerWaterMm - DeepLayerFieldCapacityMm);
        const float DeepDrainage = FMath::Min(DeepExcess, VerticalPercolationMmPerDay * 0.25f * Days);
        H.DeepLayerWaterMm -= DeepDrainage;
        H.GroundwaterWaterEquivalentMm += DeepDrainage;
        Report.VerticalDrainageMm += DeepDrainage;

        // Nutrients move with percolating water. These are game-scale relative pools, not laboratory units.
        const float PercolationFraction = FMath::Clamp((TopToRoot + RootToDeep + DeepDrainage)
            / FMath::Max(1.f, TopLayerFieldCapacityMm + RootLayerFieldCapacityMm), 0.f, 0.5f);
        Cell.SoilNitrogen = FMath::Max(0.f, Cell.SoilNitrogen * (1.f - PercolationFraction * NitrogenLeachingFraction));
        Cell.SoilPhosphorus = FMath::Max(0.f, Cell.SoilPhosphorus * (1.f - PercolationFraction * PhosphorusLeachingFraction));
        Cell.SoilPotassium = FMath::Max(0.f, Cell.SoilPotassium * (1.f - PercolationFraction * PotassiumLeachingFraction));

        H.TopLayerWaterMm = FMath::Max(0.f, H.TopLayerWaterMm);
        H.RootLayerWaterMm = FMath::Max(0.f, H.RootLayerWaterMm);
        H.DeepLayerWaterMm = FMath::Max(0.f, H.DeepLayerWaterMm);
        H.GroundwaterWaterEquivalentMm = FMath::Max(0.f, H.GroundwaterWaterEquivalentMm);
        Cell.SoilMoisture = FMath::Clamp(H.RootLayerWaterMm / FMath::Max(1.f, RootLayerFieldCapacityMm), 0.f, 1.f);
        Cell.Water.Level = Cell.Water.Volume / Area;
        Cell.bHasWater = Cell.Water.Volume > 0.001f;
    }

    // Two-phase neighbour exchange: compute transfers from the same snapshot, then apply deltas.
    for (int32 I = 0; I < Chunk->Cells.Num(); ++I)
        SurfaceBefore[I] = FMath::Max(0.f, Chunk->Cells[I].Water.Volume);
    TArray<float> Outgoing;
    Outgoing.Init(0.f, Chunk->Cells.Num());
    for (int32 I = 0; I < Chunk->Cells.Num(); ++I)
    {
        const int32 X = I % Axis, Y = I / Axis;
        const int32 Neighbors[2] = { X + 1 < Axis ? I + 1 : INDEX_NONE, Y + 1 < Axis ? I + Axis : INDEX_NONE };
        for (const int32 N : Neighbors)
        {
            if (N == INDEX_NONE) continue;
            const FChunkCell& A = Chunk->Cells[I];
            const FChunkCell& B = Chunk->Cells[N];
            const float DepthA = SurfaceBefore[I] / Area;
            const float DepthB = SurfaceBefore[N] / Area;
            const float HeadA = A.Terrain.Height + DepthA;
            const float HeadB = B.Terrain.Height + DepthB;
            if (FMath::IsNearlyEqual(HeadA, HeadB, 0.0001f)) continue;
            const int32 From = HeadA > HeadB ? I : N;
            const int32 To = HeadA > HeadB ? N : I;
            const float HeadDifference = FMath::Abs(HeadA - HeadB);
            const float ProposedM3 = HeadDifference * SurfaceFlowCoefficientPerDay * Days * Area;
            const float AvailableM3 = FMath::Max(0.f, SurfaceBefore[From] * FMath::Clamp(MaxSurfaceTransferFraction, 0.f, 1.f) - Outgoing[From]);
            const float TransferM3 = FMath::Min(ProposedM3, AvailableM3);
            if (TransferM3 <= KINDA_SMALL_NUMBER) continue;
            SurfaceDelta[From] -= TransferM3;
            SurfaceDelta[To] += TransferM3;
            Outgoing[From] += TransferM3;
            Report.SurfaceFlowMm += TransferM3 / Area * 1000.f;
        }
    }

    for (int32 I = 0; I < Chunk->Cells.Num(); ++I)
    {
        FChunkCell& Cell = Chunk->Cells[I];
        Cell.Water.Volume = FMath::Max(0.f, SurfaceBefore[I] + SurfaceDelta[I]);
        Cell.Water.Level = Cell.Water.Volume / Area;
        Cell.Water.FlowRate = Days > 0.f ? SurfaceDelta[I] / Days : 0.f;
        Cell.Water.bIsFlowing = FMath::Abs(Cell.Water.FlowRate) > 0.0001f;
        Cell.bHasWater = Cell.Water.Volume > 0.001f;

        const float SurfaceMm = GetCellSurfaceWaterMm(Cell);
        const float Slope = GetSlopeFactor(Chunk, I, Axis);
        const float RunoffEnergy = SurfaceMm * (0.15f + Slope * 1.8f);
        const float Cover = FMath::Clamp(Cell.Biomass.Biomass / 100.f, 0.f, 1.f);
        const float Erosion = RunoffEnergy * ErosionCoefficient * (1.f - Cover * FMath::Clamp(VegetationProtection, 0.f, 1.f)) * Days;
        Cell.Hydrology.SurfaceRunoffMm = RunoffEnergy;
        Cell.Hydrology.CumulativeErosionIndex += Erosion;
        Cell.Hydrology.SuspendedSedimentKg += Erosion * Area * 0.001f;
        Cell.Hydrology.LastWaterBalanceErrorMm = 0.f;
        Cell.bIsDirty = true;
        Report.TotalWaterAfterMm += GetCellTotalWaterMm(Cell);
        Report.MeanRootZoneSaturation += Cell.SoilMoisture;
        Report.MeanSurfaceWaterMm += SurfaceMm;
    }

    const float CellCount = static_cast<float>(Chunk->Cells.Num());
    Report.MeanRootZoneSaturation /= CellCount;
    Report.MeanSurfaceWaterMm /= CellCount;
    // Internal transfers and vertical drainage are conservative. ET is the modeled external sink.
    Report.WaterBalanceErrorMm = Report.TotalWaterAfterMm - (Report.TotalWaterBeforeMm - Report.EvapotranspirationMm);
    const float PerCellError = Report.WaterBalanceErrorMm / CellCount;
    for (FChunkCell& Cell : Chunk->Cells) Cell.Hydrology.LastWaterBalanceErrorMm = PerCellError;
    Chunk->MarkDirty();
    return Report;
}
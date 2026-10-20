#include "WorldStateSubsystem.h"
#include "WorldPhaseW2_Lazy/SimulationBudgetAllocator.h"
#include "WorldPhaseW2_Lazy/EventChainTracker.h"
#include "WorldPhaseW1_Chunk/WorldChunkSubsystem.h"
#include "WorldPhaseW4_WaterSoil/WaterSimulation.h"
#include "WorldPhaseW4_WaterSoil/SoilSimulation.h"
#include "WorldPhaseW4_WaterSoil/AdvancedWaterSoilSimulation.h"
#include "RegionSeason/RegionSeasonSubsystem.h"
#include "RegionSeason/RegionSeasonTypes.h"
#include "WorldPhaseW5_FireDecayChem/FireSimulation.h"
#include "WorldPhaseW5_FireDecayChem/DecaySimulation.h"
#include "WorldPhaseW5_FireDecayChem/ChemistrySimulation.h"
#include "SimCore/SimulationSubsystem.h"
#include "SimCore/SimulationState.h"
#include "SimCore/DeterministicRandom.h"
#include "Engine/World.h"
void UWorldStateSubsystem::Initialize(FSubsystemCollectionBase& C)
{
    Super::Initialize(C); StateStore=NewObject<UChunkStateStore>(this); BudgetAllocator=NewObject<USimulationBudgetAllocator>(this);
    WaterSimulation=NewObject<UWaterSimulation>(this); SoilSimulation=NewObject<USoilSimulation>(this); AdvancedWaterSoilSimulation=NewObject<UAdvancedWaterSoilSimulation>(this); FireSimulation=NewObject<UFireSimulation>(this);
    DecaySimulation=NewObject<UDecaySimulation>(this); ChemistrySimulation=NewObject<UChemistrySimulation>(this); EventTracker=NewObject<UEventChainTracker>(this);
    WorldTime=FWorldTime();
}
void UWorldStateSubsystem::Deinitialize() { Super::Deinitialize(); }
TStatId UWorldStateSubsystem::GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(UWorldStateSubsystem, STATGROUP_Tickables); }
void UWorldStateSubsystem::Tick(float DeltaTime)
{
    if (!StateStore) return;

    bool bUsingSharedClock = false;
    if (UWorld* World = GetWorld())
    {
        if (USimulationSubsystem* Clock = World->GetSubsystem<USimulationSubsystem>())
        {
            if (USimulationState* SimState = Clock->GetState())
            {
                bUsingSharedClock = true;
                WorldTime.Day = FMath::Max(1, SimState->DayNumber);
                WorldTime.Hour = FMath::Clamp(SimState->HourOfDay, 0, 23);
                WorldTime.TimeOfDay = FMath::Clamp(SimState->TimeOfDay, 0.0f, 23.999f);
                TemperatureC = SimState->Temperature;
                TempMinC = SimState->TempMin;
                TempMaxC = SimState->TempMax;
                RainfallMm = SimState->Rainfall;
                WindSpeed = SimState->WindSpeed;
                bIsRaining = SimState->bIsRaining;
                bIsSnowing = SimState->bIsSnowing;
                RandomSeed = SimState->RandomSeed;
            }
        }
    }
    if (!bUsingSharedClock) UpdateWorldTimeTick(FMath::Max(0.0f, DeltaTime));

    if (BudgetAllocator)
    {
        BudgetAllocator->MaxChunksPerFrame = FMath::Max(1, MaxChunksSimulatedPerFrame);
        BudgetAllocator->ResetFrame();
    }

    TArray<UChunkState*> Chunks;
    if (UWorld* World = GetWorld())
    {
        if (UWorldChunkSubsystem* ChunkSubsystem = World->GetSubsystem<UWorldChunkSubsystem>())
        {
            for (const FIntPoint& Coord : ChunkSubsystem->LoadedChunks)
                if (UChunkState* Chunk = StateStore->FindChunk(Coord)) Chunks.Add(Chunk);
        }
    }
    if (Chunks.Num() == 0)
    {
        for (UChunkState* Chunk : StateStore->GetAllChunks())
            if (Chunk && Chunk->Metadata.ActivationMode != EChunkActivationMode::Passive) Chunks.Add(Chunk);
    }
    if (Chunks.Num() == 0) return;

    Chunks.Sort([](const UChunkState& A, const UChunkState& B)
    {
        return A.Coordinate.X == B.Coordinate.X ? A.Coordinate.Y < B.Coordinate.Y : A.Coordinate.X < B.Coordinate.X;
    });

    const double RealNow = FPlatformTime::Seconds();
    const float CurrentTotalDay = WorldTime.ToTotalDays();
    const float CurrentDayIndex = FMath::FloorToFloat(CurrentTotalDay);
    const int32 StartIndex = SimulationCursor % Chunks.Num();
    int32 Visited = 0;
    for (int32 Offset = 0; Offset < Chunks.Num(); ++Offset)
    {
        if (BudgetAllocator && !BudgetAllocator->HasTimeRemaining()) break;
        ++Visited;
        const int32 Index = (StartIndex + Offset) % Chunks.Num();
        UChunkState* Chunk = Chunks[Index];
        if (!Chunk) continue;

        // Rain is applied once per world-day per chunk. Unloaded chunks receive a bounded lazy catch-up on reactivation.
        const float WeatherDays = FMath::Max(0.0f, CurrentDayIndex - Chunk->Metadata.LastWeatherUpdateDay);
        if (WeatherDays > 0.0f)
        {
            const float HistoricalPrecipitationMm = ComputeHistoricalRainfallSince(Chunk->Metadata.LastWeatherUpdateDay, CurrentDayIndex);
            const bool bNeedsCurrentPrecipitation = CurrentDayIndex > Chunk->Metadata.LastWeatherUpdateDay;
            FRegionClimateSnapshot Climate;
            if (URegionSeasonSubsystem* RegionSeason = GetWorld()->GetSubsystem<URegionSeasonSubsystem>())
                Climate = RegionSeason->GetClimateAtLocation(ChunkSettings.ChunkToWorldCenter(Chunk->Coordinate));
            const float RegionPrecipitationScale = Climate.bHasKnownRegion
                ? FMath::Max(0.f, Climate.PrecipitationMultiplier) * FMath::Clamp(Climate.MeanAnnualRainfallMm / 600.f, 0.f, 5.f) : 1.f;
            const float RegionTemperatureOffset = Climate.bHasKnownRegion
                ? Climate.TemperatureOffsetC + (Climate.BaselineTemperatureC - 15.f) : 0.f;
            const bool bLocalSnow = bIsSnowing || (Climate.bHasKnownRegion && TempMinC + RegionTemperatureOffset <= Climate.SnowThresholdOffsetC);
            if (AdvancedWaterSoilSimulation)
            {
                AdvancedWaterSoilSimulation->CellSizeMeters = ChunkSettings.GetCellSizeMeters();
                if (HistoricalPrecipitationMm > 0.0f)
                    AdvancedWaterSoilSimulation->ApplyPrecipitation(Chunk, HistoricalPrecipitationMm * RegionPrecipitationScale, false, TempMinC + RegionTemperatureOffset);
                if (bNeedsCurrentPrecipitation && RainfallMm > 0.0f)
                    AdvancedWaterSoilSimulation->ApplyPrecipitation(Chunk, RainfallMm * RegionPrecipitationScale, bLocalSnow, TempMinC + RegionTemperatureOffset);
            }
            else if (WaterSimulation)
            {
                const float MissedRainfallMm = ComputeRainfallSince(Chunk->Metadata.LastWeatherUpdateDay, CurrentDayIndex);
                if (MissedRainfallMm > 0.0f) WaterSimulation->ApplyRainfall(Chunk, MissedRainfallMm);
            }
            Chunk->Metadata.LastWeatherUpdateDay = CurrentDayIndex;
            Chunk->MarkDirty();
        }

        if (Chunk->Metadata.ActivationMode == EChunkActivationMode::Passive) continue;
        const bool bContinuousDue = Chunk->Metadata.ActivationMode == EChunkActivationMode::Continuous;
        const bool bScheduledDue = Chunk->Metadata.ActivationMode == EChunkActivationMode::Scheduled &&
            (Chunk->Metadata.LastSimulationTime <= 0.0 || RealNow - Chunk->Metadata.LastSimulationTime >= 5.0);
        if (!bContinuousDue && !bScheduledDue) continue;
        if (BudgetAllocator && !BudgetAllocator->TryAllocateChunk()) break;

        const float ChunkDeltaDays = FMath::Max(0.0f, CurrentTotalDay - Chunk->Metadata.LastUpdateDay);
        if (ChunkDeltaDays > 0.0f) SimulateChunk(Chunk, ChunkDeltaDays, CurrentTotalDay);
        Chunk->Metadata.LastSimulationTime = RealNow;
    }
    SimulationCursor = (StartIndex + FMath::Max(1, Visited)) % Chunks.Num();
}
void UWorldStateSubsystem::UpdateWorldTimeTick(float DeltaTime)
{
    HourAccumulator += DeltaTime * FMath::Max(0.0f,GameHoursPerRealSecond);
    while(HourAccumulator>=1.0) { HourAccumulator-=1.0; WorldTime.Hour=(WorldTime.Hour+1)%24; WorldTime.TimeOfDay=(float)WorldTime.Hour; if(WorldTime.Hour==0) ++WorldTime.Day; }
}
void UWorldStateSubsystem::AdvanceWorldTime(float DeltaDays) { if(DeltaDays>0) WorldTime=FWorldTime::FromTotalDays(WorldTime.ToTotalDays()+DeltaDays); }
void UWorldStateSubsystem::SynchronizeAfterLoad()
{
    SimulationCursor=0; HourAccumulator=0.0;
    if (UWorld* World=GetWorld()) if (USimulationSubsystem* Clock=World->GetSubsystem<USimulationSubsystem>()) if (USimulationState* SimState=Clock->GetState())
    {
        SimState->DayNumber=WorldTime.Day; SimState->HourOfDay=WorldTime.Hour; SimState->TimeOfDay=WorldTime.TimeOfDay;
        SimState->Temperature=TemperatureC; SimState->TempMin=TempMinC; SimState->TempMax=TempMaxC; SimState->Rainfall=RainfallMm;
        SimState->WindSpeed=WindSpeed; SimState->bIsRaining=bIsRaining; SimState->bIsSnowing=bIsSnowing; Clock->SetRandomSeed(RandomSeed);
        Clock->TickRates.HourAccumulator=0.0f; Clock->TickRates.DayAccumulator=0.0f; Clock->TickRates.FrameAccumulator=0.0f;
    }
}
UChunkState* UWorldStateSubsystem::GetOrCreateChunkAtWorld(const FVector& P) { return StateStore?StateStore->GetOrCreateChunk(ChunkSettings.WorldToChunk(P),ChunkSettings.CellsPerChunkAxis):nullptr; }
UChunkState* UWorldStateSubsystem::GetChunk(const FIntPoint& C) const { return StateStore?StateStore->FindChunk(C):nullptr; }
FChunkCell* UWorldStateSubsystem::GetCellAtWorld(const FVector& P,const FIntPoint& Given)
{
    UChunkState* C=GetOrCreateChunkAtWorld(P); if(!C)return nullptr; FIntPoint L=Given;
    if(Given.X<0||Given.Y<0)L=ChunkSettings.WorldToLocalCell(P); FChunkCell* Cell=C->GetCell(L.X,L.Y,ChunkSettings.CellsPerChunkAxis); if(Cell) { Cell->bIsDirty=true; C->MarkDirty(); } return Cell;
}
TArray<UChunkState*> UWorldStateSubsystem::GetChunksInRadius(const FVector& Center,float RadiusMeters) const
{
    TArray<UChunkState*> Result;
    if (!StateStore) return Result;
    const float RadiusCm = FMath::Max(0.0f, RadiusMeters) * 100.0f;
    const float ChunkCm = FMath::Max(100.0f, ChunkSettings.ChunkSizeMeters * 100.0f);
    const FIntPoint CenterChunk = ChunkSettings.WorldToChunk(Center);
    const int32 Range = FMath::Clamp(FMath::CeilToInt(RadiusCm / ChunkCm) + 1, 0, 1024);
    for (int32 DX = -Range; DX <= Range; ++DX)
    {
        for (int32 DY = -Range; DY <= Range; ++DY)
        {
            const FIntPoint Coord(CenterChunk.X + DX, CenterChunk.Y + DY);
            const float MinX = Coord.X * ChunkCm, MaxX = MinX + ChunkCm;
            const float MinY = Coord.Y * ChunkCm, MaxY = MinY + ChunkCm;
            const float NearestX = FMath::Clamp(Center.X, MinX, MaxX);
            const float NearestY = FMath::Clamp(Center.Y, MinY, MaxY);
            const float DeltaX = Center.X - NearestX, DeltaY = Center.Y - NearestY;
            if (DeltaX * DeltaX + DeltaY * DeltaY > RadiusCm * RadiusCm) continue;
            if (UChunkState* Chunk = StateStore->FindChunk(Coord)) Result.Add(Chunk);
        }
    }
    return Result;
}
float UWorldStateSubsystem::ComputeRainfallForDay(int32 DayNumber) const
{
    // Mirrors the deterministic daily weather roll in USimulationSubsystem::AdvanceOneDay.
    const uint32 Seed = static_cast<uint32>(RandomSeed) ^ static_cast<uint32>(DayNumber * 7919);
    FDeterministicRandom DailyRng(Seed);
    const float Roll = DailyRng.NextFloat();
    if (Roll >= 0.75f && Roll < 0.90f) return DailyRng.Range(3.0f, 15.0f);
    if (Roll >= 0.90f && Roll < 0.98f) return DailyRng.Range(15.0f, 40.0f);
    if (Roll >= 0.98f) return DailyRng.Range(2.0f, 10.0f); // The advanced hydrology solver routes precipitation into liquid water or snowpack.
    return 0.0f;
}

float UWorldStateSubsystem::ComputeRainfallSince(float LastWeatherDay, int32 CurrentDayIndex) const
{
    const int32 FirstMissedDay = FMath::Max(2, FMath::FloorToInt(LastWeatherDay) + 1);
    if (CurrentDayIndex < FirstMissedDay) return 0.0f;

    float TotalRainfall = 0.0f;
    // Reconstruct historical days from the same seed. Use the live weather value for today,
    // because a level/weather actor may have overridden the generated roll.
    for (int32 Day = FirstMissedDay; Day <= CurrentDayIndex; ++Day)
    {
        TotalRainfall += (Day == CurrentDayIndex) ? FMath::Max(0.0f, RainfallMm) : ComputeRainfallForDay(Day);
    }
    return TotalRainfall;
}

float UWorldStateSubsystem::ComputeHistoricalRainfallSince(float LastWeatherDay, int32 CurrentDayIndex) const
{
    const int32 FirstMissedDay = FMath::Max(2, FMath::FloorToInt(LastWeatherDay) + 1);
    const int32 LastHistoricalDay = CurrentDayIndex - 1;
    if (LastHistoricalDay < FirstMissedDay) return 0.0f;
    float Total = 0.0f;
    for (int32 Day = FirstMissedDay; Day <= LastHistoricalDay; ++Day)
        Total += ComputeRainfallForDay(Day);
    return Total;
}

void UWorldStateSubsystem::RequestChunkSimulation(UChunkState* Chunk,float Days)
{
    if (!Chunk || Days <= 0.0f) return;
    const float CurrentTotalDay=WorldTime.ToTotalDays();
    const int32 CurrentDayIndex=FMath::FloorToInt(CurrentTotalDay);
    const float HistoricalPrecipitationMm = ComputeHistoricalRainfallSince(Chunk->Metadata.LastWeatherUpdateDay, CurrentDayIndex);
    const bool bNeedsCurrentPrecipitation = CurrentDayIndex > Chunk->Metadata.LastWeatherUpdateDay;
    FRegionClimateSnapshot Climate;
    if (URegionSeasonSubsystem* RegionSeason = GetWorld()->GetSubsystem<URegionSeasonSubsystem>())
        Climate = RegionSeason->GetClimateAtLocation(ChunkSettings.ChunkToWorldCenter(Chunk->Coordinate));
    const float RegionPrecipitationScale = Climate.bHasKnownRegion
        ? FMath::Max(0.f, Climate.PrecipitationMultiplier) * FMath::Clamp(Climate.MeanAnnualRainfallMm / 600.f, 0.f, 5.f) : 1.f;
    const float RegionTemperatureOffset = Climate.bHasKnownRegion
        ? Climate.TemperatureOffsetC + (Climate.BaselineTemperatureC - 15.f) : 0.f;
    const bool bLocalSnow = bIsSnowing || (Climate.bHasKnownRegion && TempMinC + RegionTemperatureOffset <= Climate.SnowThresholdOffsetC);
    if (AdvancedWaterSoilSimulation)
    {
        AdvancedWaterSoilSimulation->CellSizeMeters = ChunkSettings.GetCellSizeMeters();
        if (HistoricalPrecipitationMm > 0.0f)
            AdvancedWaterSoilSimulation->ApplyPrecipitation(Chunk, HistoricalPrecipitationMm * RegionPrecipitationScale, false, TempMinC + RegionTemperatureOffset);
        if (bNeedsCurrentPrecipitation && RainfallMm > 0.0f)
            AdvancedWaterSoilSimulation->ApplyPrecipitation(Chunk, RainfallMm * RegionPrecipitationScale, bLocalSnow, TempMinC + RegionTemperatureOffset);
    }
    else if (WaterSimulation)
    {
        const float MissedRainfallMm = ComputeRainfallSince(Chunk->Metadata.LastWeatherUpdateDay, CurrentDayIndex);
        if (MissedRainfallMm > 0.0f) WaterSimulation->ApplyRainfall(Chunk, MissedRainfallMm);
    }
    Chunk->Metadata.LastWeatherUpdateDay=FMath::Max(Chunk->Metadata.LastWeatherUpdateDay,(float)CurrentDayIndex);
    SimulateChunk(Chunk,Days,CurrentTotalDay);
    Chunk->Metadata.LastSimulationTime=FPlatformTime::Seconds();
}
void UWorldStateSubsystem::SimulateChunk(UChunkState* Chunk,float Days,float TargetDay)
{
    if (!Chunk || Days <= 0.0f) return;
    if (AdvancedWaterSoilSimulation)
    {
        AdvancedWaterSoilSimulation->CellSizeMeters = ChunkSettings.GetCellSizeMeters();
        float LocalTemperatureC = TemperatureC;
        if (URegionSeasonSubsystem* RegionSeason = GetWorld()->GetSubsystem<URegionSeasonSubsystem>())
        {
            const FRegionClimateSnapshot Climate = RegionSeason->GetClimateAtLocation(ChunkSettings.ChunkToWorldCenter(Chunk->Coordinate));
            if (Climate.bHasKnownRegion) LocalTemperatureC += Climate.TemperatureOffsetC + (Climate.BaselineTemperatureC - 15.f);
        }
        AdvancedWaterSoilSimulation->SimulateChunk(Chunk, Days, LocalTemperatureC, 0.55f);
    }
    else
    {
        if (WaterSimulation) WaterSimulation->SimulateChunk(Chunk, Days);
        if (SoilSimulation) SoilSimulation->SimulateChunk(Chunk, Days, TemperatureC);
    }
    if(FireSimulation) FireSimulation->SimulateChunk(Chunk,Days,WindSpeed,FVector2D::ZeroVector); if(DecaySimulation)DecaySimulation->SimulateChunk(Chunk,Days,TemperatureC);
    if(ChemistrySimulation)ChemistrySimulation->SimulateChunk(Chunk,Days); Chunk->Metadata.LastUpdateDay=(TargetDay>=0.0f)?TargetDay:WorldTime.ToTotalDays(); Chunk->Metadata.bHasPendingEvents=false;
}

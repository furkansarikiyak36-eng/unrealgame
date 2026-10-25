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
                WindSpeed = FMath::IsFinite(SimState->WindSpeed) ? FMath::Max(0.0f, SimState->WindSpeed) : 0.0f;
                WindDirection = SimState->WindDirection.IsNearlyZero() ? FVector2D(1.0f, 0.0f) : SimState->WindDirection.GetSafeNormal();
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
    const int32 CurrentDayIndex = FMath::FloorToInt(CurrentTotalDay);
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

        // Preserve unprocessed simulation time across budgeted frames. Newly elapsed
        // time is recorded once, separately from the chunk's simulated-through day.
        const float NewlyElapsedDays = FMath::Max(0.0f, CurrentTotalDay - Chunk->Metadata.LastObservedWorldDay);
        if (FMath::IsFinite(NewlyElapsedDays) && NewlyElapsedDays > 0.0f)
        {
            const float PendingCap = FMath::Clamp(
                FMath::IsFinite(MaxAccumulatedSimulationDays) ? MaxAccumulatedSimulationDays : 365.0f, 1.0f, 365.0f);
            const float ExistingPending = FMath::IsFinite(Chunk->Metadata.AccumulatedDeltaDays)
                ? FMath::Clamp(Chunk->Metadata.AccumulatedDeltaDays, 0.0f, PendingCap) : 0.0f;
            Chunk->Metadata.AccumulatedDeltaDays = FMath::Clamp(ExistingPending + NewlyElapsedDays, 0.0f, PendingCap);
            Chunk->Metadata.LastObservedWorldDay = CurrentTotalDay;
            Chunk->MarkDirty();
        }

        const bool bContinuousDue = Chunk->Metadata.ActivationMode == EChunkActivationMode::Continuous;
        const bool bScheduledDue = Chunk->Metadata.ActivationMode == EChunkActivationMode::Scheduled &&
            (Chunk->Metadata.LastSimulationTime <= 0.0 || RealNow - Chunk->Metadata.LastSimulationTime >= 5.0);
        if ((!bContinuousDue && !bScheduledDue) ||
            Chunk->Metadata.AccumulatedDeltaDays <= KINDA_SMALL_NUMBER) continue;
        if (BudgetAllocator && !BudgetAllocator->TryAllocateChunk()) break;

        const float MaxStep = FMath::Clamp(
            FMath::IsFinite(MaxSimulationDaysPerStep) ? MaxSimulationDaysPerStep : 1.0f, 0.01f, 1.0f);
        const float StepDays = FMath::Min(Chunk->Metadata.AccumulatedDeltaDays, MaxStep);
        if (StepDays > KINDA_SMALL_NUMBER)
        {
            SimulateChunk(Chunk, StepDays, Chunk->Metadata.LastUpdateDay + StepDays);
            Chunk->Metadata.AccumulatedDeltaDays = FMath::Max(0.0f, Chunk->Metadata.AccumulatedDeltaDays - StepDays);
            Chunk->Metadata.LastSimulationTime = RealNow;
            Chunk->MarkDirty();
        }
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
    SimulationCursor=0; HourAccumulator=FMath::Clamp(HourAccumulator, 0.0, 0.999999);
    if (UWorld* World=GetWorld()) if (USimulationSubsystem* Clock=World->GetSubsystem<USimulationSubsystem>()) if (USimulationState* SimState=Clock->GetState())
    {
        SimState->DayNumber=WorldTime.Day; SimState->HourOfDay=WorldTime.Hour; SimState->TimeOfDay=WorldTime.TimeOfDay;
        SimState->Temperature=TemperatureC; SimState->TempMin=TempMinC; SimState->TempMax=TempMaxC; SimState->Rainfall=RainfallMm;
        SimState->WindSpeed=WindSpeed; SimState->WindDirection=WindDirection.IsNearlyZero()?FVector2D(1.0f,0.0f):WindDirection.GetSafeNormal(); SimState->bIsRaining=bIsRaining; SimState->bIsSnowing=bIsSnowing; Clock->SetRandomSeed(RandomSeed);
        Clock->TickRates.HourAccumulator=0.0f; Clock->TickRates.DayAccumulator=0.0f; Clock->TickRates.FrameAccumulator=0.0f;
    }
}

bool UWorldStateSubsystem::ExportSnapshot(FWorldStateSnapshot& OutSnapshot) const
{
    if(!StateStore) return false;
    FWorldStateSnapshot Snapshot;
    Snapshot.SchemaVersion=1;
    Snapshot.CellsPerChunkAxis=StateStore->CellsPerChunkAxis;
    Snapshot.WorldTime=WorldTime;
    Snapshot.TemperatureC=TemperatureC;
    Snapshot.TempMinC=TempMinC;
    Snapshot.TempMaxC=TempMaxC;
    Snapshot.RainfallMm=RainfallMm;
    Snapshot.WindSpeed=WindSpeed;
    Snapshot.WindDirection=WindDirection;
    Snapshot.bIsRaining=bIsRaining;
    Snapshot.bIsSnowing=bIsSnowing;
    Snapshot.RandomSeed=RandomSeed;
    Snapshot.HourAccumulator=HourAccumulator;
    TArray<UChunkState*> Chunks=StateStore->GetAllChunks();
    Chunks.Sort([](const UChunkState& A,const UChunkState& B)
    {
        return A.Coordinate.X==B.Coordinate.X ? A.Coordinate.Y<B.Coordinate.Y : A.Coordinate.X<B.Coordinate.X;
    });
    Snapshot.Chunks.Reserve(Chunks.Num());
    for(const UChunkState* Chunk:Chunks)
    {
        if(!Chunk) return false;
        FChunkStateSnapshot Entry;
        Entry.Coordinate=Chunk->Coordinate;
        Entry.Cells=Chunk->Cells;
        Entry.Metadata=Chunk->Metadata;
        Snapshot.Chunks.Add(MoveTemp(Entry));
    }
    if(!ValidateSnapshot(Snapshot)) return false;
    OutSnapshot=MoveTemp(Snapshot);
    return true;
}

bool UWorldStateSubsystem::ValidateSnapshot(const FWorldStateSnapshot& Snapshot) const
{
    if(Snapshot.SchemaVersion!=1 || Snapshot.CellsPerChunkAxis<1 || Snapshot.CellsPerChunkAxis>256 ||
       Snapshot.Chunks.Num()>MaxSnapshotChunks || Snapshot.WorldTime.Day<1 ||
       Snapshot.WorldTime.Hour<0 || Snapshot.WorldTime.Hour>23 ||
       !FMath::IsFinite(Snapshot.WorldTime.TimeOfDay) || Snapshot.WorldTime.TimeOfDay<0.0f || Snapshot.WorldTime.TimeOfDay>=24.0f ||
       !FMath::IsFinite(Snapshot.TemperatureC) || Snapshot.TemperatureC < -100.0f || Snapshot.TemperatureC > 100.0f ||
       !FMath::IsFinite(Snapshot.TempMinC) || Snapshot.TempMinC < -100.0f || Snapshot.TempMinC > 100.0f ||
       !FMath::IsFinite(Snapshot.TempMaxC) || Snapshot.TempMaxC < -100.0f || Snapshot.TempMaxC > 100.0f ||
       Snapshot.TempMinC>Snapshot.TempMaxC ||
       !FMath::IsFinite(Snapshot.RainfallMm) || Snapshot.RainfallMm<0.0f || Snapshot.RainfallMm>1000.0f ||
       !FMath::IsFinite(Snapshot.WindSpeed) || Snapshot.WindSpeed<0.0f || Snapshot.WindSpeed>500.0f ||
       !FMath::IsFinite(Snapshot.WindDirection.X) || !FMath::IsFinite(Snapshot.WindDirection.Y) ||
       Snapshot.WindDirection.SizeSquared()>4.0f ||
       !FMath::IsFinite(Snapshot.HourAccumulator) || Snapshot.HourAccumulator<0.0 || Snapshot.HourAccumulator>=1.0)
        return false;

    const int64 ExpectedCells=static_cast<int64>(Snapshot.CellsPerChunkAxis)*Snapshot.CellsPerChunkAxis;
    int64 TotalCells=0;
    TSet<FIntPoint> Coordinates;
    Coordinates.Reserve(Snapshot.Chunks.Num());
    const auto Finite=[](float V){return FMath::IsFinite(V);};
    const auto InRange=[&Finite](float V,float Min,float Max){return Finite(V)&&V>=Min&&V<=Max;};
    for(const FChunkStateSnapshot& Chunk:Snapshot.Chunks)
    {
        if(Coordinates.Contains(Chunk.Coordinate) || Chunk.Cells.Num()!=ExpectedCells) return false;
        Coordinates.Add(Chunk.Coordinate);
        TotalCells+=Chunk.Cells.Num();
        if(TotalCells>MaxSnapshotCells) return false;
        const FChunkSimulationMetadata& M=Chunk.Metadata;
        if(!Finite(M.LastUpdateDay)||M.LastUpdateDay<1.0f ||
           !Finite(M.LastWeatherUpdateDay)||M.LastWeatherUpdateDay<1.0f ||
           !Finite(M.LastObservedWorldDay)||M.LastObservedWorldDay<1.0f ||
           !FMath::IsFinite(M.LastSimulationTime)||M.LastSimulationTime<0.0 ||
           !Finite(M.AccumulatedDeltaDays)||M.AccumulatedDeltaDays<0.0f||M.AccumulatedDeltaDays>365.0f ||
           static_cast<uint8>(M.ActivationMode)>static_cast<uint8>(EChunkActivationMode::Continuous) ||
           M.ActiveEventCount<0) return false;

        for(const FChunkCell& Cell:Chunk.Cells)
        {
            const FTerrainCell& T=Cell.Terrain;
            const FWaterCell& W=Cell.Water;
            const FChemistryCell& C=Cell.Chemistry;
            const FBiomassCell& B=Cell.Biomass;
            if(!Finite(T.Height)||!Finite(T.OriginalHeight)||!Finite(T.Deformation)||!InRange(T.Permeability,0.0f,1.0f) ||
               !InRange(W.Volume,0.0f,1.0e9f)||!InRange(W.Level,0.0f,1.0e9f)||!Finite(W.FlowRate) ||
               !Finite(W.FlowDirection.X)||!Finite(W.FlowDirection.Y)||
               !InRange(C.ToxinConcentration,0.0f,1.0f)||!InRange(C.Radioactivity,0.0f,1.0e6f)||
               !InRange(C.pH,0.0f,14.0f)||!InRange(C.Pollution,0.0f,1.0e6f)||
               !InRange(B.Biomass,0.0f,1.0e9f)||!InRange(B.Health,0.0f,1.0f)||
               !InRange(B.MoistureContent,0.0f,1.0f)||!InRange(B.DecayRate,0.0f,1000.0f)||
               !InRange(Cell.SoilMoisture,0.0f,1.0f)||!InRange(Cell.SoilNitrogen,0.0f,1.0e9f)||
               !InRange(Cell.SoilPhosphorus,0.0f,1.0e9f)||!InRange(Cell.SoilPotassium,0.0f,1.0e9f)||
               !InRange(Cell.SoilPH,0.0f,14.0f)) return false;
        }
    }
    return true;
}

bool UWorldStateSubsystem::ImportSnapshot(const FWorldStateSnapshot& Snapshot)
{
    if(!StateStore || !ValidateSnapshot(Snapshot) ||
       !StateStore->ReplaceAllChunks(Snapshot.Chunks,Snapshot.CellsPerChunkAxis)) return false;
    WorldTime=Snapshot.WorldTime;
    TemperatureC=Snapshot.TemperatureC;
    TempMinC=Snapshot.TempMinC;
    TempMaxC=Snapshot.TempMaxC;
    RainfallMm=Snapshot.RainfallMm;
    WindSpeed=Snapshot.WindSpeed;
    WindDirection=Snapshot.WindDirection.IsNearlyZero()?FVector2D(1.0f,0.0f):Snapshot.WindDirection.GetSafeNormal();
    bIsRaining=Snapshot.bIsRaining;
    bIsSnowing=Snapshot.bIsSnowing;
    RandomSeed=Snapshot.RandomSeed;
    HourAccumulator=FMath::Clamp(Snapshot.HourAccumulator,0.0,0.999999);
    SynchronizeAfterLoad();
    return true;
}

UChunkState* UWorldStateSubsystem::GetOrCreateChunkAtWorld(const FVector& P)
{
    if(!StateStore) return nullptr;
    const FIntPoint Coordinate=ChunkSettings.WorldToChunk(P);
    if(UChunkState* Existing=StateStore->FindChunk(Coordinate)) return Existing;
    UChunkState* Created=StateStore->GetOrCreateChunk(Coordinate,ChunkSettings.CellsPerChunkAxis);
    if(Created)
    {
        const float CurrentDay=WorldTime.ToTotalDays();
        Created->Metadata.LastUpdateDay=CurrentDay;
        Created->Metadata.LastObservedWorldDay=CurrentDay;
        Created->Metadata.LastWeatherUpdateDay=FMath::FloorToFloat(CurrentDay);
    }
    return Created;
}
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
    int32 FirstMissedDay = FMath::Max(2, FMath::FloorToInt(LastWeatherDay) + 1);
    if (CurrentDayIndex < FirstMissedDay) return 0.0f;
    // Bound deterministic lazy weather reconstruction; very old passive history is intentionally
    // approximated rather than blocking a frame with an unbounded per-day loop.
    FirstMissedDay = FMath::Max(FirstMissedDay, CurrentDayIndex - MaxRainfallDaysToReconstruct + 1);

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
    if (!Chunk || !FMath::IsFinite(Days) || Days <= 0.0f) return;
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
    Chunk->Metadata.LastObservedWorldDay=FMath::Max(Chunk->Metadata.LastObservedWorldDay,CurrentTotalDay);
    const float PendingCap=FMath::Clamp(MaxAccumulatedSimulationDays,1.0f,365.0f);
    Chunk->Metadata.AccumulatedDeltaDays=FMath::Clamp(Chunk->Metadata.AccumulatedDeltaDays+Days,0.0f,PendingCap);
    const float StepDays=FMath::Min(Chunk->Metadata.AccumulatedDeltaDays,FMath::Clamp(MaxSimulationDaysPerStep,0.01f,1.0f));
    if(StepDays>KINDA_SMALL_NUMBER)
    {
        SimulateChunk(Chunk,StepDays,Chunk->Metadata.LastUpdateDay+StepDays);
        Chunk->Metadata.AccumulatedDeltaDays=FMath::Max(0.0f,Chunk->Metadata.AccumulatedDeltaDays-StepDays);
    }
    Chunk->Metadata.LastSimulationTime=FPlatformTime::Seconds();
}

void UWorldStateSubsystem::SimulateChunk(UChunkState* Chunk,float Days,float TargetDay)
{
    const float StepDays = FMath::Min(FMath::Clamp(Days, 0.0f, 1.0f),
        FMath::Clamp(MaxSimulationDaysPerStep, 0.01f, 1.0f));
    if(!Chunk || !FMath::IsFinite(StepDays) || StepDays<=0.0f) return;
    if (AdvancedWaterSoilSimulation)
    {
        // Layered hydrology (snow, infiltration, drainage, groundwater) with the region's climate offset.
        AdvancedWaterSoilSimulation->CellSizeMeters = ChunkSettings.GetCellSizeMeters();
        float LocalTemperatureC = TemperatureC;
        if (URegionSeasonSubsystem* RegionSeason = GetWorld() ? GetWorld()->GetSubsystem<URegionSeasonSubsystem>() : nullptr)
        {
            const FRegionClimateSnapshot Climate = RegionSeason->GetClimateAtLocation(ChunkSettings.ChunkToWorldCenter(Chunk->Coordinate));
            if (Climate.bHasKnownRegion) LocalTemperatureC += Climate.TemperatureOffsetC + (Climate.BaselineTemperatureC - 15.f);
        }
        AdvancedWaterSoilSimulation->SimulateChunk(Chunk, StepDays, LocalTemperatureC, 0.55f);
    }
    else
    {
        if(WaterSimulation) WaterSimulation->SimulateChunk(Chunk,StepDays);
        if(SoilSimulation) SoilSimulation->SimulateChunk(Chunk,StepDays,TemperatureC);
    }
    if(ChemistrySimulation) ChemistrySimulation->SimulateChunk(Chunk,StepDays);
    ApplyCrossDomainCoupling(Chunk,StepDays);
    // Vegetation cells now consume their synchronized moisture/health state; the same chunk
    // fire simulation owns ignition/spread, and its direction comes from the shared weather state.
    if(FireSimulation) FireSimulation->SimulateChunk(Chunk,StepDays,WindSpeed,WindDirection);
    if(DecaySimulation) DecaySimulation->SimulateChunk(Chunk,StepDays,TemperatureC);
    Chunk->Metadata.LastUpdateDay=(TargetDay>=0.0f)?FMath::Min(TargetDay,Chunk->Metadata.LastUpdateDay+StepDays):Chunk->Metadata.LastUpdateDay+StepDays;
    Chunk->Metadata.bHasPendingEvents=false;
}

void UWorldStateSubsystem::ApplyCrossDomainCoupling(UChunkState* Chunk,float Days)
{
    if(!Chunk||!FMath::IsFinite(Days)||Days<=0.0f) return;
    const float MoistureRate=FMath::Clamp(BiomassMoistureResponsePerDay,0.0f,10.0f);
    const float ToxinRate=FMath::Clamp(ToxinBiomassDamagePerDay,0.0f,1.0f);
    const float ResponseAlpha=1.0f-FMath::Exp(-MoistureRate*Days);
    for(FChunkCell& Cell:Chunk->Cells)
    {
        FBiomassCell& Biomass=Cell.Biomass;
        if(Biomass.Biomass<=0.0f) continue;
        const float TargetMoisture=FMath::Clamp(Cell.SoilMoisture,0.0f,1.0f);
        const float OldMoisture=FMath::Clamp(FMath::IsFinite(Biomass.MoistureContent)?Biomass.MoistureContent:0.5f,0.0f,1.0f);
        const float NewMoisture=FMath::Clamp(FMath::Lerp(OldMoisture,TargetMoisture,ResponseAlpha),0.0f,1.0f);
        const float Toxin=FMath::Clamp(FMath::IsFinite(Cell.Chemistry.ToxinConcentration)?Cell.Chemistry.ToxinConcentration:0.0f,0.0f,1.0f);
        const float OldHealth=FMath::Clamp(FMath::IsFinite(Biomass.Health)?Biomass.Health:1.0f,0.0f,1.0f);
        const float NewHealth=FMath::Clamp(OldHealth-Toxin*ToxinRate*Days,0.0f,1.0f);
        if(!FMath::IsNearlyEqual(OldMoisture,NewMoisture)||!FMath::IsNearlyEqual(OldHealth,NewHealth))
        {
            Biomass.MoistureContent=NewMoisture;
            Biomass.Health=NewHealth;
            Cell.bIsDirty=true;
        }
    }
    Chunk->MarkDirty();
}

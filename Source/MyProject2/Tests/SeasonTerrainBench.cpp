#include "Tests/SeasonTerrainBench.h"
#include "WorldCore/ChunkState.h"
#include "WorldCore/WorldStateSubsystem.h"
#include "WorldPhaseW4_WaterSoil/AdvancedWaterSoilSimulation.h"
#include "WorldPhaseW4_WaterSoil/DiggableTerrainSubsystem.h"
#include "RegionSeason/RegionSeasonSubsystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "HAL/PlatformTime.h"

namespace
{
	UWorld* WorldOf(UObject* Ctx)
	{
		return GEngine ? GEngine->GetWorldFromContextObject(Ctx, EGetWorldErrorMode::ReturnNull) : nullptr;
	}

	const TCHAR* SeasonName(EWorldSeason S)
	{
		switch (S)
		{
		case EWorldSeason::Spring: return TEXT("Ilkbahar");
		case EWorldSeason::Summer: return TEXT("Yaz");
		case EWorldSeason::Autumn: return TEXT("Sonbahar");
		default: return TEXT("Kis");
		}
	}

	// Test chunks sit this far out so they never touch the play area.
	constexpr float FarAwayCm = 5.0e6f;
}

FString USeasonTerrainBench::BenchmarkHydrology(int32 ChunkCount, int32 Days, float AirTemperatureC, float RainMm)
{
	ChunkCount = FMath::Clamp(ChunkCount, 1, 4096);
	Days = FMath::Clamp(Days, 1, 3650);
	UAdvancedWaterSoilSimulation* Sim = NewObject<UAdvancedWaterSoilSimulation>();
	TArray<UChunkState*> Chunks;
	FRandomStream Rng(7);
	for (int32 i = 0; i < ChunkCount; ++i)
	{
		UChunkState* C = NewObject<UChunkState>();
		C->Initialize(16);
		C->Coordinate = FIntPoint(i, 0);
		for (FChunkCell& Cell : C->Cells) Cell.Terrain.Height = Rng.FRandRange(0.f, 2.f); // gentle slope, water moves
		Chunks.Add(C);
	}
	const int64 CellBytes = sizeof(FChunkCell);
	double Total = 0.0, Worst = 0.0;
	float WorstError = 0.f;
	float Saturation = 0.f;
	for (int32 d = 0; d < Days; ++d)
	{
		const bool bRain = (d % 3) == 0 && RainMm > 0.f;
		const double T0 = FPlatformTime::Seconds();
		for (UChunkState* C : Chunks)
		{
			if (bRain) Sim->ApplyPrecipitation(C, RainMm, AirTemperatureC < 0.f, AirTemperatureC);
			const FHydrologyStepReport R = Sim->SimulateChunk(C, 1.f, AirTemperatureC, 0.55f);
			WorstError = FMath::Max(WorstError, FMath::Abs(R.WaterBalanceErrorMm));
			Saturation = R.MeanRootZoneSaturation;
		}
		const double Ms = (FPlatformTime::Seconds() - T0) * 1000.0;
		Total += Ms;
		Worst = FMath::Max(Worst, Ms);
	}
	const double PerChunkDay = Total / (double(ChunkCount) * Days);
	const FString Out = FString::Printf(
		TEXT("HYDRO chunks=%d days=%d temp=%.1fC rain=%.1fmm | toplam=%.1f ms | chunk-gun basi=%.4f ms | en kotu gun=%.2f ms (%d chunk) | hucre=%lld bayt, chunk=%.1f KB | su dengesi hatasi max=%.4f mm | kok bolgesi doygunlugu=%.2f"),
		ChunkCount, Days, AirTemperatureC, RainMm, Total, PerChunkDay, Worst, ChunkCount, CellBytes, CellBytes * 256 / 1024.0, WorstError, Saturation);
	UE_LOG(LogTemp, Display, TEXT("%s"), *Out);
	return Out;
}

FString USeasonTerrainBench::BenchmarkWorldStep(UObject* WorldContext, int32 ChunkCount, int32 Steps)
{
	UWorld* World = WorldOf(WorldContext);
	UWorldStateSubsystem* WS = World ? World->GetSubsystem<UWorldStateSubsystem>() : nullptr;
	if (!WS) return TEXT("WORLDSTEP no world state");
	ChunkCount = FMath::Clamp(ChunkCount, 1, 2048);
	Steps = FMath::Clamp(Steps, 1, 365);
	const float ChunkCm = WS->ChunkSettings.ChunkSizeMeters * 100.f;
	TArray<UChunkState*> Chunks;
	const int32 Side = FMath::CeilToInt(FMath::Sqrt(float(ChunkCount)));
	for (int32 i = 0; i < ChunkCount; ++i)
	{
		const FVector P(FarAwayCm + (i % Side + 0.5f) * ChunkCm, FarAwayCm + (i / Side + 0.5f) * ChunkCm, 0.f);
		if (UChunkState* C = WS->GetOrCreateChunkAtWorld(P)) Chunks.Add(C);
	}
	double Total = 0.0, Worst = 0.0;
	for (int32 s = 0; s < Steps; ++s)
	{
		const double T0 = FPlatformTime::Seconds();
		for (UChunkState* C : Chunks) WS->RequestChunkSimulation(C, 1.f);
		const double Ms = (FPlatformTime::Seconds() - T0) * 1000.0;
		Total += Ms;
		Worst = FMath::Max(Worst, Ms);
	}
	const double PerChunk = Total / FMath::Max(1.0, double(Chunks.Num()) * Steps);
	const int32 Budget = WS->MaxChunksSimulatedPerFrame;
	const FString Out = FString::Printf(
		TEXT("WORLDSTEP chunks=%d steps=%d | toplam=%.1f ms | chunk-adim basi=%.4f ms | en kotu tur=%.2f ms | kare butcesi %d chunk => %.3f ms/kare"),
		Chunks.Num(), Steps, Total, PerChunk, Worst, Budget, PerChunk * Budget);
	UE_LOG(LogTemp, Display, TEXT("%s"), *Out);
	return Out;
}

FString USeasonTerrainBench::BenchmarkDigging(UObject* WorldContext, int32 BrushCount, float RadiusMeters)
{
	UWorld* World = WorldOf(WorldContext);
	UDiggableTerrainSubsystem* Dig = World ? World->GetSubsystem<UDiggableTerrainSubsystem>() : nullptr;
	UWorldStateSubsystem* WS = World ? World->GetSubsystem<UWorldStateSubsystem>() : nullptr;
	if (!Dig || !WS) return TEXT("DIGBENCH no subsystem");
	BrushCount = FMath::Clamp(BrushCount, 1, 10000);
	FRandomStream Rng(11);
	const float AreaCm = 20.f * WS->ChunkSettings.ChunkSizeMeters * 100.f;
	TArray<FVector> Spots;
	for (int32 i = 0; i < BrushCount; ++i)
	{
		const FVector P(FarAwayCm * 2.f + Rng.FRand() * AreaCm, FarAwayCm * 2.f + Rng.FRand() * AreaCm, 0.f);
		WS->GetOrCreateChunkAtWorld(P); // digging only edits cells that exist
		Spots.Add(P);
	}
	int32 Cells = 0, Fails = 0;
	float Dug = 0.f, Filled = 0.f;
	double T0 = FPlatformTime::Seconds();
	for (const FVector& P : Spots)
	{
		const FDigOperationResult R = Dig->DigAtWorld(P, RadiusMeters, 0.3f, EDigBrushShape::Circle);
		Cells += R.CellsChanged; Dug += R.ExcavatedVolumeM3; Fails += R.bSucceeded ? 0 : 1;
	}
	const double DigMs = (FPlatformTime::Seconds() - T0) * 1000.0;
	T0 = FPlatformTime::Seconds();
	for (const FVector& P : Spots) Filled += Dig->FillAtWorld(P, RadiusMeters, 1000.f, EDigBrushShape::Circle).FilledVolumeM3;
	const double FillMs = (FPlatformTime::Seconds() - T0) * 1000.0;
	T0 = FPlatformTime::Seconds();
	for (const FVector& P : Spots) Dig->PrepareSoilForPlanting(P, RadiusMeters, EDigBrushShape::Circle);
	const double TillMs = (FPlatformTime::Seconds() - T0) * 1000.0;
	const FString Out = FString::Printf(
		TEXT("DIGBENCH n=%d r=%.1fm | kazma %.4f ms/adet (ort %.1f hucre, %.2f m3, basarisiz %d) | doldurma %.4f ms/adet (%.2f m3) | capalama %.4f ms/adet"),
		BrushCount, RadiusMeters, DigMs / BrushCount, float(Cells) / BrushCount, Dug / BrushCount, Fails, FillMs / BrushCount, Filled / BrushCount, TillMs / BrushCount);
	UE_LOG(LogTemp, Display, TEXT("%s"), *Out);
	return Out;
}

FString USeasonTerrainBench::DescribeCellAtWorld(UObject* WorldContext, FVector WorldLocation)
{
	UWorld* World = WorldOf(WorldContext);
	UWorldStateSubsystem* WS = World ? World->GetSubsystem<UWorldStateSubsystem>() : nullptr;
	if (!WS) return TEXT("CELL no world state");
	WS->GetOrCreateChunkAtWorld(WorldLocation);
	const FChunkCell* C = WS->GetCellAtWorld(WorldLocation);
	if (!C) return TEXT("CELL none");
	const FTerrainCell& T = C->Terrain;
	const FSoilHydrologyState& H = C->Hydrology;
	return FString::Printf(
		TEXT("CELL h=%.3f orig=%.3f soilDepth=%.2fm dug=%.2fm treat=%d tex=%d compact=%.2f root=%.2fm | moist=%.3f N=%.1f | water top=%.1f root=%.1f deep=%.1f snow=%.1f gw=%.1f mm soilT=%.1fC runoff=%.1f init=%d"),
		T.Height, T.OriginalHeight, T.SoilDepthMeters, T.ExcavatedDepthMeters, int32(T.GroundTreatment), int32(T.SoilTexture), T.Compaction, T.RootingDepthMeters,
		C->SoilMoisture, C->SoilNitrogen, H.TopLayerWaterMm, H.RootLayerWaterMm, H.DeepLayerWaterMm, H.SnowpackWaterEquivalentMm, H.GroundwaterWaterEquivalentMm,
		H.SoilTemperatureC, H.SurfaceRunoffMm, H.bInitialized ? 1 : 0);
}

FString USeasonTerrainBench::DescribeSeasonAtWorld(UObject* WorldContext, FVector WorldLocation)
{
	UWorld* World = WorldOf(WorldContext);
	URegionSeasonSubsystem* RS = World ? World->GetSubsystem<URegionSeasonSubsystem>() : nullptr;
	if (!RS) return TEXT("SEASON no subsystem");
	const FRegionClimateSnapshot Cl = RS->GetClimateAtLocation(WorldLocation);
	return FString::Printf(
		TEXT("SEASON day=%d doy=%d %s | region=%s known=%d | base=%.1fC offset=%+.1fC rain x%.2f daylight=%.1fh"),
		RS->GetCurrentWorldDay(), RS->GetCurrentDayOfYear(), SeasonName(RS->GetCurrentSeason()), *Cl.RegionId.ToString(), Cl.bHasKnownRegion ? 1 : 0,
		Cl.BaselineTemperatureC, Cl.TemperatureOffsetC, Cl.PrecipitationMultiplier, Cl.DaylightHours);
}

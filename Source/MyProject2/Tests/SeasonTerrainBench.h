#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SeasonTerrainBench.generated.h"

/**
 * Test hooks for seasons, digging and the layered water/soil model: cost measurements and a readable dump of one
 * world cell. Called from editor Python (Scripts/pie_season_dig_test.py); nothing in the game depends on them.
 */
UCLASS()
class MYPROJECT2_API USeasonTerrainBench : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/**
	 * Layered hydrology alone on ChunkCount throwaway chunks (16x16 cells each) for Days one-day steps, rain of RainMm
	 * every third day. Returns timings (ms per chunk-day), memory per chunk and the worst water balance error.
	 */
	UFUNCTION(BlueprintCallable, Category="Tests|SeasonTerrain")
	static FString BenchmarkHydrology(int32 ChunkCount, int32 Days, float AirTemperatureC, float RainMm);

	/**
	 * The whole chunk step the game runs (rain catch-up, hydrology, chemistry, coupling, fire, decay) through
	 * UWorldStateSubsystem::RequestChunkSimulation, on ChunkCount chunks far from the play area, Steps times each.
	 */
	UFUNCTION(BlueprintCallable, Category="Tests|SeasonTerrain", meta=(WorldContext="WorldContext"))
	static FString BenchmarkWorldStep(UObject* WorldContext, int32 ChunkCount, int32 Steps);

	/** Digs, fills and tills BrushCount times on random spots far from the play area and times each kind. */
	UFUNCTION(BlueprintCallable, Category="Tests|SeasonTerrain", meta=(WorldContext="WorldContext"))
	static FString BenchmarkDigging(UObject* WorldContext, int32 BrushCount, float RadiusMeters);

	/** One world cell: height, soil depth, dug depth, treatment, compaction, layer water, snow, soil temperature. */
	UFUNCTION(BlueprintCallable, Category="Tests|SeasonTerrain", meta=(WorldContext="WorldContext"))
	static FString DescribeCellAtWorld(UObject* WorldContext, FVector WorldLocation);

	/** Calendar and climate at a spot: world day, day of year, season, temperature offset, rain multiplier, daylight. */
	UFUNCTION(BlueprintCallable, Category="Tests|SeasonTerrain", meta=(WorldContext="WorldContext"))
	static FString DescribeSeasonAtWorld(UObject* WorldContext, FVector WorldLocation);
};

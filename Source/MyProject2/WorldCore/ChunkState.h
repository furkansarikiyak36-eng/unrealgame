#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WorldTypes.h"
#include "ChunkState.generated.h"

USTRUCT(BlueprintType)
struct FChunkCell
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, BlueprintReadWrite) FTerrainCell Terrain;
    UPROPERTY(SaveGame, BlueprintReadWrite) FWaterCell Water;
    UPROPERTY(SaveGame, BlueprintReadWrite) FChemistryCell Chemistry;
    UPROPERTY(SaveGame, BlueprintReadWrite) FBiomassCell Biomass;
    UPROPERTY(SaveGame, BlueprintReadWrite) float SoilMoisture = 0.3f;
    UPROPERTY(SaveGame, BlueprintReadWrite) FSoilHydrologyState Hydrology;
    UPROPERTY(SaveGame, BlueprintReadWrite) float SoilNitrogen = 50.0f;
    UPROPERTY(SaveGame, BlueprintReadWrite) float SoilPhosphorus = 30.0f;
    UPROPERTY(SaveGame, BlueprintReadWrite) float SoilPotassium = 40.0f;
    UPROPERTY(SaveGame, BlueprintReadWrite) float SoilPH = 6.5f;
    UPROPERTY(SaveGame, BlueprintReadWrite) bool bIsDirty = false;
    UPROPERTY(SaveGame, BlueprintReadWrite) bool bHasWater = false;
    UPROPERTY(SaveGame, BlueprintReadWrite) bool bHasFire = false;
    UPROPERTY(SaveGame, BlueprintReadWrite) bool bHasToxin = false;
};
USTRUCT(BlueprintType)
struct FChunkSimulationMetadata
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, BlueprintReadWrite) float LastUpdateDay = 1.0f;
    UPROPERTY(SaveGame, BlueprintReadWrite) float LastWeatherUpdateDay = 1.0f;
    UPROPERTY(SaveGame, BlueprintReadWrite) float LastObservedWorldDay = 1.0f;
    UPROPERTY(SaveGame, BlueprintReadWrite) double LastSimulationTime = 0.0;
    UPROPERTY(SaveGame, BlueprintReadWrite) float AccumulatedDeltaDays = 0.0f;
    UPROPERTY(SaveGame, BlueprintReadWrite) EChunkActivationMode ActivationMode = EChunkActivationMode::Passive;
    UPROPERTY(SaveGame, BlueprintReadWrite) bool bHasPendingEvents = false;
    UPROPERTY(SaveGame, BlueprintReadWrite) bool bIsInitialized = false;
    UPROPERTY(SaveGame, BlueprintReadWrite) int32 ActiveEventCount = 0;
};
/** Save-boundary value snapshot; runtime ownership remains in UChunkStateStore. */
USTRUCT(BlueprintType)
struct FChunkStateSnapshot
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, BlueprintReadOnly) FIntPoint Coordinate = FIntPoint::ZeroValue;
    UPROPERTY(SaveGame, BlueprintReadOnly) TArray<FChunkCell> Cells;
    UPROPERTY(SaveGame, BlueprintReadOnly) FChunkSimulationMetadata Metadata;
};

USTRUCT(BlueprintType)
struct FWorldStateSnapshot
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, BlueprintReadOnly) int32 SchemaVersion = 1;
    UPROPERTY(SaveGame, BlueprintReadOnly) int32 CellsPerChunkAxis = 16;
    UPROPERTY(SaveGame, BlueprintReadOnly) FWorldTime WorldTime;
    UPROPERTY(SaveGame, BlueprintReadOnly) float TemperatureC = 20.0f;
    UPROPERTY(SaveGame, BlueprintReadOnly) float TempMinC = 15.0f;
    UPROPERTY(SaveGame, BlueprintReadOnly) float TempMaxC = 25.0f;
    UPROPERTY(SaveGame, BlueprintReadOnly) float RainfallMm = 0.0f;
    UPROPERTY(SaveGame, BlueprintReadOnly) float WindSpeed = 0.0f;
    UPROPERTY(SaveGame, BlueprintReadOnly) FVector2D WindDirection = FVector2D(1.0f, 0.0f);
    UPROPERTY(SaveGame, BlueprintReadOnly) bool bIsRaining = false;
    UPROPERTY(SaveGame, BlueprintReadOnly) bool bIsSnowing = false;
    UPROPERTY(SaveGame, BlueprintReadOnly) int32 RandomSeed = 12345;
    UPROPERTY(SaveGame, BlueprintReadOnly) double HourAccumulator = 0.0;
    UPROPERTY(SaveGame, BlueprintReadOnly) TArray<FChunkStateSnapshot> Chunks;
};

UCLASS(BlueprintType)
class MYPROJECT2_API UChunkState : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame, BlueprintReadWrite, Category="Identity") FIntPoint Coordinate = FIntPoint::ZeroValue;
    UPROPERTY(SaveGame, BlueprintReadWrite, Category="Cells") TArray<FChunkCell> Cells;
    // Sparse per-cell extension payloads (e.g. MaterialInstanceState binary snapshots).
    TMap<int32, TArray<uint8>> ExtensionCellPayloads; // not reflected: UHT cannot nest TArray in TMap
    UPROPERTY(SaveGame, BlueprintReadWrite, Category="Metadata") FChunkSimulationMetadata Metadata;

    int32 GetCellIndex(int32 X, int32 Y, int32 CellsPerAxis) const;
    FChunkCell* GetCell(int32 X, int32 Y, int32 CellsPerAxis);
    const FChunkCell* GetCellConst(int32 X, int32 Y, int32 CellsPerAxis) const;
    UFUNCTION(BlueprintPure, Category="Cell") bool ReadCell(int32 X, int32 Y, int32 CellsPerAxis, FChunkCell& OutCell) const;
    UFUNCTION(BlueprintCallable, Category="Cell") bool WriteCell(int32 X, int32 Y, int32 CellsPerAxis, const FChunkCell& InCell);
    UFUNCTION(BlueprintCallable, Category="Init") void Initialize(int32 CellsPerAxis);
    UFUNCTION(BlueprintCallable, Category="Dirty") void MarkDirty() { Metadata.bHasPendingEvents = true; }
    UFUNCTION(BlueprintCallable, Category="Dirty") void ClearDirty() { Metadata.bHasPendingEvents = false; }
};

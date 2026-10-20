#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WorldTypes.h"
#include "ChunkState.generated.h"

USTRUCT(BlueprintType)
struct FChunkCell
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FTerrainCell Terrain;
    UPROPERTY(BlueprintReadWrite) FWaterCell Water;
    UPROPERTY(BlueprintReadWrite) FChemistryCell Chemistry;
    UPROPERTY(BlueprintReadWrite) FBiomassCell Biomass;
    UPROPERTY(BlueprintReadWrite) float SoilMoisture = 0.3f;
    UPROPERTY(BlueprintReadWrite) FSoilHydrologyState Hydrology;
    UPROPERTY(BlueprintReadWrite) float SoilNitrogen = 50.0f;
    UPROPERTY(BlueprintReadWrite) float SoilPhosphorus = 30.0f;
    UPROPERTY(BlueprintReadWrite) float SoilPotassium = 40.0f;
    UPROPERTY(BlueprintReadWrite) float SoilPH = 6.5f;
    UPROPERTY(BlueprintReadWrite) bool bIsDirty = false;
    UPROPERTY(BlueprintReadWrite) bool bHasWater = false;
    UPROPERTY(BlueprintReadWrite) bool bHasFire = false;
    UPROPERTY(BlueprintReadWrite) bool bHasToxin = false;
};
USTRUCT(BlueprintType)
struct FChunkSimulationMetadata
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) float LastUpdateDay = 1.0f;
    UPROPERTY(BlueprintReadWrite) float LastWeatherUpdateDay = 1.0f;
    UPROPERTY(BlueprintReadWrite) double LastSimulationTime = 0.0;
    UPROPERTY(BlueprintReadWrite) float AccumulatedDeltaDays = 0.0f;
    UPROPERTY(BlueprintReadWrite) EChunkActivationMode ActivationMode = EChunkActivationMode::Passive;
    UPROPERTY(BlueprintReadWrite) bool bHasPendingEvents = false;
    UPROPERTY(BlueprintReadWrite) bool bIsInitialized = false;
    UPROPERTY(BlueprintReadWrite) int32 ActiveEventCount = 0;
};
UCLASS(BlueprintType)
class MYPROJECT2_API UChunkState : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category="Identity") FIntPoint Coordinate = FIntPoint::ZeroValue;
    UPROPERTY(BlueprintReadWrite, Category="Cells") TArray<FChunkCell> Cells;
    // Sparse per-cell extension payloads (e.g. MaterialInstanceState binary snapshots).
    TMap<int32, TArray<uint8>> ExtensionCellPayloads; // not reflected: UHT cannot nest TArray in TMap
    UPROPERTY(BlueprintReadWrite, Category="Metadata") FChunkSimulationMetadata Metadata;

    int32 GetCellIndex(int32 X, int32 Y, int32 CellsPerAxis) const;
    FChunkCell* GetCell(int32 X, int32 Y, int32 CellsPerAxis);
    const FChunkCell* GetCellConst(int32 X, int32 Y, int32 CellsPerAxis) const;
    UFUNCTION(BlueprintPure, Category="Cell") bool ReadCell(int32 X, int32 Y, int32 CellsPerAxis, FChunkCell& OutCell) const;
    UFUNCTION(BlueprintCallable, Category="Cell") bool WriteCell(int32 X, int32 Y, int32 CellsPerAxis, const FChunkCell& InCell);
    UFUNCTION(BlueprintCallable, Category="Init") void Initialize(int32 CellsPerAxis);
    UFUNCTION(BlueprintCallable, Category="Dirty") void MarkDirty() { Metadata.bHasPendingEvents = true; }
    UFUNCTION(BlueprintCallable, Category="Dirty") void ClearDirty() { Metadata.bHasPendingEvents = false; }
};

#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WorldTypes.h"
#include "ChunkSerializer.generated.h"
class UChunkState;
USTRUCT(BlueprintType)
struct FSerializedCell
{
    GENERATED_BODY()
    UPROPERTY() float TerrainHeight=0.0f;
    UPROPERTY() float TerrainOriginalHeight=0.0f;
    UPROPERTY() float TerrainDeformation=0.0f;
    UPROPERTY() float TerrainPermeability=0.5f;
    UPROPERTY() FName TerrainMaterialID=NAME_None;
    UPROPERTY() float WaterVolume=0.0f;
    UPROPERTY() float WaterLevel=0.0f;
    UPROPERTY() float WaterFlowRate=0.0f;
    UPROPERTY() FVector2D WaterFlowDirection=FVector2D::ZeroVector;
    UPROPERTY() bool bWaterSource=false;
    UPROPERTY() bool bWaterFlowing=false;
    UPROPERTY() float SoilMoisture=0.3f;
    UPROPERTY() float SoilN=50.0f;
    UPROPERTY() float SoilP=30.0f;
    UPROPERTY() float SoilK=40.0f;
    UPROPERTY() float SoilPH=6.5f;
    UPROPERTY() float Toxin=0.0f;
    UPROPERTY() float Radioactivity=0.0f;
    UPROPERTY() float ChemistryPH=7.0f;
    UPROPERTY() float Pollution=0.0f;
    UPROPERTY() float Biomass=0.0f;
    UPROPERTY() float BiomassHealth=1.0f;
    UPROPERTY() float BiomassMoisture=0.5f;
    UPROPERTY() float BiomassDecayRate=0.02f;
    UPROPERTY() FName SpeciesID=NAME_None;
    UPROPERTY() bool bBurning=false;
};
USTRUCT(BlueprintType)
struct FSerializedChunk
{
    GENERATED_BODY()
    UPROPERTY() FIntPoint Coordinate=FIntPoint::ZeroValue;
    UPROPERTY() int32 CellsPerAxis=16;
    UPROPERTY() TMap<int32,FSerializedCell> Cells;
    TMap<int32,TArray<uint8>> ExtensionCellPayloads; // not reflected: UHT cannot nest TArray in TMap
    UPROPERTY() float LastUpdateDay=1.0f;
    UPROPERTY() float LastWeatherUpdateDay=1.0f;
    UPROPERTY() double LastSimulationTime=0.0;
    UPROPERTY() bool bIsInitialized=false;
    UPROPERTY() EChunkActivationMode ActivationMode=EChunkActivationMode::Passive;
};
UCLASS()
class MYPROJECT2_API UChunkSerializer : public UObject
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable,Category="Serialize") static FSerializedChunk SerializeChunk(UChunkState* Chunk);
    UFUNCTION(BlueprintCallable,Category="Serialize") static UChunkState* DeserializeChunk(UObject* Outer,const FSerializedChunk& Data);
};

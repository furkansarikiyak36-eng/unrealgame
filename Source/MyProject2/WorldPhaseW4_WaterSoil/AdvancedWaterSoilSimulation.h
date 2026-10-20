#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WorldCore/WorldTypes.h"
#include "AdvancedWaterSoilSimulation.generated.h"

class UChunkState;
struct FChunkCell;

USTRUCT(BlueprintType)
struct FHydrologyStepReport
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) float TotalWaterBeforeMm = 0.f;
    UPROPERTY(BlueprintReadOnly) float TotalWaterAfterMm = 0.f;
    UPROPERTY(BlueprintReadOnly) float InfiltrationMm = 0.f;
    UPROPERTY(BlueprintReadOnly) float EvapotranspirationMm = 0.f;
    UPROPERTY(BlueprintReadOnly) float SnowmeltMm = 0.f;
    UPROPERTY(BlueprintReadOnly) float VerticalDrainageMm = 0.f;
    UPROPERTY(BlueprintReadOnly) float SurfaceFlowMm = 0.f;
    UPROPERTY(BlueprintReadOnly) float WaterBalanceErrorMm = 0.f;
    UPROPERTY(BlueprintReadOnly) float MeanRootZoneSaturation = 0.f;
    UPROPERTY(BlueprintReadOnly) float MeanSurfaceWaterMm = 0.f;
};

UCLASS(BlueprintType)
class MYPROJECT2_API UAdvancedWaterSoilSimulation : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Geometry", meta=(ClampMin="0.1")) float CellSizeMeters = 4.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Soil|Layers", meta=(ClampMin="1")) float TopLayerFieldCapacityMm = 35.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Soil|Layers", meta=(ClampMin="0")) float TopLayerWiltingMm = 4.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Soil|Layers", meta=(ClampMin="1")) float RootLayerFieldCapacityMm = 120.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Soil|Layers", meta=(ClampMin="0")) float RootLayerWiltingMm = 18.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Soil|Layers", meta=(ClampMin="1")) float DeepLayerFieldCapacityMm = 220.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Soil|Hydraulics", meta=(ClampMin="0")) float BaseInfiltrationMmPerDay = 28.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Soil|Hydraulics", meta=(ClampMin="0")) float VerticalPercolationMmPerDay = 16.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water", meta=(ClampMin="0")) float SnowMeltMmPerDegreeDay = 3.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water", meta=(ClampMin="0")) float MaximumSnowMeltMmPerDay = 35.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water", meta=(ClampMin="0")) float BaseEvaporationMmPerDay = 2.4f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water", meta=(ClampMin="0")) float SurfaceFlowCoefficientPerDay = 0.65f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water", meta=(ClampMin="0", ClampMax="1")) float MaxSurfaceTransferFraction = 0.35f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Erosion", meta=(ClampMin="0")) float ErosionCoefficient = 0.012f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Erosion", meta=(ClampMin="0", ClampMax="1")) float VegetationProtection = 0.8f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nutrients", meta=(ClampMin="0", ClampMax="1")) float NitrogenLeachingFraction = 0.08f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nutrients", meta=(ClampMin="0", ClampMax="1")) float PhosphorusLeachingFraction = 0.015f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nutrients", meta=(ClampMin="0", ClampMax="1")) float PotassiumLeachingFraction = 0.04f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Thermal", meta=(ClampMin="0", ClampMax="1")) float SoilThermalResponsePerDay = 0.18f;

    UFUNCTION(BlueprintCallable, Category="Hydrology") void ApplyPrecipitation(UChunkState* Chunk, float PrecipitationMm, bool bSnow, float AirTemperatureC);
    UFUNCTION(BlueprintCallable, Category="Hydrology") FHydrologyStepReport SimulateChunk(UChunkState* Chunk, float DeltaDays, float AirTemperatureC, float AirHumidity01 = 0.55f);
    UFUNCTION(BlueprintCallable, Category="Hydrology") void InitializeCellHydrology(FChunkCell& Cell) const;
    UFUNCTION(BlueprintPure, Category="Hydrology") float GetCellTotalWaterMm(const FChunkCell& Cell) const;
private:
    float GetCellSurfaceWaterMm(const FChunkCell& Cell) const;
    float GetInfiltrationFactor(const FChunkCell& Cell) const;
    float GetSlopeFactor(const UChunkState* Chunk, int32 Index, int32 Axis) const;
    float GetTextureCapacityMultiplier(EWorldSoilTexture Texture) const;
    float GetTextureConductivityMultiplier(EWorldSoilTexture Texture) const;
    float GetTopFieldCapacity(const FChunkCell& Cell) const;
    float GetRootFieldCapacity(const FChunkCell& Cell) const;
    float GetDeepFieldCapacity(const FChunkCell& Cell) const;
};
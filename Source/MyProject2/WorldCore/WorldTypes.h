#pragma once
#include "CoreMinimal.h"
#include "WorldTypes.generated.h"

UENUM(BlueprintType)
enum class EWorldLayerType : uint8 { Terrain, Water, Soil, Biomass, Chemistry, Objects, Metadata };
UENUM(BlueprintType)
enum class EChunkActivationMode : uint8 { Passive, Scheduled, Continuous };
UENUM(BlueprintType)
enum class EWorldEventType : uint8 { None, CanalWallBroken, WaterLevelChanged, SoilMoistureChanged, FireStarted, FireSpread, FireExtinguished, DecayTick, ToxinSpread, TerrainDeformed, BuildingDamaged, NeighborActivated, Erosion };
UENUM(BlueprintType)
enum class EWorldSoilTexture : uint8 { Sand, LoamySand, Loam, SiltLoam, ClayLoam, Clay, OrganicPeat, Rock };
UENUM(BlueprintType)
enum class EWorldGroundTreatment : uint8 { Natural, Disturbed, Tilled, RaisedBed, Trench, Pit, WaterCatchment, TrapPrepared };
UENUM(BlueprintType)
enum class EBuildingDamageType : uint8 { None, Impact, Fire, Decay, Erosion, Toxin };

USTRUCT(BlueprintType)
struct FWorldTime
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) int32 Day = 1;
    UPROPERTY(BlueprintReadWrite) int32 Hour = 0;
    UPROPERTY(BlueprintReadWrite) float TimeOfDay = 0.0f;
    float ToTotalDays() const { return (float)Day + FMath::Clamp(TimeOfDay, 0.0f, 24.0f) / 24.0f; }
    static FWorldTime FromTotalDays(float Days)
    {
        FWorldTime T; const float Safe = FMath::Max(1.0f, Days);
        T.Day = FMath::FloorToInt(Safe); T.TimeOfDay = (Safe - T.Day) * 24.0f;
        T.Hour = FMath::Clamp(FMath::FloorToInt(T.TimeOfDay), 0, 23); return T;
    }
};
USTRUCT(BlueprintType)
struct FWaterCell
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) float Volume = 0.0f;
    UPROPERTY(BlueprintReadWrite) float Level = 0.0f;
    UPROPERTY(BlueprintReadWrite) float FlowRate = 0.0f;
    UPROPERTY(BlueprintReadWrite) FVector2D FlowDirection = FVector2D::ZeroVector;
    UPROPERTY(BlueprintReadWrite) bool bIsSource = false;
    UPROPERTY(BlueprintReadWrite) bool bIsFlowing = false;
};
USTRUCT(BlueprintType)
struct FTerrainCell
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) float Height = 0.0f;
    UPROPERTY(BlueprintReadWrite) float OriginalHeight = 0.0f;
    UPROPERTY(BlueprintReadWrite) float Deformation = 0.0f;
    UPROPERTY(BlueprintReadWrite) float Permeability = 0.5f;
    UPROPERTY(BlueprintReadWrite) FName MaterialID = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Soil") EWorldSoilTexture SoilTexture = EWorldSoilTexture::Loam;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Soil", meta=(ClampMin="0", ClampMax="1")) float Compaction = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Soil", meta=(ClampMin="0", ClampMax="0.3")) float OrganicMatterFraction = 0.03f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Soil", meta=(ClampMin="0.1")) float BedrockDepthMeters = 3.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Soil", meta=(ClampMin="0.05", ClampMax="3")) float RootingDepthMeters = 0.6f;
    // Remaining soil profile thickness. Digging reduces this while increasing ExcavatedDepthMeters.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Soil|Digging", meta=(ClampMin="0")) float SoilDepthMeters = 1.5f;
    // Net cut below the cell's original surface; bounds refilling and bedrock access.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Soil|Digging", meta=(ClampMin="0")) float ExcavatedDepthMeters = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Soil|Digging") EWorldGroundTreatment GroundTreatment = EWorldGroundTreatment::Natural;
};
USTRUCT(BlueprintType)
struct FSoilHydrologyState
{
    GENERATED_BODY()
    // Water storage is water-equivalent depth in millimetres per cell area.
    UPROPERTY(BlueprintReadWrite) float TopLayerWaterMm = 0.0f;
    UPROPERTY(BlueprintReadWrite) float RootLayerWaterMm = 0.0f;
    UPROPERTY(BlueprintReadWrite) float DeepLayerWaterMm = 0.0f;
    UPROPERTY(BlueprintReadWrite) float SnowpackWaterEquivalentMm = 0.0f;
    UPROPERTY(BlueprintReadWrite) float GroundwaterWaterEquivalentMm = 0.0f;
    UPROPERTY(BlueprintReadWrite) float SoilTemperatureC = 12.0f;
    UPROPERTY(BlueprintReadWrite) float SurfaceRunoffMm = 0.0f;
    UPROPERTY(BlueprintReadWrite) float CumulativeInfiltrationMm = 0.0f;
    UPROPERTY(BlueprintReadWrite) float CumulativeEvapotranspirationMm = 0.0f;
    UPROPERTY(BlueprintReadWrite) float CumulativeErosionIndex = 0.0f;
    UPROPERTY(BlueprintReadWrite) float SuspendedSedimentKg = 0.0f;
    UPROPERTY(BlueprintReadWrite) float LastWaterBalanceErrorMm = 0.0f;
    UPROPERTY(BlueprintReadWrite) bool bInitialized = false;
};
USTRUCT(BlueprintType)
struct FChemistryCell
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) float ToxinConcentration = 0.0f;
    UPROPERTY(BlueprintReadWrite) float Radioactivity = 0.0f;
    UPROPERTY(BlueprintReadWrite) float pH = 7.0f;
    UPROPERTY(BlueprintReadWrite) float Pollution = 0.0f;
};
USTRUCT(BlueprintType)
struct FBiomassCell
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) float Biomass = 0.0f;
    UPROPERTY(BlueprintReadWrite) float Health = 1.0f;
    UPROPERTY(BlueprintReadWrite) float MoistureContent = 0.5f;
    UPROPERTY(BlueprintReadWrite) float DecayRate = 0.02f;
    UPROPERTY(BlueprintReadWrite) FName SpeciesID = NAME_None;
    UPROPERTY(BlueprintReadWrite) bool bIsBurning = false;
};
USTRUCT(BlueprintType)
struct FWorldEvent
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) EWorldEventType Type = EWorldEventType::None;
    UPROPERTY(BlueprintReadWrite) FIntPoint ChunkCoord = FIntPoint::ZeroValue;
    UPROPERTY(BlueprintReadWrite) FVector WorldLocation = FVector::ZeroVector;
    UPROPERTY(BlueprintReadWrite) float AffectedRadius = 0.0f;
    UPROPERTY(BlueprintReadWrite) float Magnitude = 1.0f;
    UPROPERTY(BlueprintReadWrite) FName SourceTag = NAME_None;
    UPROPERTY(BlueprintReadWrite) double Timestamp = 0.0;
    UPROPERTY(BlueprintReadWrite) TMap<FName, float> NumericPayload;
    UPROPERTY(BlueprintReadWrite) TMap<FName, FVector> VectorPayload;
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChunkActivated, FIntPoint, ChunkCoord);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChunkDeactivated, FIntPoint, ChunkCoord);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldEventBroadcast, const FWorldEvent&, Event);
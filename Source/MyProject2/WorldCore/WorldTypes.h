#pragma once
#include "CoreMinimal.h"
#include "WorldTypes.generated.h"

UENUM(BlueprintType)
enum class EWorldLayerType : uint8 { Terrain, Water, Soil, Biomass, Chemistry, Objects, Metadata };
UENUM(BlueprintType)
enum class EChunkActivationMode : uint8 { Passive, Scheduled, Continuous };
UENUM(BlueprintType)
enum class EWorldEventType : uint8 { None, CanalWallBroken, WaterLevelChanged, SoilMoistureChanged, FireStarted, FireSpread, FireExtinguished, DecayTick, ToxinSpread, TerrainDeformed, BuildingDamaged, NeighborActivated };
UENUM(BlueprintType)
enum class EBuildingDamageType : uint8 { None, Impact, Fire, Decay, Erosion, Toxin };

USTRUCT(BlueprintType)
struct FWorldTime
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, BlueprintReadWrite) int32 Day = 1;
    UPROPERTY(SaveGame, BlueprintReadWrite) int32 Hour = 0;
    UPROPERTY(SaveGame, BlueprintReadWrite) float TimeOfDay = 0.0f;
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
    UPROPERTY(SaveGame, BlueprintReadWrite) float Volume = 0.0f;
    UPROPERTY(SaveGame, BlueprintReadWrite) float Level = 0.0f;
    UPROPERTY(SaveGame, BlueprintReadWrite) float FlowRate = 0.0f;
    UPROPERTY(SaveGame, BlueprintReadWrite) FVector2D FlowDirection = FVector2D::ZeroVector;
    UPROPERTY(SaveGame, BlueprintReadWrite) bool bIsSource = false;
    UPROPERTY(SaveGame, BlueprintReadWrite) bool bIsFlowing = false;
};
USTRUCT(BlueprintType)
struct FTerrainCell
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, BlueprintReadWrite) float Height = 0.0f;
    UPROPERTY(SaveGame, BlueprintReadWrite) float OriginalHeight = 0.0f;
    UPROPERTY(SaveGame, BlueprintReadWrite) float Deformation = 0.0f;
    UPROPERTY(SaveGame, BlueprintReadWrite) float Permeability = 0.5f;
    UPROPERTY(SaveGame, BlueprintReadWrite) FName MaterialID = NAME_None;
};
USTRUCT(BlueprintType)
struct FChemistryCell
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, BlueprintReadWrite) float ToxinConcentration = 0.0f;
    UPROPERTY(SaveGame, BlueprintReadWrite) float Radioactivity = 0.0f;
    UPROPERTY(SaveGame, BlueprintReadWrite) float pH = 7.0f;
    UPROPERTY(SaveGame, BlueprintReadWrite) float Pollution = 0.0f;
};
USTRUCT(BlueprintType)
struct FBiomassCell
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, BlueprintReadWrite) float Biomass = 0.0f;
    UPROPERTY(SaveGame, BlueprintReadWrite) float Health = 1.0f;
    UPROPERTY(SaveGame, BlueprintReadWrite) float MoistureContent = 0.5f;
    UPROPERTY(SaveGame, BlueprintReadWrite) float DecayRate = 0.02f;
    UPROPERTY(SaveGame, BlueprintReadWrite) FName SpeciesID = NAME_None;
    UPROPERTY(SaveGame, BlueprintReadWrite) bool bIsBurning = false;
};
USTRUCT(BlueprintType)
struct FWorldEvent
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, BlueprintReadWrite) EWorldEventType Type = EWorldEventType::None;
    UPROPERTY(SaveGame, BlueprintReadWrite) FIntPoint ChunkCoord = FIntPoint::ZeroValue;
    UPROPERTY(SaveGame, BlueprintReadWrite) FVector WorldLocation = FVector::ZeroVector;
    UPROPERTY(SaveGame, BlueprintReadWrite) float AffectedRadius = 0.0f;
    UPROPERTY(SaveGame, BlueprintReadWrite) float Magnitude = 1.0f;
    UPROPERTY(SaveGame, BlueprintReadWrite) FName SourceTag = NAME_None;
    UPROPERTY(SaveGame, BlueprintReadWrite) double Timestamp = 0.0;
    UPROPERTY(SaveGame, BlueprintReadWrite) TMap<FName, float> NumericPayload;
    UPROPERTY(SaveGame, BlueprintReadWrite) TMap<FName, FVector> VectorPayload;
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChunkActivated, FIntPoint, ChunkCoord);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChunkDeactivated, FIntPoint, ChunkCoord);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldEventBroadcast, const FWorldEvent&, Event);

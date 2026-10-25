#pragma once

#include "CoreMinimal.h"
#include "RegionLogisticsTypes.generated.h"

/** Infrastructure types share one route-cost contract; actors remain owners of their physical behavior. */
UENUM(BlueprintType)
enum class ELogisticsLinkType : uint8
{
    Trail,
    Road,
    Bridge,
    Ramp,
    SledRoute,
    Ropeway
};

USTRUCT(BlueprintType)
struct MYPROJECT2_API FRegionLogisticsNode
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Logistics")
    FGuid NodeId;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Logistics")
    FName StableKey = NAME_None;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Logistics")
    FVector WorldLocation = FVector::ZeroVector;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Logistics")
    FName NodeTag = NAME_None;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Logistics")
    bool bEnabled = true;
};

USTRUCT(BlueprintType)
struct MYPROJECT2_API FRegionLogisticsLink
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Logistics")
    FGuid LinkId;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Logistics")
    FName StableKey = NAME_None;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Logistics")
    FGuid FromNodeId;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Logistics")
    FGuid ToNodeId;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Logistics")
    ELogisticsLinkType Type = ELogisticsLinkType::Trail;

    /** Maximum payload supported by this link, in kilograms. */
    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Logistics")
    float MaxPayloadKg = 250.0f;

    /** Structural/maintenance condition in the inclusive range [0,1]. */
    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Logistics")
    float Condition01 = 1.0f;

    /** Explicit local hazard multiplier in the inclusive range [0,1]. */
    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Logistics")
    float Risk01 = 0.0f;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Logistics")
    bool bBidirectional = true;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Logistics")
    bool bOperational = true;
};

/** Save-boundary data only. URegionLogisticsSubsystem remains the live owner. */
USTRUCT(BlueprintType)
struct MYPROJECT2_API FRegionLogisticsSnapshot
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Logistics|Save")
    int32 SchemaVersion = 1;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Logistics|Save")
    int64 Revision = 0;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Logistics|Save")
    TArray<FRegionLogisticsNode> Nodes;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Logistics|Save")
    TArray<FRegionLogisticsLink> Links;
};

/** A read-only projection over canonical world cells; this is intentionally not another resource store. */
USTRUCT(BlueprintType)
struct MYPROJECT2_API FRegionSurvey
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Region")
    FIntPoint RegionCoord = FIntPoint::ZeroValue;

    UPROPERTY(BlueprintReadOnly, Category="Region")
    int32 ChunksSurveyed = 0;

    UPROPERTY(BlueprintReadOnly, Category="Region")
    int32 CellsSurveyed = 0;

    UPROPERTY(BlueprintReadOnly, Category="Region")
    float AvailableBiomassEstimate = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Region")
    float SoilNutrientEstimate = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Region")
    float StandingWaterVolume = 0.0f;

    /** Derived from existing fire, toxin and standing-water cell state, in [0,1]. */
    UPROPERTY(BlueprintReadOnly, Category="Region")
    float EnvironmentalRisk01 = 0.0f;
};

USTRUCT(BlueprintType)
struct MYPROJECT2_API FRegionRouteResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Logistics|Route")
    bool bFound = false;

    UPROPERTY(BlueprintReadOnly, Category="Logistics|Route")
    TArray<FGuid> NodeIds;

    UPROPERTY(BlueprintReadOnly, Category="Logistics|Route")
    TArray<FGuid> LinkIds;

    UPROPERTY(BlueprintReadOnly, Category="Logistics|Route")
    float TotalDistanceMeters = 0.0f;

    /** Estimated traversal time. Includes link type, condition, payload, and sampled regional risk. */
    UPROPERTY(BlueprintReadOnly, Category="Logistics|Route")
    float EstimatedTravelSeconds = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Logistics|Route")
    float MaximumRisk01 = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Logistics|Route")
    float BottleneckCapacityKg = 0.0f;
};

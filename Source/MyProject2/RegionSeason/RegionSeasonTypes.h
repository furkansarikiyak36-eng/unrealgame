#pragma once
#include "CoreMinimal.h"
#include "RegionSeasonTypes.generated.h"

UENUM(BlueprintType)
enum class EWorldSeason : uint8 { Spring, Summer, Autumn, Winter };

UENUM(BlueprintType)
enum class EWorldRegionType : uint8 { Unknown, Forest, Grassland, Wetland, Mountain, Desert, Coast, Tundra, Farmland, Settlement };

USTRUCT(BlueprintType)
struct FRegionSeasonModifier
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Season") EWorldSeason Season = EWorldSeason::Spring;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Season", meta=(ClampMin="-50", ClampMax="50")) float TemperatureOffsetC = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Season", meta=(ClampMin="0", ClampMax="5")) float PrecipitationMultiplier = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Season", meta=(ClampMin="0", ClampMax="5")) float WindMultiplier = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Season", meta=(ClampMin="-30", ClampMax="30")) float SnowThresholdOffsetC = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Season", meta=(ClampMin="0", ClampMax="24")) float DaylightHours = 12.0f;
};

USTRUCT(BlueprintType)
struct FRegionClimateSnapshot
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Region") bool bHasKnownRegion = false;
    UPROPERTY(BlueprintReadOnly, Category="Region") FName RegionId = NAME_None;
    UPROPERTY(BlueprintReadOnly, Category="Region") FText RegionDisplayName;
    UPROPERTY(BlueprintReadOnly, Category="Region") EWorldRegionType RegionType = EWorldRegionType::Unknown;
    UPROPERTY(BlueprintReadOnly, Category="Season") EWorldSeason Season = EWorldSeason::Spring;
    UPROPERTY(BlueprintReadOnly, Category="Climate") float BaselineTemperatureC = 15.0f;
    UPROPERTY(BlueprintReadOnly, Category="Climate") float TemperatureOffsetC = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="Climate") float PrecipitationMultiplier = 1.0f;
    UPROPERTY(BlueprintReadOnly, Category="Climate") float WindMultiplier = 1.0f;
    UPROPERTY(BlueprintReadOnly, Category="Climate") float SnowThresholdOffsetC = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="Climate") float DaylightHours = 12.0f;
    UPROPERTY(BlueprintReadOnly, Category="Climate") float ElevationMeters = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="Climate") float MeanAnnualRainfallMm = 600.0f;
};

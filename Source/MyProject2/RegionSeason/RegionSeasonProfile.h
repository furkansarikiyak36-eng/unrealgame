#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RegionSeasonTypes.h"
#include "RegionSeasonProfile.generated.h"

/** Data-driven baseline climate and seasonal modifiers for a region archetype. */
UCLASS(BlueprintType)
class MYPROJECT2_API URegionSeasonProfile : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity") FName RegionId = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity") FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity") EWorldRegionType RegionType = EWorldRegionType::Unknown;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Climate", meta=(ClampMin="-50", ClampMax="60")) float BaselineTemperatureC = 15.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Climate", meta=(ClampMin="-500", ClampMax="9000")) float ElevationMeters = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Climate", meta=(ClampMin="0", ClampMax="10000")) float MeanAnnualRainfallMm = 600.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Climate", meta=(ClampMin="0", ClampMax="10")) float BaselineWindMultiplier = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Season", meta=(TitleProperty="Season")) TArray<FRegionSeasonModifier> SeasonalModifiers;
    UFUNCTION(BlueprintPure, Category="Region|Climate") FRegionSeasonModifier GetModifierForSeason(EWorldSeason Season) const;
};

#pragma once

#include "CoreMinimal.h"
#include "WorldCore/ExcavationBaseDensitySource.h"
#include "ExcavationFlatBaseDensitySource.generated.h"

/**
 * Safe prototype fallback surface: solid below SurfaceHeightCm, empty above.
 * A registered project terrain provider takes precedence over this fallback.
 */
UCLASS(BlueprintType, Blueprintable)
class MYPROJECT2_API UExcavationFlatBaseDensitySource : public UObject, public IExcavationBaseDensitySource
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation|Base Surface")
    float SurfaceHeightCm = 0.0f;

    /** Soil above the bedrock, cm; the bedrock top waves by SoilDepthVariationCm over tens of metres. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation|Base Surface", meta = (ClampMin = "0"))
    float SoilDepthCm = 250.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation|Base Surface", meta = (ClampMin = "0"))
    float SoilDepthVariationCm = 80.0f;
    /** Boulders inside the soil: 3D noise above this is rock (1 = none). The top 40 cm stay clear. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation|Base Surface", meta = (ClampMin = "0", ClampMax = "1"))
    float BoulderThreshold = 0.55f;
    /** Material ID of bedrock and boulders (UExcavationVolumeSubsystem::UndiggableMaterialIds). 0 is soil. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation|Base Surface")
    int32 RockMaterialId = 2;

    virtual float SampleBaseDensityAtWorld_Implementation(const FVector& WorldPositionCm) const override;
    virtual int32 SampleBaseMaterialIdAtWorld_Implementation(const FVector& WorldPositionCm) const override;
};

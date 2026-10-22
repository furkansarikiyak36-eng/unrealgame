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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation|Base Surface", meta = (ClampMin = "1.0"))
    float DensityTransitionWidthCm = 25.0f;

    virtual float SampleBaseDensityAtWorld_Implementation(const FVector& WorldPositionCm) const override;
    virtual int32 SampleBaseMaterialIdAtWorld_Implementation(const FVector& WorldPositionCm) const override;
};

#include "ExcavationFlatBaseDensitySource.h"

float UExcavationFlatBaseDensitySource::SampleBaseDensityAtWorld_Implementation(
    const FVector& WorldPositionCm) const
{
    if (!FMath::IsFinite(WorldPositionCm.Z) || !FMath::IsFinite(SurfaceHeightCm))
    {
        // Invalid density is deliberately rejected by the authoritative sampler.
        return -1.0f;
    }

    return WorldPositionCm.Z <= SurfaceHeightCm ? 1.0f : 0.0f;
}

int32 UExcavationFlatBaseDensitySource::SampleBaseMaterialIdAtWorld_Implementation(
    const FVector& WorldPositionCm) const
{
    return 0;
}

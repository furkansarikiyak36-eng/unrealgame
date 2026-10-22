#include "ExcavationFlatBaseDensitySource.h"

float UExcavationFlatBaseDensitySource::SampleBaseDensityAtWorld_Implementation(
    const FVector& WorldPositionCm) const
{
    if (!FMath::IsFinite(WorldPositionCm.X)
        || !FMath::IsFinite(WorldPositionCm.Y)
        || !FMath::IsFinite(WorldPositionCm.Z)
        || !FMath::IsFinite(SurfaceHeightCm)
        || !FMath::IsFinite(DensityTransitionWidthCm)
        || DensityTransitionWidthCm < 1.0f)
    {
        // Invalid density is deliberately rejected by the authoritative sampler.
        return -1.0f;
    }

    return FMath::Clamp(
        0.5f + (SurfaceHeightCm - WorldPositionCm.Z) / DensityTransitionWidthCm,
        0.0f,
        1.0f);
}

int32 UExcavationFlatBaseDensitySource::SampleBaseMaterialIdAtWorld_Implementation(
    const FVector& WorldPositionCm) const
{
    return 0;
}

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
    if (!FMath::IsFinite(WorldPositionCm.X) || !FMath::IsFinite(WorldPositionCm.Y) || !FMath::IsFinite(WorldPositionCm.Z))
    {
        return 0;
    }
    const float Depth = SurfaceHeightCm - static_cast<float>(WorldPositionCm.Z);
    const float BedrockTop = SoilDepthCm
        + SoilDepthVariationCm * FMath::PerlinNoise2D(FVector2D(WorldPositionCm.X, WorldPositionCm.Y) / 2500.0);
    if (Depth >= BedrockTop)
    {
        return RockMaterialId;
    }
    if (Depth > 40.0f && BoulderThreshold < 1.0f
        && FMath::PerlinNoise3D(FVector(WorldPositionCm) / 260.0 + FVector(17.3, 4.1, 9.7)) > BoulderThreshold)
    {
        return RockMaterialId;
    }
    return 0;
}

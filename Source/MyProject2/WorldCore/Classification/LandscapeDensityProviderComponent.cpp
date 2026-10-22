#include "LandscapeDensityProviderComponent.h"

#include "Engine/World.h"
#include "LandscapeProxy.h"
#include "WorldCore/ExcavationVolumeSubsystem.h"

ULandscapeDensityProviderComponent::ULandscapeDensityProviderComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void ULandscapeDensityProviderComponent::BeginPlay()
{
    Super::BeginPlay();
    if (UWorld* World = GetWorld())
    {
        if (UExcavationVolumeSubsystem* Volume = World->GetSubsystem<UExcavationVolumeSubsystem>())
        {
            Volume->RegisterBaseDensitySource(this);
        }
    }
}

void ULandscapeDensityProviderComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        if (UExcavationVolumeSubsystem* Volume = World->GetSubsystem<UExcavationVolumeSubsystem>())
        {
            Volume->UnregisterBaseDensitySource(this);
        }
    }
    HeightCache.Reset();
    Super::EndPlay(EndPlayReason);
}

void ULandscapeDensityProviderComponent::ClearHeightCache()
{
    HeightCache.Reset();
}

bool ULandscapeDensityProviderComponent::TryGetLandscapeHeight(float Xcm, float Ycm, float& OutHeightCm) const
{
    const float SafeCellSize = FMath::Max(1.0f, CacheCellSizeCm);
    const FIntPoint Key(FMath::FloorToInt(Xcm / SafeCellSize), FMath::FloorToInt(Ycm / SafeCellSize));
    if (const float* Cached = HeightCache.Find(Key))
    {
        if (*Cached <= TNumericLimits<float>::Lowest() * 0.5f)
        {
            return false;
        }
        OutHeightCm = *Cached;
        return true;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    const FVector CachePoint((static_cast<float>(Key.X) + 0.5f) * SafeCellSize,
        (static_cast<float>(Key.Y) + 0.5f) * SafeCellSize, 0.0f);
    const FVector Start = FVector(CachePoint.X, CachePoint.Y, TraceHalfHeightCm);
    const FVector End = FVector(CachePoint.X, CachePoint.Y, -TraceHalfHeightCm);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(LandscapeDensityProvider), true);

    FHitResult Hit;
    const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, LandscapeTraceChannel, Params);
    const ALandscapeProxy* Landscape = bHit ? Cast<ALandscapeProxy>(Hit.GetActor()) : nullptr;
    const float StoredHeight = Landscape ? Hit.ImpactPoint.Z : TNumericLimits<float>::Lowest();
    if (HeightCache.Num() > 100000)
    {
        HeightCache.Reset();
    }
    HeightCache.Add(Key, StoredHeight);
    if (!Landscape)
    {
        return false;
    }
    OutHeightCm = StoredHeight;
    return true;
}

float ULandscapeDensityProviderComponent::SampleBaseDensityAtWorld_Implementation(const FVector& WorldPositionCm) const
{
    float SurfaceHeightCm = 0.0f;
    if (!TryGetLandscapeHeight(WorldPositionCm.X, WorldPositionCm.Y, SurfaceHeightCm))
    {
        return 0.0f;
    }

    const float Blend = FMath::Max(1.0f, BlendHalfThicknessCm);
    return FMath::Clamp(0.5f - (WorldPositionCm.Z - SurfaceHeightCm) / (2.0f * Blend), 0.0f, 1.0f);
}

int32 ULandscapeDensityProviderComponent::SampleBaseMaterialIdAtWorld_Implementation(const FVector& WorldPositionCm) const
{
    return 0;
}

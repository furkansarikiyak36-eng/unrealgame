#include "ExcavationLandscapeDensitySource.h"

#include "LandscapeProxy.h"
#include "LandscapeHeightfieldCollisionComponent.h"

bool UExcavationLandscapeDensitySource::SetLandscapeProxies(const TArray<ALandscapeProxy*>& InProxies)
{
    TArray<TWeakObjectPtr<ALandscapeProxy>> Candidate;
    Candidate.Reserve(InProxies.Num());

    for (ALandscapeProxy* Proxy : InProxies)
    {
        if (IsValid(Proxy) && !Candidate.Contains(TWeakObjectPtr<ALandscapeProxy>(Proxy)))
        {
            Candidate.Add(Proxy);
        }
    }

    bool bChanged = Candidate.Num() != LandscapeProxies.Num();
    if (!bChanged)
    {
        for (int32 Index = 0; Index < Candidate.Num(); ++Index)
        {
            if (Candidate[Index].Get() != LandscapeProxies[Index].Get())
            {
                bChanged = true;
                break;
            }
        }
    }

    if (bChanged)
    {
        LandscapeProxies = MoveTemp(Candidate);
    }
    return bChanged;
}

float UExcavationLandscapeDensitySource::SampleBaseDensityAtWorld_Implementation(
    const FVector& WorldPositionCm) const
{
    if (!FMath::IsFinite(WorldPositionCm.X)
        || !FMath::IsFinite(WorldPositionCm.Y)
        || !FMath::IsFinite(WorldPositionCm.Z))
    {
        return -1.0f;
    }

    // Streaming landscapes can be split into proxies; query each loaded proxy until one owns this XY.
    for (const TWeakObjectPtr<ALandscapeProxy>& WeakProxy : LandscapeProxies)
    {
        const ALandscapeProxy* Proxy = WeakProxy.Get();
        if (!IsValid(Proxy))
        {
            continue;
        }

        const TOptional<float> SurfaceHeight =
            Proxy->GetHeightAtLocation(WorldPositionCm, EHeightfieldSource::Simple);
        if (SurfaceHeight.IsSet() && FMath::IsFinite(SurfaceHeight.GetValue()))
        {
            return WorldPositionCm.Z <= SurfaceHeight.GetValue() ? 1.0f : 0.0f;
        }
    }

    // Outside all loaded landscape proxies is empty space; never invent a solid wall at proxy boundaries.
    return 0.0f;
}

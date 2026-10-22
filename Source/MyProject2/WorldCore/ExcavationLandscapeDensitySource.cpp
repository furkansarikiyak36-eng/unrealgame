#include "ExcavationLandscapeDensitySource.h"

#include "LandscapeProxy.h"
#include "LandscapeHeightfieldCollisionComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

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

    Candidate.Sort([](const TWeakObjectPtr<ALandscapeProxy>& A, const TWeakObjectPtr<ALandscapeProxy>& B)
    {
        const ALandscapeProxy* ProxyA = A.Get();
        const ALandscapeProxy* ProxyB = B.Get();
        const FString NameA = IsValid(ProxyA) ? ProxyA->GetPathName() : FString();
        const FString NameB = IsValid(ProxyB) ? ProxyB->GetPathName() : FString();
        return NameA < NameB;
    });

    if (bChanged)
    {
        LandscapeProxies = MoveTemp(Candidate);
        SurfaceHeightCache.Reset();
    }
    return bChanged;
}

bool UExcavationLandscapeDensitySource::SetMaterialMappings(
    const TArray<FExcavationLandscapeMaterialMapping>& InMappings)
{
    TArray<FExcavationLandscapeMaterialMapping> Candidate;
    for (const FExcavationLandscapeMaterialMapping& Mapping : InMappings)
    {
        if (!IsValid(Mapping.PhysicalMaterial)
            || Mapping.MaterialId < 1 || Mapping.MaterialId > 31)
        {
            continue;
        }

        bool bDuplicateId = false;
        for (const FExcavationLandscapeMaterialMapping& Existing : Candidate)
        {
            if (Existing.MaterialId == Mapping.MaterialId)
            {
                bDuplicateId = true;
                break;
            }
        }

        if (!bDuplicateId)
        {
            Candidate.Add(Mapping);
        }
    }

    bool bChanged = Candidate.Num() != MaterialMappings.Num();
    if (!bChanged)
    {
        for (int32 Index = 0; Index < Candidate.Num(); ++Index)
        {
            if (Candidate[Index].PhysicalMaterial != MaterialMappings[Index].PhysicalMaterial
                || Candidate[Index].MaterialId != MaterialMappings[Index].MaterialId)
            {
                bChanged = true;
                break;
            }
        }
    }

    if (bChanged)
    {
        MaterialMappings = MoveTemp(Candidate);
    }
    return bChanged;
}

bool UExcavationLandscapeDensitySource::SetDensityTransitionWidthCm(float NewWidthCm)
{
    if (!FMath::IsFinite(NewWidthCm) || NewWidthCm < 1.0f)
    {
        return false;
    }

    if (!FMath::IsNearlyEqual(DensityTransitionWidthCm, NewWidthCm, 0.001f))
    {
        DensityTransitionWidthCm = NewWidthCm;
    }
    return true;
}

bool UExcavationLandscapeDensitySource::TryGetSurfaceHeight(
    float WorldXcm,
    float WorldYcm,
    float& OutHeightCm) const
{
    if (!FMath::IsFinite(WorldXcm) || !FMath::IsFinite(WorldYcm))
    {
        return false;
    }

    // Sample grid positions are spaced by at least 12.5 cm in the HIGH_END profile,
    // so integer-centimetre keys preserve distinct voxel columns while avoiding a
    // repeated heightfield query for every Z sample in the same column.
    const double KeyX = FMath::RoundToDouble(WorldXcm);
    const double KeyY = FMath::RoundToDouble(WorldYcm);
    if (KeyX < TNumericLimits<int32>::Lowest() || KeyX > TNumericLimits<int32>::Max()
        || KeyY < TNumericLimits<int32>::Lowest() || KeyY > TNumericLimits<int32>::Max())
    {
        return false;
    }

    const FIntPoint Key(static_cast<int32>(KeyX), static_cast<int32>(KeyY));
    if (const float* Cached = SurfaceHeightCache.Find(Key))
    {
        if (*Cached == TNumericLimits<float>::Lowest())
        {
            return false;
        }
        OutHeightCm = *Cached;
        return true;
    }

    const FVector QueryPosition(WorldXcm, WorldYcm, 0.0f);
    for (const TWeakObjectPtr<ALandscapeProxy>& WeakProxy : LandscapeProxies)
    {
        const ALandscapeProxy* Proxy = WeakProxy.Get();
        if (!IsValid(Proxy))
        {
            continue;
        }

        const TOptional<float> SurfaceHeight =
            Proxy->GetHeightAtLocation(QueryPosition, EHeightfieldSource::Simple);
        if (SurfaceHeight.IsSet() && FMath::IsFinite(SurfaceHeight.GetValue()))
        {
            const float Height = SurfaceHeight.GetValue();
            if (SurfaceHeightCache.Num() >= 262144)
            {
                SurfaceHeightCache.Reset();
            }
            SurfaceHeightCache.Add(Key, Height);
            OutHeightCm = Height;
            return true;
        }
    }

    if (SurfaceHeightCache.Num() >= 262144)
    {
        SurfaceHeightCache.Reset();
    }
    SurfaceHeightCache.Add(Key, TNumericLimits<float>::Lowest());
    return false;
}

float UExcavationLandscapeDensitySource::SampleBaseDensityAtWorld_Implementation(
    const FVector& WorldPositionCm) const
{
    if (!FMath::IsFinite(WorldPositionCm.X)
        || !FMath::IsFinite(WorldPositionCm.Y)
        || !FMath::IsFinite(WorldPositionCm.Z)
        || !FMath::IsFinite(DensityTransitionWidthCm)
        || DensityTransitionWidthCm < 1.0f)
    {
        return -1.0f;
    }

    float SurfaceHeightCm = 0.0f;
    if (!TryGetSurfaceHeight(WorldPositionCm.X, WorldPositionCm.Y, SurfaceHeightCm))
    {
        // Outside all loaded landscape proxies is empty space; do not invent a solid wall.
        return 0.0f;
    }

    // Density 0.5 is the isosurface. A one-cell-wide continuous ramp allows the
    // marcher to interpolate the Landscape height instead of snapping it to cell centres.
    return FMath::Clamp(
        0.5f + (SurfaceHeightCm - WorldPositionCm.Z) / DensityTransitionWidthCm,
        0.0f,
        1.0f);
}

int32 UExcavationLandscapeDensitySource::SampleBaseMaterialIdAtWorld_Implementation(
    const FVector& WorldPositionCm) const
{
    if (!FMath::IsFinite(WorldPositionCm.X)
        || !FMath::IsFinite(WorldPositionCm.Y)
        || !FMath::IsFinite(WorldPositionCm.Z))
    {
        return -1;
    }

    for (const TWeakObjectPtr<ALandscapeProxy>& WeakProxy : LandscapeProxies)
    {
        const ALandscapeProxy* Proxy = WeakProxy.Get();
        if (!IsValid(Proxy))
        {
            continue;
        }

        float SurfaceHeightCm = 0.0f;
        if (!TryGetSurfaceHeight(WorldPositionCm.X, WorldPositionCm.Y, SurfaceHeightCm))
        {
            continue;
        }

        const UPhysicalMaterial* PhysicalMaterial =
            Proxy->GetPhysicalMaterialAtLocation(WorldPositionCm, EHeightfieldSource::Simple);
        if (!IsValid(PhysicalMaterial))
        {
            return 0;
        }

        for (const FExcavationLandscapeMaterialMapping& Mapping : MaterialMappings)
        {
            if (Mapping.PhysicalMaterial == PhysicalMaterial)
            {
                return Mapping.MaterialId;
            }
        }

        return 0;
    }

    return 0;
}

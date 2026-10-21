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

    if (bChanged)
    {
        LandscapeProxies = MoveTemp(Candidate);
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

        const TOptional<float> SurfaceHeight =
            Proxy->GetHeightAtLocation(WorldPositionCm, EHeightfieldSource::Simple);
        if (!SurfaceHeight.IsSet())
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

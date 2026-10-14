#pragma once

#include "CoreMinimal.h"
#include "WorldCore/ExcavationBaseDensitySource.h"
#include "ExcavationLandscapeDensitySource.generated.h"

class ALandscapeProxy;

/**
 * Samples loaded Landscape/streaming-proxy heightfields as the unmodified base
 * world. Runtime sparse excavation overrides are layered on top by the owner.
 */
UCLASS(BlueprintType)
class MYPROJECT2_API UExcavationLandscapeDensitySource : public UObject, public IExcavationBaseDensitySource
{
    GENERATED_BODY()

public:
    /** Returns true when the set of Landscape proxies changed. */
    bool SetLandscapeProxies(const TArray<ALandscapeProxy*>& InProxies);

    virtual float SampleBaseDensityAtWorld_Implementation(const FVector& WorldPositionCm) const override;
    virtual int32 SampleBaseMaterialIdAtWorld_Implementation(const FVector& WorldPositionCm) const override;

private:
    TArray<TWeakObjectPtr<ALandscapeProxy>> LandscapeProxies;
};

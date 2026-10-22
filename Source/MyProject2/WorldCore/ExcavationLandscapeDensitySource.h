#pragma once

#include "CoreMinimal.h"
#include "WorldCore/ExcavationBaseDensitySource.h"
#include "ExcavationLandscapeDensitySource.generated.h"

class ALandscapeProxy;
class UPhysicalMaterial;

USTRUCT(BlueprintType)
struct MYPROJECT2_API FExcavationLandscapeMaterialMapping
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation|Landscape Materials")
    TObjectPtr<UPhysicalMaterial> PhysicalMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation|Landscape Materials", meta = (ClampMin = "1", ClampMax = "31"))
    int32 MaterialId = 1;
};

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
    bool SetMaterialMappings(const TArray<FExcavationLandscapeMaterialMapping>& InMappings);
    bool SetDensityTransitionWidthCm(float NewWidthCm);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation|Density", meta = (ClampMin = "1.0"))
    float DensityTransitionWidthCm = 25.0f;

    virtual float SampleBaseDensityAtWorld_Implementation(const FVector& WorldPositionCm) const override;
    virtual int32 SampleBaseMaterialIdAtWorld_Implementation(const FVector& WorldPositionCm) const override;

private:
    bool TryGetSurfaceHeight(float WorldXcm, float WorldYcm, float& OutHeightCm) const;

    TArray<TWeakObjectPtr<ALandscapeProxy>> LandscapeProxies;
    mutable TMap<FIntPoint, float> SurfaceHeightCache;

    UPROPERTY(Transient)
    TArray<FExcavationLandscapeMaterialMapping> MaterialMappings;
};

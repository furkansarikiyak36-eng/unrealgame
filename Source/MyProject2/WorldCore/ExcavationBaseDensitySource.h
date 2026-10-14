#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ExcavationBaseDensitySource.generated.h"

/**
 * Implemented by the terrain/world layer that owns the unmodified base surface.
 * Density is normalized: 0 = empty space, 1 = solid material.
 *
 * Excavation overrides are applied after this query. This interface lets a
 * heightfield or procedural terrain provide its baseline without owning or
 * duplicating the sparse 3D excavation delta.
 */
UINTERFACE(BlueprintType)
class MYPROJECT2_API UExcavationBaseDensitySource : public UInterface
{
    GENERATED_BODY()
};

class MYPROJECT2_API IExcavationBaseDensitySource
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Excavation|Base Density")
    float SampleBaseDensityAtWorld(const FVector& WorldPositionCm) const;
};

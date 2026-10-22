#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ExcavationBaseDensitySource.h"
#include "LandscapeDensityProviderComponent.generated.h"

/**
 * Heightfield adapter for a Landscape actor. It samples the existing Landscape collision
 * once per XY cache cell; the voxel density then uses this height as a smooth base field.
 * Attach it to the primary Landscape actor. Static meshes which block the trace are not
 * silently treated as editable landscape.
 */
UCLASS(ClassGroup = (World), meta = (BlueprintSpawnableComponent))
class MYPROJECT2_API ULandscapeDensityProviderComponent
    : public UActorComponent, public IExcavationBaseDensitySource
{
    GENERATED_BODY()

public:
    ULandscapeDensityProviderComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Base Density", meta = (ClampMin = "1.0"))
    float BlendHalfThicknessCm = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Base Density", meta = (ClampMin = "1.0"))
    float CacheCellSizeCm = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Base Density", meta = (ClampMin = "100.0"))
    float TraceHalfHeightCm = 200000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Base Density")
    TEnumAsByte<ECollisionChannel> LandscapeTraceChannel = ECC_Visibility;

    UFUNCTION(BlueprintCallable, Category = "Voxel|Base Density")
    void ClearHeightCache();

    virtual float SampleBaseDensityAtWorld_Implementation(const FVector& WorldPositionCm) const override;
    virtual int32 SampleBaseMaterialIdAtWorld_Implementation(const FVector& WorldPositionCm) const override;

private:
    mutable TMap<FIntPoint, float> HeightCache;
    bool TryGetLandscapeHeight(float Xcm, float Ycm, float& OutHeightCm) const;
};

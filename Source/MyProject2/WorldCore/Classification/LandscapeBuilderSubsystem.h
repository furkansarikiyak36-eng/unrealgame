#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldClassificationTypes.h"
#include "LandscapeBuilderSubsystem.generated.h"

class AActor;

/**
 * Builds a bounds-based first-pass map for editable voxel regions and protected static geometry.
 * It is demand-driven/editor-invoked, not a whole-world per-frame scan.
 */
UCLASS()
class MYPROJECT2_API ULandscapeBuilderSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "World|Landscape Builder")
    int32 BuildClassificationSnapshot();

    UFUNCTION(BlueprintCallable, Category = "World|Landscape Builder")
    bool RegisterRegionOverride(FVector CenterCm, FVector ExtentCm, EWorldTerrainMode Mode,
        int32 Priority, FName SourceLabel);

    UFUNCTION(BlueprintCallable, Category = "World|Landscape Builder")
    void ClearRegionOverrides();

    UFUNCTION(BlueprintPure, Category = "World|Landscape Builder")
    FWorldClassificationResult QueryModeAtWorld(FVector WorldPositionCm) const;

    UFUNCTION(BlueprintPure, Category = "World|Landscape Builder")
    int32 GetRegionCount() const { return Regions.Num(); }

    UFUNCTION(BlueprintPure, Category = "World|Landscape Builder")
    bool IsVoxelEditableAtWorld(FVector WorldPositionCm) const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World|Landscape Builder")
    bool bTreatLandscapeAsEditableByDefault = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World|Landscape Builder", meta = (ClampMin = "0.0"))
    float LandscapeMarginBelowCm = 50000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World|Landscape Builder", meta = (ClampMin = "0.0"))
    float LandscapeMarginAboveCm = 1000.0f;

    UPROPERTY(BlueprintReadOnly, Category = "World|Landscape Builder")
    int32 LastScannedActorCount = 0;

private:
    struct FRegion
    {
        FBox Bounds = FBox(ForceInit);
        EWorldTerrainMode Mode = EWorldTerrainMode::Unknown;
        int32 Priority = 0;
        bool bExplicit = false;
        FName SourceLabel = NAME_None;
    };

    TArray<FRegion> Regions;

    void AddActorRegion(AActor* Actor, const FBox& Bounds, EWorldTerrainMode Mode,
        int32 Priority, bool bExplicit, FName SourceLabel);
    static bool ActorHasTerrainTag(const AActor* Actor, FName Tag);
};

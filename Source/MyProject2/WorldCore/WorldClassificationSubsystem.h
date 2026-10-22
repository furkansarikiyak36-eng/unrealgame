// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldCore/WorldSystemContracts.h"
#include "WorldClassificationSubsystem.generated.h"

struct FWorldManualRegionOverride
{
    FName Id = NAME_None;
    FBox Bounds = FBox(ForceInit);
    EWorldRegionKind Kind = EWorldRegionKind::Unknown;
    FString Reason;
};

/**
 * World-local, deterministic coarse classification map.
 *
 * Classification order:
 * manual AABB override > actor/component landscape-layer marker > actor/component region tag
 * > bounded center-surface probe > Unknown.
 *
 * Landscape-layer markers are explicit component/actor tags in this phase. This
 * class does not read painted Landscape weightmap textures or alter Landscape data.
 */
UCLASS()
class MYPROJECT2_API UWorldClassificationSubsystem final
    : public UWorldSubsystem
    , public IWorldClassificationSource
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    virtual bool BuildClassification(
        const FWorldClassificationBuildRequest& Request,
        FWorldClassificationBuildResult& OutResult) override;

    virtual bool QueryClassification(
        const FVector& WorldPositionCm,
        FWorldRegionClassification& OutResult) const override;

    /** Manually classify a world-space region. Overrides are applied before automatic tags. */
    bool SetManualOverride(FName OverrideId, const FBox& Bounds, EWorldRegionKind Kind, const FString& Reason = FString());

    /** Removes one explicit override. The built map must be rebuilt before it can be queried again. */
    bool RemoveManualOverride(FName OverrideId);

    void ClearManualOverrides();
    void InvalidateClassification();

    uint64 GetClassificationRevision() const { return ClassificationRevision; }
    bool HasBuiltClassification() const { return bHasBuiltClassification; }
    FIntVector GetCachedGridDimensions() const { return CachedGridDimensions; }

    /** Parses stable region/layer tag names. Returns false for unrelated tags. */
    static bool TryClassifyRegionTag(
        FName Tag,
        EWorldRegionKind& OutKind,
        EWorldClassificationSource& OutSource);

private:
    struct FCachedClassificationCell
    {
        FWorldRegionClassification Classification;
        int32 Priority = INDEX_NONE;
        FString StableKey;
        bool bPriorityConflict = false;
    };

    bool IsValidRegionKind(EWorldRegionKind Kind) const;
    FBox MakeCellBounds(const FIntVector& Coordinate) const;
    bool GetCellCoordinate(const FVector& WorldPositionCm, FIntVector& OutCoordinate) const;

    TArray<FWorldManualRegionOverride> ManualOverrides;
    TMap<FIntVector, FCachedClassificationCell> CachedRegions;

    FBox CachedBuildBounds = FBox(ForceInit);
    FVector CachedRegionSizeCm = FVector::ZeroVector;
    FIntVector CachedGridDimensions = FIntVector::ZeroValue;
    uint64 ClassificationRevision = 0;
    bool bHasBuiltClassification = false;
};

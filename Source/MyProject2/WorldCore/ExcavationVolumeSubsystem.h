#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ExcavationTypes.h"
#include "ExcavationVolumeSubsystem.generated.h"

/**
 * Sparse 3D override storage for local excavation edits.
 *
 * This subsystem is the authoritative data layer for edits recorded through
 * this API. Base terrain sampling, dynamic mesh generation, collision rebuilds,
 * navigation and save-game serialization are intentionally separate adapters.
 */
UCLASS()
class MYPROJECT2_API UExcavationVolumeSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Deinitialize() override;

    /**
     * Apply an all-or-nothing spherical carve/fill against cell centers.
     * The resulting receipt can roll the edit back if no later edit has
     * overwritten any of its changed cells.
     */
    UFUNCTION(BlueprintCallable, Category = "Excavation")
    bool ApplySphereEdit(
        const FVector& CenterCm,
        float RadiusCm,
        EExcavationEditType EditType,
        FExcavationEditRecord& OutRecord);

    /** Apply a rounded-capsule carve/fill. Useful for continuous horizontal or sloped tunnel segments. */
    UFUNCTION(BlueprintCallable, Category = "Excavation")
    bool ApplyCapsuleEdit(
        const FVector& StartCm,
        const FVector& EndCm,
        float RadiusCm,
        EExcavationEditType EditType,
        FExcavationEditRecord& OutRecord);

    /** Apply an axis-aligned box carve/fill. Useful for rooms, shafts and rectangular chambers. */
    UFUNCTION(BlueprintCallable, Category = "Excavation")
    bool ApplyBoxEdit(
        const FVector& MinWorldCm,
        const FVector& MaxWorldCm,
        EExcavationEditType EditType,
        FExcavationEditRecord& OutRecord);

    /** Roll back a previous edit only when every changed cell is still owned by that edit. */
    UFUNCTION(BlueprintCallable, Category = "Excavation")
    bool RollbackEdit(const FExcavationEditRecord& Record);

    /** Returns true only when the cell has an explicit sparse override. */
    UFUNCTION(BlueprintPure, Category = "Excavation")
    bool TryGetDensityOverrideAtWorld(const FVector& WorldPositionCm, float& OutDensity) const;

    UFUNCTION(BlueprintPure, Category = "Excavation")
    bool TryGetDensityOverrideAtCell(const FIntVector& Cell, float& OutDensity) const;

    /** Sample final density: sparse override first, then registered base terrain, then explicit fallback. */
    UFUNCTION(BlueprintPure, Category = "Excavation")
    bool TrySampleDensityAtWorld(const FVector& WorldPositionCm, float& OutDensity) const;

    /** Register an object implementing UExcavationBaseDensitySource. Returns false for invalid providers. */
    UFUNCTION(BlueprintCallable, Category = "Excavation")
    bool RegisterBaseDensitySource(UObject* Source);

    /** Only the currently registered source (or null) may clear the base density provider. */
    UFUNCTION(BlueprintCallable, Category = "Excavation")
    void UnregisterBaseDensitySource(UObject* Source);

    /** Export a stable, versioned snapshot for the persistence adapter. */
    UFUNCTION(BlueprintCallable, Category = "Excavation|Persistence")
    bool ExportSnapshot(FExcavationVolumeSnapshot& OutSnapshot) const;

    /** Validate a snapshot fully before replacing live sparse data. Existing data is protected by default. */
    UFUNCTION(BlueprintCallable, Category = "Excavation|Persistence")
    bool ImportSnapshot(const FExcavationVolumeSnapshot& Snapshot, bool bReplaceExistingState = false);

    /** Use caller-sampled base terrain density when this sparse overlay has no entry. */
    UFUNCTION(BlueprintPure, Category = "Excavation")
    bool ResolveDensityAtCell(const FIntVector& Cell, float BaseDensity, float& OutDensity) const;

    UFUNCTION(BlueprintPure, Category = "Excavation")
    bool IsCellOverridden(const FIntVector& Cell) const;

    UFUNCTION(BlueprintPure, Category = "Excavation")
    int32 GetOverrideCellCount() const { return CellOverrides.Num(); }

    /** Chunk coordinate for a global voxel-cell coordinate; uses floor division for negatives. */
    UFUNCTION(BlueprintPure, Category = "Excavation")
    bool TryGetChunkCoordinateForCell(const FIntVector& Cell, FIntVector& OutChunkCoordinate) const;

    /** Returns the explicitly overridden cells belonging to a chunk in stable sorted order. */
    UFUNCTION(BlueprintPure, Category = "Excavation")
    TArray<FIntVector> GetOverrideCellsInChunk(const FIntVector& ChunkCoordinate) const;

    /** Includes neighboring chunks touched at a one-cell mesh sampling border. */
    UFUNCTION(BlueprintPure, Category = "Excavation")
    TArray<FIntVector> GetAffectedChunksForEdit(const FExcavationEditRecord& Record) const;

    UFUNCTION(BlueprintPure, Category = "Excavation")
    bool TryWorldToCell(const FVector& WorldPositionCm, FIntVector& OutCell) const;

    UFUNCTION(BlueprintPure, Category = "Excavation")
    FVector CellToWorldCenter(const FIntVector& Cell) const;

    /** Edge length of a cell in Unreal centimeters. It is locked while overrides exist. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation|Config", meta = (ClampMin = "1.0"))
    float CellSizeCm = 50.0f;

    /** Safety cap for the candidate bounding volume of a single edit. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation|Config", meta = (ClampMin = "1"))
    int32 MaxCellsPerEdit = 250000;

    /** Used only when no base-density source is registered; 1 means solid. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation|Config", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DefaultBaseDensity = 1.0f;

    /** Number of voxel cells along each chunk axis; minimum 4 supports border sampling. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation|Config", meta = (ClampMin = "4"))
    int32 CellsPerChunkAxis = 32;

    /** Maximum sparse cell overrides accepted by the versioned snapshot importer/exporter. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation|Persistence", meta = (ClampMin = "1"))
    int32 MaxSnapshotOverrideCells = 2000000;

protected:
    /** Captures the grid size used by live overrides so changing config cannot remap them. */
    float LockedCellSizeCm = 0.0f;
    int32 LockedCellsPerChunkAxis = 0;
    bool IsGridConfigurationCompatible() const;
    /** Shared atomic implementation used by all supported geometric edit shapes. */
    bool ApplyBoundedEdit(
        const FVector& MinBoundsCm,
        const FVector& MaxBoundsCm,
        EExcavationShapeType ShapeType,
        const FVector& CenterCm,
        const FVector& EndCm,
        const FVector& HalfExtentCm,
        float RadiusCm,
        EExcavationEditType EditType,
        FExcavationEditRecord& OutRecord);


    /** Derived index for localized mesh/collision updates; rebuilt from CellOverrides after restore. */
    TMap<FIntVector, TSet<FIntVector>> CellsByChunk;

    /** Object implementing UExcavationBaseDensitySource; not an authoritative owner of excavation deltas. */
    UPROPERTY(Transient)
    TObjectPtr<UObject> BaseDensitySourceObject;

    /** Sparse explicit overrides only; missing cells are sampled from the base-world provider. */
    UPROPERTY(Transient)
    TMap<FIntVector, FExcavationCellOverride> CellOverrides;
};

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

    /** Roll back a previous edit only when every changed cell is still owned by that edit. */
    UFUNCTION(BlueprintCallable, Category = "Excavation")
    bool RollbackEdit(const FExcavationEditRecord& Record);

    /** Returns true only when the cell has an explicit sparse override. */
    UFUNCTION(BlueprintPure, Category = "Excavation")
    bool TryGetDensityOverrideAtWorld(const FVector& WorldPositionCm, float& OutDensity) const;

    UFUNCTION(BlueprintPure, Category = "Excavation")
    bool TryGetDensityOverrideAtCell(const FIntVector& Cell, float& OutDensity) const;

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

    /** Number of voxel cells along each chunk axis; minimum 4 supports border sampling. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation|Config", meta = (ClampMin = "4"))
    int32 CellsPerChunkAxis = 32;

protected:
    /** Captures the grid size used by live overrides so changing config cannot remap them. */
    float LockedCellSizeCm = 0.0f;
    int32 LockedCellsPerChunkAxis = 0;

    /** Derived index for localized mesh/collision updates; rebuilt from CellOverrides after restore. */
    TMap<FIntVector, TSet<FIntVector>> CellsByChunk;

    /** Sparse explicit overrides only; missing cells are sampled from the base-world provider. */
    UPROPERTY(Transient)
    TMap<FIntVector, FExcavationCellOverride> CellOverrides;
};

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ExcavationTypes.h"
#include "WorldSystemContracts.h"
#include "ExcavationVolumeSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnExcavationVolumeChanged,
    const FExcavationEditRecord&, EditRecord,
    bool, bIsRollback);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnExcavationVolumeReset);

/**
 * Sparse 3D override storage for local excavation edits.
 *
 * This subsystem is the authoritative data layer for edits recorded through
 * this API. Rendering/collision adapters consume this state but never own a
 * second copy of excavation density.
 */
UCLASS()
class MYPROJECT2_API UExcavationVolumeSubsystem : public UWorldSubsystem, public IVoxelDensitySource
{
    GENERATED_BODY()

public:
    virtual void Deinitialize() override;

    UPROPERTY(BlueprintAssignable, Category = "Excavation|Events")
    FOnExcavationVolumeChanged OnVolumeChanged;

    UPROPERTY(BlueprintAssignable, Category = "Excavation|Events")
    FOnExcavationVolumeReset OnVolumeReset;

    UFUNCTION(BlueprintCallable, Category = "Excavation")
    bool ApplySphereEdit(const FVector& CenterCm, float RadiusCm, EExcavationEditType EditType, FExcavationEditRecord& OutRecord);

    UFUNCTION(BlueprintCallable, Category = "Excavation")
    bool ApplyCapsuleEdit(const FVector& StartCm, const FVector& EndCm, float RadiusCm, EExcavationEditType EditType, FExcavationEditRecord& OutRecord);

    UFUNCTION(BlueprintCallable, Category = "Excavation")
    bool ApplyBoxEdit(const FVector& MinWorldCm, const FVector& MaxWorldCm, EExcavationEditType EditType, FExcavationEditRecord& OutRecord);

    UFUNCTION(BlueprintCallable, Category = "Excavation")
    bool RollbackEdit(const FExcavationEditRecord& Record);

    UFUNCTION(BlueprintPure, Category = "Excavation")
    bool TryGetDensityOverrideAtWorld(const FVector& WorldPositionCm, float& OutDensity) const;

    UFUNCTION(BlueprintPure, Category = "Excavation")
    bool TryGetDensityOverrideAtCell(const FIntVector& Cell, float& OutDensity) const;

    /** Sparse delta wins; otherwise active provider, fallback provider, then configured scalar fallback. */
    UFUNCTION(BlueprintPure, Category = "Excavation")
    bool TrySampleDensityAtWorld(const FVector& WorldPositionCm, float& OutDensity) const;

    /** Samples the unmodified terrain baseline without applying sparse excavation overrides. */
    UFUNCTION(BlueprintPure, Category = "Excavation")
    bool TrySampleBaseDensityAtWorld(const FVector& WorldPositionCm, float& OutDensity) const;

    /** Configure the density grid before the first edit. Existing overrides must already match. */
    bool ConfigureGridForRuntimeProfile(float InCellSizeCm, int32 InCellsPerChunkAxis, FString& OutError);

    /** Call after the registered base density source or its terrain coverage changes. */
    void NotifyBaseDensityChanged();

    virtual bool SampleBaseDensityAtWorld(const FVector& PositionCm, float& OutDensity) const override;
    virtual bool SampleDensityAtWorld(const FVector& PositionCm, float& OutDensity) const override;
    virtual bool BuildChunkSnapshot(
        const FIntVector& ChunkCoordinate,
        FWorldDensityChunkSnapshot& OutSnapshot) const override;

    uint64 GetDensityRevision() const { return DensityRevision; }

    /** Samples the base provider only (ignores excavation density overrides) for exposed-surface material selection. */
    UFUNCTION(BlueprintPure, Category = "Excavation|Materials")
    bool TrySampleBaseMaterialIdAtWorld(const FVector& WorldPositionCm, int32& OutMaterialId) const;

    /** Register an active provider (for example, a Blueprint implementing UExcavationBaseDensitySource). */
    UFUNCTION(BlueprintCallable, Category = "Excavation")
    bool RegisterBaseDensitySource(UObject* Source);

    /** Register a provider used only when no active world-terrain provider is registered. */
    UFUNCTION(BlueprintCallable, Category = "Excavation")
    bool RegisterFallbackBaseDensitySource(UObject* Source);

    /** Clears only the active provider; the fallback provider remains available. */
    UFUNCTION(BlueprintCallable, Category = "Excavation")
    void UnregisterBaseDensitySource(UObject* Source);

    UFUNCTION(BlueprintCallable, Category = "Excavation")
    void UnregisterFallbackBaseDensitySource(UObject* Source);

    UFUNCTION(BlueprintCallable, Category = "Excavation|Persistence")
    bool ExportSnapshot(FExcavationVolumeSnapshot& OutSnapshot) const;

    UFUNCTION(BlueprintCallable, Category = "Excavation|Persistence")
    bool ImportSnapshot(const FExcavationVolumeSnapshot& Snapshot, bool bReplaceExistingState = false);

    UFUNCTION(BlueprintPure, Category = "Excavation")
    bool ResolveDensityAtCell(const FIntVector& Cell, float BaseDensity, float& OutDensity) const;

    UFUNCTION(BlueprintPure, Category = "Excavation")
    bool IsCellOverridden(const FIntVector& Cell) const;

    UFUNCTION(BlueprintPure, Category = "Excavation")
    int32 GetOverrideCellCount() const { return CellOverrides.Num(); }

    UFUNCTION(BlueprintPure, Category = "Excavation")
    bool TryGetChunkCoordinateForCell(const FIntVector& Cell, FIntVector& OutChunkCoordinate) const;

    UFUNCTION(BlueprintPure, Category = "Excavation")
    TArray<FIntVector> GetOverrideCellsInChunk(const FIntVector& ChunkCoordinate) const;

    /** Includes neighboring chunks touched at a one-cell mesh sampling border. */
    UFUNCTION(BlueprintPure, Category = "Excavation")
    TArray<FIntVector> GetAffectedChunksForEdit(const FExcavationEditRecord& Record) const;

    UFUNCTION(BlueprintPure, Category = "Excavation")
    bool TryWorldToCell(const FVector& WorldPositionCm, FIntVector& OutCell) const;

    UFUNCTION(BlueprintPure, Category = "Excavation")
    FVector CellToWorldCenter(const FIntVector& Cell) const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation|Config", meta = (ClampMin = "1.0"))
    float CellSizeCm = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation|Config", meta = (ClampMin = "1"))
    int32 MaxCellsPerEdit = 250000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation|Config", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DefaultBaseDensity = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation|Config", meta = (ClampMin = "4"))
    int32 CellsPerChunkAxis = 32;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation|Persistence", meta = (ClampMin = "1"))
    int32 MaxSnapshotOverrideCells = 2000000;

protected:
    float LockedCellSizeCm = 0.0f;
    int32 LockedCellsPerChunkAxis = 0;
    bool IsGridConfigurationCompatible() const;
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

    TMap<FIntVector, TSet<FIntVector>> CellsByChunk;

    /** Changes after edits/import so consumers can discard out-of-date mesh work. */
    uint64 DensityRevision = 1;

    UPROPERTY(Transient)
    TObjectPtr<UObject> BaseDensitySourceObject;

    UPROPERTY(Transient)
    TObjectPtr<UObject> FallbackBaseDensitySourceObject;

    UPROPERTY(Transient)
    TMap<FIntVector, FExcavationCellOverride> CellOverrides;
};

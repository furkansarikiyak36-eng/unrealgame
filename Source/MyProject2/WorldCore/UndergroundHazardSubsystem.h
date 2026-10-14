#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldCore/UndergroundHazardTypes.h"
#include "WorldCore/WorldTypes.h"
#include "UndergroundHazardSubsystem.generated.h"

class AUndergroundSupportActor;
class UExcavationVolumeSubsystem;
class UWorldStateSubsystem;
class UChunkState;
struct FChunkCell;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUndergroundRoofCollapse, const FVector&, WorldLocationCm, int32, CellsCollapsed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUndergroundWaterSeepage, const FVector&, WorldLocationCm, float, VolumeAddedM3);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUndergroundSupportChanged, const FGuid&, SupportId, bool, bIsOperational);

/** Owns underground support actors and 3D water pockets; reuses canonical excavation, structure and surface-water state. */
UCLASS(Config = Game)
class MYPROJECT2_API UUndergroundHazardSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

    UPROPERTY(BlueprintAssignable, Category = "Underground|Events")
    FOnUndergroundRoofCollapse OnRoofCollapse;
    UPROPERTY(BlueprintAssignable, Category = "Underground|Events")
    FOnUndergroundWaterSeepage OnWaterSeepage;
    UPROPERTY(BlueprintAssignable, Category = "Underground|Events")
    FOnUndergroundSupportChanged OnSupportChanged;

    UFUNCTION(BlueprintCallable, Category = "Underground|Support")
    AUndergroundSupportActor* PlaceSupport(const FVector& LocationCm, const FRotator& Rotation, EUndergroundSupportType Type, const FGuid& SupportId);
    UFUNCTION(BlueprintPure, Category = "Underground|Support")
    bool HasOperationalSupportNear(const FVector& LocationCm, float RadiusCm) const;
    void RegisterSupportActor(AUndergroundSupportActor* Support);
    void UnregisterSupportActor(AUndergroundSupportActor* Support);
    void NotifySupportActorChanged(AUndergroundSupportActor* Support);

    UFUNCTION(BlueprintPure, Category = "Underground|Water")
    bool TryGetWaterPocketAtCell(const FIntVector& Cell, FUndergroundWaterPocket& OutPocket) const;
    UFUNCTION(BlueprintPure, Category = "Underground|Water")
    int32 GetUndergroundWaterPocketCount() const { return WaterPockets.Num(); }
    UFUNCTION(BlueprintPure, Category = "Underground|Stability")
    int32 GetPendingStabilityCellCount() const { return FMath::Max(0, PendingStabilityCells.Num() - StabilityQueueCursor); }

    UFUNCTION(BlueprintCallable, Category = "Underground|Persistence")
    bool ExportSnapshot(FUndergroundHazardSnapshot& OutSnapshot) const;
    UFUNCTION(BlueprintPure, Category = "Underground|Persistence")
    bool ValidateSnapshot(const FUndergroundHazardSnapshot& Snapshot) const;
    UFUNCTION(BlueprintCallable, Category = "Underground|Persistence")
    bool ImportSnapshot(const FUndergroundHazardSnapshot& Snapshot);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Underground|Support")
    TSubclassOf<AUndergroundSupportActor> SupportActorClass;

    UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Underground|Stability", meta = (ClampMin = "2", ClampMax = "32"))
    int32 MaxRoofSpanScanCells = 12;
    UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Underground|Stability", meta = (ClampMin = "2", ClampMax = "32"))
    int32 CriticalUnsupportedRoofSpanCells = 8;
    UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Underground|Stability", meta = (ClampMin = "1", ClampMax = "8"))
    int32 MaxRoofSearchHeightCells = 3;
    UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Underground|Stability", meta = (ClampMin = "0", ClampMax = "8"))
    int32 MaxCollapseCellsPerTick = 1;
    UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Underground|Stability", meta = (ClampMin = "1", ClampMax = "256"))
    int32 MaxStabilityChecksPerTick = 24;
    UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Underground|Stability", meta = (ClampMin = "1", ClampMax = "50000"))
    int32 MaxPendingStabilityCells = 20000;
    UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Underground|Support", meta = (ClampMin = "1.0"))
    float SupportInfluenceRadiusCm = 150.0f;

    UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Underground|Water", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SeepageSoilMoistureThreshold = 0.72f;
    UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Underground|Water", meta = (ClampMin = "0.0"))
    float SeepageMinSurfaceWaterM3 = 0.02f;
    UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Underground|Water", meta = (ClampMin = "0.0001", ClampMax = "1.0"))
    float MaxSeepagePerPocketTickM3 = 0.005f;
    UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Underground|Water", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SurfaceWaterTransferFraction = 0.1f;
    UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Underground|Water", meta = (ClampMin = "0.01", ClampMax = "10.0"))
    float HydrologyTickSeconds = 1.0f;
    UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Underground|Water", meta = (ClampMin = "0.0"))
    float MaxGravityTransferPerTickM3 = 0.02f;
    UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Underground|Water", meta = (ClampMin = "1", ClampMax = "50000"))
    int32 MaxTrackedOpenCaveCells = 20000;
    UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Underground|Water", meta = (ClampMin = "1", ClampMax = "1024"))
    int32 MaxHydrologyChecksPerTick = 32;
    UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Underground|Persistence", meta = (ClampMin = "1", ClampMax = "50000"))
    int32 MaxSnapshotSupports = 4096;
    UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Underground|Persistence", meta = (ClampMin = "1", ClampMax = "50000"))
    int32 MaxSnapshotWaterPockets = 20000;
    UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Underground|Persistence", meta = (ClampMin = "1", ClampMax = "50000"))
    int32 MaxSnapshotSurfaceCells = 20000;

protected:
    UFUNCTION()
    void HandleExcavationChanged(const FExcavationEditRecord& Record, bool bIsRollback);
    UFUNCTION()
    void HandleExcavationReset();

    void EnqueueStabilityCell(const FIntVector& Cell);
    void RefreshOpenCellTracking(const FIntVector& Cell);
    void SeedOpenCellTrackingFromSnapshot();
    bool ProcessStabilityCell(const FIntVector& Cell);
    bool TrySampleCellDensity(const FIntVector& Cell, float& OutDensity) const;
    int32 ComputeCavitySpan(const FIntVector& Cell) const;
    bool TryOffsetCell(const FIntVector& Cell, const FIntVector& Offset, FIntVector& OutCell) const;
    void ProcessHydrologyCell(const FIntVector& Cell);
    void SimulateUndergroundGravityFlow();
    bool FindWorldSurfaceCell(const FVector& CaveWorldPosition, FChunkCell*& OutCell, UChunkState*& OutChunk, int32& OutCellIndex, FIntPoint& OutGlobalCell) const;
    void BroadcastUndergroundWorldEvent(EWorldEventType Type, const FVector& LocationCm, float Magnitude, FName SourceTag);

    UPROPERTY(Transient)
    TMap<FGuid, TWeakObjectPtr<AUndergroundSupportActor>> RegisteredSupports;
    UPROPERTY(Transient)
    TMap<FIntVector, FUndergroundWaterPocket> WaterPockets;

    UPROPERTY(Transient)
    TMap<FIntPoint, FUndergroundSurfaceWaterCellSnapshot> ModifiedSurfaceCells;

    TSet<FIntPoint> SurfaceCellsProcessedThisTick;

    TArray<FIntVector> OpenCaveCells;
    TSet<FIntVector> OpenCaveCellSet;
    TArray<FIntVector> PendingStabilityCells;
    TSet<FIntVector> PendingStabilityCellSet;
    int32 StabilityQueueCursor = 0;
    int32 HydrologyCursor = 0;
    float HydrologyAccumulatorSeconds = 0.0f;
    bool bResolvingCollapse = false;
};

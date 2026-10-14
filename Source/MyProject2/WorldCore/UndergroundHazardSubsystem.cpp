#include "UndergroundHazardSubsystem.h"

#include "UndergroundSupportActor.h"
#include "WorldCore/ExcavationVolumeSubsystem.h"
#include "WorldCore/WorldStateSubsystem.h"
#include "WorldCore/ChunkState.h"
#include "WorldPhaseW3_Event/WorldEventSubsystem.h"
#include "StructuralIntegritySubsystem.h"
#include "Engine/World.h"

namespace
{
    bool IsFiniteVector3(const FVector& V)
    {
        return FMath::IsFinite(V.X) && FMath::IsFinite(V.Y) && FMath::IsFinite(V.Z);
    }

    bool IsValidSupportType(EUndergroundSupportType Type)
    {
        return Type == EUndergroundSupportType::TimberPost
            || Type == EUndergroundSupportType::TimberBrace
            || Type == EUndergroundSupportType::StonePillar
            || Type == EUndergroundSupportType::ReinforcedBeam;
    }

    bool IsLessCell(const FIntVector& A, const FIntVector& B)
    {
        if (A.X != B.X) return A.X < B.X;
        if (A.Y != B.Y) return A.Y < B.Y;
        return A.Z < B.Z;
    }
}

void UUndergroundHazardSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UExcavationVolumeSubsystem>();
    Collection.InitializeDependency<UWorldStateSubsystem>();

    if (!SupportActorClass)
    {
        SupportActorClass = AUndergroundSupportActor::StaticClass();
    }

    if (UWorld* World = GetWorld())
    {
        if (UExcavationVolumeSubsystem* Excavation = World->GetSubsystem<UExcavationVolumeSubsystem>())
        {
            Excavation->OnVolumeChanged.AddDynamic(this, &UUndergroundHazardSubsystem::HandleExcavationChanged);
            Excavation->OnVolumeReset.AddDynamic(this, &UUndergroundHazardSubsystem::HandleExcavationReset);
        }
    }

    SeedOpenCellTrackingFromSnapshot();
}

void UUndergroundHazardSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        if (UExcavationVolumeSubsystem* Excavation = World->GetSubsystem<UExcavationVolumeSubsystem>())
        {
            Excavation->OnVolumeChanged.RemoveDynamic(this, &UUndergroundHazardSubsystem::HandleExcavationChanged);
            Excavation->OnVolumeReset.RemoveDynamic(this, &UUndergroundHazardSubsystem::HandleExcavationReset);
        }
    }

    RegisteredSupports.Reset();
    WaterPockets.Reset();
    OpenCaveCells.Reset();
    OpenCaveCellSet.Reset();
    PendingStabilityCells.Reset();
    PendingStabilityCellSet.Reset();
    Super::Deinitialize();
}

TStatId UUndergroundHazardSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UUndergroundHazardSubsystem, STATGROUP_Tickables);
}

bool UUndergroundHazardSubsystem::IsTickable() const
{
    return !IsTemplate();
}

void UUndergroundHazardSubsystem::Tick(float DeltaTime)
{
    const int32 StabilityBudget = FMath::Clamp(MaxStabilityChecksPerTick, 1, 256);
    const int32 CollapseBudget = FMath::Clamp(MaxCollapseCellsPerTick, 0, 8);
    int32 Checks = 0;
    int32 Collapses = 0;

    while (StabilityQueueCursor < PendingStabilityCells.Num() && Checks < StabilityBudget)
    {
        const FIntVector Cell = PendingStabilityCells[StabilityQueueCursor++];
        PendingStabilityCellSet.Remove(Cell);
        ++Checks;
        if (ProcessStabilityCell(Cell))
        {
            ++Collapses;
            if (CollapseBudget > 0 && Collapses >= CollapseBudget)
            {
                break;
            }
        }
    }

    if (StabilityQueueCursor >= PendingStabilityCells.Num())
    {
        PendingStabilityCells.Reset();
        PendingStabilityCellSet.Reset();
        StabilityQueueCursor = 0;
    }
    else if (StabilityQueueCursor >= 256)
    {
        PendingStabilityCells.RemoveAt(0, StabilityQueueCursor, EAllowShrinking::No);
        StabilityQueueCursor = 0;
    }

    HydrologyAccumulatorSeconds += FMath::Max(0.0f, DeltaTime);
    const float Interval = FMath::Clamp(HydrologyTickSeconds, 0.01f, 10.0f);
    if (HydrologyAccumulatorSeconds < Interval)
    {
        return;
    }
    HydrologyAccumulatorSeconds = FMath::Fmod(HydrologyAccumulatorSeconds, Interval);

    const int32 HydroBudget = FMath::Clamp(MaxHydrologyChecksPerTick, 1, 1024);
    int32 HydroChecks = 0;
    while (!OpenCaveCells.IsEmpty() && HydroChecks < HydroBudget)
    {
        if (HydrologyCursor >= OpenCaveCells.Num())
        {
            HydrologyCursor = 0;
        }

        const FIntVector Cell = OpenCaveCells[HydrologyCursor++];
        if (!OpenCaveCellSet.Contains(Cell))
        {
            continue;
        }

        ++HydroChecks;
        ProcessHydrologyCell(Cell);
    }

    SimulateUndergroundGravityFlow();
}

void UUndergroundHazardSubsystem::RegisterSupportActor(AUndergroundSupportActor* Support)
{
    if (!IsValid(Support))
    {
        return;
    }

    FGuid Id = Support->GetSupportId();
    if (!Id.IsValid())
    {
        Id = FGuid::NewGuid();
        Support->ConfigureSupport(Id, Support->SupportType, Support->GetSupportIntegrity());
    }

    if (const TWeakObjectPtr<AUndergroundSupportActor>* Existing = RegisteredSupports.Find(Id))
    {
        if (Existing->IsValid() && Existing->Get() != Support)
        {
            Id = FGuid::NewGuid();
            Support->ConfigureSupport(Id, Support->SupportType, Support->GetSupportIntegrity());
        }
    }

    RegisteredSupports.Add(Id, Support);
    OnSupportChanged.Broadcast(Id, Support->IsSupportOperational());
}

void UUndergroundHazardSubsystem::UnregisterSupportActor(AUndergroundSupportActor* Support)
{
    if (!Support)
    {
        return;
    }

    const FGuid Id = Support->GetSupportId();
    if (const TWeakObjectPtr<AUndergroundSupportActor>* Existing = RegisteredSupports.Find(Id))
    {
        if (!Existing->IsValid() || Existing->Get() == Support)
        {
            RegisteredSupports.Remove(Id);
        }
    }
    OnSupportChanged.Broadcast(Id, false);
}

void UUndergroundHazardSubsystem::NotifySupportActorChanged(AUndergroundSupportActor* Support)
{
    if (IsValid(Support))
    {
        OnSupportChanged.Broadcast(Support->GetSupportId(), Support->IsSupportOperational());
    }
}

bool UUndergroundHazardSubsystem::HasOperationalSupportNear(const FVector& LocationCm, float RadiusCm) const
{
    if (!IsFiniteVector3(LocationCm) || !FMath::IsFinite(RadiusCm) || RadiusCm < 0.0f)
    {
        return false;
    }

    const double RadiusSquared = static_cast<double>(RadiusCm) * RadiusCm;
    for (const TPair<FGuid, TWeakObjectPtr<AUndergroundSupportActor>>& Pair : RegisteredSupports)
    {
        const AUndergroundSupportActor* Support = Pair.Value.Get();
        if (IsValid(Support) && Support->IsSupportOperational()
            && FVector::DistSquared(Support->GetActorLocation(), LocationCm) <= RadiusSquared)
        {
            return true;
        }
    }
    return false;
}

AUndergroundSupportActor* UUndergroundHazardSubsystem::PlaceSupport(
    const FVector& LocationCm, const FRotator& Rotation, EUndergroundSupportType Type, const FGuid& SupportId)
{
    if (!IsFiniteVector3(LocationCm)
        || !FMath::IsFinite(Rotation.Roll) || !FMath::IsFinite(Rotation.Pitch) || !FMath::IsFinite(Rotation.Yaw)
        || !SupportId.IsValid() || !IsValidSupportType(Type))
    {
        return nullptr;
    }

    if (const TWeakObjectPtr<AUndergroundSupportActor>* Existing = RegisteredSupports.Find(SupportId))
    {
        if (AUndergroundSupportActor* ExistingActor = Existing->Get())
        {
            return ExistingActor;
        }
    }

    UWorld* World = GetWorld();
    UExcavationVolumeSubsystem* Excavation = World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!World || !Excavation || !SupportActorClass)
    {
        return nullptr;
    }

    const float CellSize = FMath::Max(1.0f, Excavation->CellSizeCm);
    float BodyDensity = 1.0f;
    float HeadDensity = 1.0f;
    if (!Excavation->TrySampleDensityAtWorld(LocationCm + FVector(0.0, 0.0, CellSize), BodyDensity)
        || !Excavation->TrySampleDensityAtWorld(LocationCm + FVector(0.0, 0.0, CellSize * 2.0), HeadDensity)
        || BodyDensity >= 0.5f || HeadDensity >= 0.5f)
    {
        return nullptr;
    }

    AUndergroundSupportActor* Support = World->SpawnActorDeferred<AUndergroundSupportActor>(
        SupportActorClass.Get(), FTransform(Rotation, LocationCm), nullptr, nullptr,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Support)
    {
        return nullptr;
    }

    Support->ConfigureSupport(SupportId, Type, 100.0f);
    Support->FinishSpawning(FTransform(Rotation, LocationCm));
    RegisterSupportActor(Support);
    return Support;
}

bool UUndergroundHazardSubsystem::TryGetWaterPocketAtCell(
    const FIntVector& Cell, FUndergroundWaterPocket& OutPocket) const
{
    const FUndergroundWaterPocket* Pocket = WaterPockets.Find(Cell);
    if (!Pocket)
    {
        return false;
    }
    OutPocket = *Pocket;
    return true;
}

bool UUndergroundHazardSubsystem::TrySampleCellDensity(const FIntVector& Cell, float& OutDensity) const
{
    UWorld* World = GetWorld();
    UExcavationVolumeSubsystem* Excavation = World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    return Excavation && Excavation->TrySampleDensityAtWorld(Excavation->CellToWorldCenter(Cell), OutDensity);
}

bool UUndergroundHazardSubsystem::TryOffsetCell(
    const FIntVector& Cell, const FIntVector& Offset, FIntVector& OutCell) const
{
    const int64 X = static_cast<int64>(Cell.X) + Offset.X;
    const int64 Y = static_cast<int64>(Cell.Y) + Offset.Y;
    const int64 Z = static_cast<int64>(Cell.Z) + Offset.Z;
    if (X < TNumericLimits<int32>::Lowest() || X > TNumericLimits<int32>::Max()
        || Y < TNumericLimits<int32>::Lowest() || Y > TNumericLimits<int32>::Max()
        || Z < TNumericLimits<int32>::Lowest() || Z > TNumericLimits<int32>::Max())
    {
        return false;
    }
    OutCell = FIntVector(static_cast<int32>(X), static_cast<int32>(Y), static_cast<int32>(Z));
    return true;
}

int32 UUndergroundHazardSubsystem::ComputeCavitySpan(const FIntVector& Cell) const
{
    const int32 ScanLimit = FMath::Clamp(MaxRoofSpanScanCells, 2, 32);
    int32 BestSpan = 1;
    const FIntVector Axes[] = { FIntVector(1, 0, 0), FIntVector(0, 1, 0) };

    for (const FIntVector& Axis : Axes)
    {
        int32 Span = 1;
        for (const int32 Sign : { -1, 1 })
        {
            for (int32 Step = 1; Step < ScanLimit; ++Step)
            {
                FIntVector Probe;
                if (!TryOffsetCell(Cell, Axis * (Step * Sign), Probe))
                {
                    break;
                }
                float Density = 1.0f;
                if (!TrySampleCellDensity(Probe, Density) || Density >= 0.5f)
                {
                    break;
                }
                ++Span;
            }
        }
        BestSpan = FMath::Max(BestSpan, Span);
    }
    return BestSpan;
}

void UUndergroundHazardSubsystem::EnqueueStabilityCell(const FIntVector& Cell)
{
    if (PendingStabilityCellSet.Contains(Cell)
        || PendingStabilityCells.Num() >= FMath::Clamp(MaxPendingStabilityCells, 1, 50000))
    {
        return;
    }
    PendingStabilityCellSet.Add(Cell);
    PendingStabilityCells.Add(Cell);
}

void UUndergroundHazardSubsystem::RefreshOpenCellTracking(const FIntVector& Cell)
{
    float Density = 1.0f;
    if (!TrySampleCellDensity(Cell, Density))
    {
        return;
    }

    if (Density < 0.5f)
    {
        if (!OpenCaveCellSet.Contains(Cell)
            && OpenCaveCells.Num() < FMath::Clamp(MaxTrackedOpenCaveCells, 1, 50000))
        {
            OpenCaveCellSet.Add(Cell);
            OpenCaveCells.Add(Cell);
        }
        EnqueueStabilityCell(Cell);
    }
    else
    {
        OpenCaveCellSet.Remove(Cell);
        PendingStabilityCellSet.Remove(Cell);
    }
}

void UUndergroundHazardSubsystem::SeedOpenCellTrackingFromSnapshot()
{
    UExcavationVolumeSubsystem* Excavation = GetWorld()
        ? GetWorld()->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    FExcavationVolumeSnapshot Snapshot;
    if (!Excavation || !Excavation->ExportSnapshot(Snapshot))
    {
        return;
    }

    for (const FExcavationCellSnapshot& Row : Snapshot.Overrides)
    {
        if (Row.Density < 0.5f)
        {
            RefreshOpenCellTracking(Row.Cell);
        }
    }
}

void UUndergroundHazardSubsystem::HandleExcavationChanged(const FExcavationEditRecord& Record, bool bIsRollback)
{
    for (const FExcavationCellChange& Change : Record.Changes)
    {
        // Sample post-transaction density so fills, rollbacks and reloads share the same reconciliation path.
        RefreshOpenCellTracking(Change.Cell);
    }

    if (!Record.Changes.IsEmpty() && !bResolvingCollapse)
    {
        if (UWorld* World = GetWorld())
        {
            if (UStructuralIntegritySubsystem* Structures = World->GetSubsystem<UStructuralIntegritySubsystem>())
            {
                Structures->RecalculateLoadNetwork();
            }
        }
    }
}

void UUndergroundHazardSubsystem::HandleExcavationReset()
{
    OpenCaveCells.Reset();
    OpenCaveCellSet.Reset();
    PendingStabilityCells.Reset();
    PendingStabilityCellSet.Reset();
    StabilityQueueCursor = 0;
    HydrologyCursor = 0;
    SeedOpenCellTrackingFromSnapshot();
}

bool UUndergroundHazardSubsystem::ProcessStabilityCell(const FIntVector& Cell)
{
    UWorld* World = GetWorld();
    UExcavationVolumeSubsystem* Excavation = World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!Excavation || !OpenCaveCellSet.Contains(Cell))
    {
        return false;
    }

    float CavityDensity = 1.0f;
    if (!TrySampleCellDensity(Cell, CavityDensity) || CavityDensity >= 0.5f)
    {
        return false;
    }

    FIntVector RoofCell = Cell;
    bool bFoundRoof = false;
    const int32 SearchHeight = FMath::Clamp(MaxRoofSearchHeightCells, 1, 8);
    for (int32 Height = 1; Height <= SearchHeight; ++Height)
    {
        if (!TryOffsetCell(Cell, FIntVector(0, 0, Height), RoofCell))
        {
            break;
        }
        float RoofDensity = 0.0f;
        if (!TrySampleCellDensity(RoofCell, RoofDensity))
        {
            return false;
        }
        if (RoofDensity >= 0.5f)
        {
            bFoundRoof = true;
            break;
        }
    }

    if (!bFoundRoof || ComputeCavitySpan(Cell) < FMath::Clamp(CriticalUnsupportedRoofSpanCells, 2, 32))
    {
        return false;
    }

    const FVector RoofWorld = Excavation->CellToWorldCenter(RoofCell);
    if (HasOperationalSupportNear(RoofWorld, FMath::Max(1.0f, SupportInfluenceRadiusCm)) || bResolvingCollapse)
    {
        return false;
    }

    FExcavationEditRecord CollapseRecord;
    bResolvingCollapse = true;
    const bool bCarved = Excavation->ApplySphereEdit(
        RoofWorld, FMath::Max(1.0f, Excavation->CellSizeCm * 0.76f),
        EExcavationEditType::Carve, CollapseRecord);
    bResolvingCollapse = false;

    if (!bCarved)
    {
        return false;
    }

    OnRoofCollapse.Broadcast(RoofWorld, CollapseRecord.Changes.Num());
    BroadcastUndergroundWorldEvent(EWorldEventType::TerrainDeformed, RoofWorld,
        static_cast<float>(CollapseRecord.Changes.Num()), TEXT("UndergroundRoofCollapse"));
    return true;
}

bool UUndergroundHazardSubsystem::FindWorldSurfaceCell(
    const FVector& CavePosition, FChunkCell*& OutCell, UChunkState*& OutChunk,
    int32& OutCellIndex, FIntPoint& OutGlobalCell) const
{
    OutCell = nullptr;
    OutChunk = nullptr;
    OutCellIndex = INDEX_NONE;
    OutGlobalCell = FIntPoint::ZeroValue;

    UWorld* World = GetWorld();
    UWorldStateSubsystem* State = World ? World->GetSubsystem<UWorldStateSubsystem>() : nullptr;
    if (!State || !State->StateStore)
    {
        return false;
    }

    const FIntPoint ChunkCoord = State->ChunkSettings.WorldToChunk(CavePosition);
    UChunkState* Chunk = State->GetChunk(ChunkCoord);
    if (!Chunk)
    {
        return false;
    }

    const FIntPoint Local = State->ChunkSettings.WorldToLocalCell(CavePosition);
    FChunkCell* Cell = Chunk->GetCell(Local.X, Local.Y, State->ChunkSettings.CellsPerChunkAxis);
    if (!Cell)
    {
        return false;
    }

    OutCell = Cell;
    OutChunk = Chunk;
    OutCellIndex = Chunk->GetCellIndex(Local.X, Local.Y, State->ChunkSettings.CellsPerChunkAxis);
    OutGlobalCell = State->ChunkSettings.WorldToCell(CavePosition);
    return OutCellIndex != INDEX_NONE;
}

void UUndergroundHazardSubsystem::ProcessHydrologyCell(const FIntVector& Cell)
{
    UWorld* World = GetWorld();
    UExcavationVolumeSubsystem* Excavation = World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!Excavation || !OpenCaveCellSet.Contains(Cell))
    {
        return;
    }

    const FVector CavePosition = Excavation->CellToWorldCenter(Cell);
    FChunkCell* SurfaceCell = nullptr;
    UChunkState* SourceChunk = nullptr;
    int32 SourceCellIndex = INDEX_NONE;
    FIntPoint GlobalCell = FIntPoint::ZeroValue;
    if (!FindWorldSurfaceCell(CavePosition, SurfaceCell, SourceChunk, SourceCellIndex, GlobalCell)
        || !SurfaceCell || !SourceChunk)
    {
        return;
    }

    float TransferM3 = 0.0f;
    const float MaxTransfer = FMath::Clamp(MaxSeepagePerPocketTickM3, 0.0001f, 1.0f);
    if (FMath::IsFinite(SurfaceCell->Water.Volume)
        && SurfaceCell->Water.Volume > FMath::Max(0.0f, SeepageMinSurfaceWaterM3))
    {
        TransferM3 = FMath::Min(MaxTransfer,
            SurfaceCell->Water.Volume * FMath::Clamp(SurfaceWaterTransferFraction, 0.0f, 1.0f));
        SurfaceCell->Water.Volume = FMath::Max(0.0f, SurfaceCell->Water.Volume - TransferM3);
        SurfaceCell->Water.Level = SurfaceCell->Water.Volume / 16.0f;
        SurfaceCell->bHasWater = SurfaceCell->Water.Volume > 0.001f;
    }
    else if (FMath::IsFinite(SurfaceCell->SoilMoisture)
        && SurfaceCell->SoilMoisture > FMath::Clamp(SeepageSoilMoistureThreshold, 0.0f, 1.0f))
    {
        const float ExcessMoisture = SurfaceCell->SoilMoisture -
            FMath::Clamp(SeepageSoilMoistureThreshold, 0.0f, 1.0f);
        TransferM3 = FMath::Min(MaxTransfer, ExcessMoisture * 0.02f);
        SurfaceCell->SoilMoisture = FMath::Clamp(
            SurfaceCell->SoilMoisture - TransferM3 * 10.0f, 0.0f, 1.0f);
    }

    if (TransferM3 <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    FUndergroundWaterPocket& Pocket = WaterPockets.FindOrAdd(Cell);
    Pocket.Cell = Cell;
    Pocket.VolumeM3 = FMath::Max(0.0f, Pocket.VolumeM3) + TransferM3;
    Pocket.FlowRateM3PerSecond = TransferM3 / FMath::Max(0.01f, HydrologyTickSeconds);

    SurfaceCell->bIsDirty = true;
    SourceChunk->MarkDirty();

    for (const TPair<FGuid, TWeakObjectPtr<AUndergroundSupportActor>>& Pair : RegisteredSupports)
    {
        if (AUndergroundSupportActor* Support = Pair.Value.Get())
        {
            if (Support->IsSupportOperational()
                && FVector::DistSquared(Support->GetActorLocation(), CavePosition)
                    <= FMath::Square(FMath::Max(1.0f, SupportInfluenceRadiusCm)))
            {
                Support->ApplyMoistureExposure(TransferM3);
            }
        }
    }

    OnWaterSeepage.Broadcast(CavePosition, TransferM3);
    BroadcastUndergroundWorldEvent(EWorldEventType::WaterLevelChanged, CavePosition,
        TransferM3, TEXT("UndergroundSeepage"));
}

void UUndergroundHazardSubsystem::SimulateUndergroundGravityFlow()
{
    if (WaterPockets.IsEmpty())
    {
        return;
    }

    TArray<FIntVector> Cells;
    WaterPockets.GenerateKeyArray(Cells);
    Cells.Sort(&IsLessCell);

    UExcavationVolumeSubsystem* Excavation = GetWorld()
        ? GetWorld()->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!Excavation)
    {
        return;
    }

    const float MaxTransfer = FMath::Max(0.0f, MaxGravityTransferPerTickM3);
    if (MaxTransfer <= 0.0f)
    {
        return;
    }

    for (const FIntVector& Cell : Cells)
    {
        FUndergroundWaterPocket* Pocket = WaterPockets.Find(Cell);
        if (!Pocket || Pocket->VolumeM3 <= KINDA_SMALL_NUMBER)
        {
            continue;
        }

        FIntVector Below;
        if (!TryOffsetCell(Cell, FIntVector(0, 0, -1), Below) || !OpenCaveCellSet.Contains(Below))
        {
            continue;
        }

        float BelowDensity = 1.0f;
        if (!TrySampleCellDensity(Below, BelowDensity) || BelowDensity >= 0.5f)
        {
            continue;
        }

        const float Transfer = FMath::Min(Pocket->VolumeM3, MaxTransfer);
        if (Transfer <= KINDA_SMALL_NUMBER)
        {
            continue;
        }

        Pocket->VolumeM3 -= Transfer;
        Pocket->FlowRateM3PerSecond = -Transfer / FMath::Max(0.01f, HydrologyTickSeconds);
        FUndergroundWaterPocket& Destination = WaterPockets.FindOrAdd(Below);
        Destination.Cell = Below;
        Destination.VolumeM3 = FMath::Max(0.0f, Destination.VolumeM3) + Transfer;
        Destination.FlowRateM3PerSecond = Transfer / FMath::Max(0.01f, HydrologyTickSeconds);
    }
}

void UUndergroundHazardSubsystem::BroadcastUndergroundWorldEvent(
    EWorldEventType Type, const FVector& LocationCm, float Magnitude, FName SourceTag)
{
    UWorld* World = GetWorld();
    UWorldEventSubsystem* Events = World ? World->GetSubsystem<UWorldEventSubsystem>() : nullptr;
    UWorldStateSubsystem* State = World ? World->GetSubsystem<UWorldStateSubsystem>() : nullptr;
    if (!Events || !State)
    {
        return;
    }

    FWorldEvent Event;
    Event.Type = Type;
    Event.WorldLocation = LocationCm;
    Event.ChunkCoord = State->ChunkSettings.WorldToChunk(LocationCm);
    Event.Magnitude = FMath::Max(0.0f, Magnitude);
    Event.SourceTag = SourceTag;
    Event.Timestamp = World->GetTimeSeconds();
    Event.NumericPayload.Add(TEXT("UndergroundHazard"), Event.Magnitude);
    Events->BroadcastEvent(Event);
}

bool UUndergroundHazardSubsystem::ValidateSnapshot(const FUndergroundHazardSnapshot& Snapshot) const
{
    if (Snapshot.SchemaVersion != 1
        || Snapshot.Supports.Num() > FMath::Max(1, MaxSnapshotSupports)
        || Snapshot.WaterPockets.Num() > FMath::Max(1, MaxSnapshotWaterPockets))
    {
        return false;
    }

    TSet<FGuid> SupportIds;
    for (const FUndergroundSupportSaveRecord& Support : Snapshot.Supports)
    {
        if (!Support.SupportId.IsValid() || SupportIds.Contains(Support.SupportId)
            || !IsFiniteVector3(Support.LocationCm)
            || !FMath::IsFinite(Support.Rotation.Roll) || !FMath::IsFinite(Support.Rotation.Pitch)
            || !FMath::IsFinite(Support.Rotation.Yaw) || !IsValidSupportType(Support.Type)
            || !FMath::IsFinite(Support.MaximumIntegrity) || Support.MaximumIntegrity < 1.0f
            || !FMath::IsFinite(Support.Integrity) || Support.Integrity < 0.0f
            || Support.Integrity > Support.MaximumIntegrity
            || !FMath::IsFinite(Support.MoistureDamagePerM3) || Support.MoistureDamagePerM3 < 0.0f)
        {
            return false;
        }
        SupportIds.Add(Support.SupportId);
    }

    TSet<FIntVector> PocketCells;
    for (const FUndergroundWaterPocket& Pocket : Snapshot.WaterPockets)
    {
        if (!FMath::IsFinite(Pocket.VolumeM3) || Pocket.VolumeM3 < 0.0f
            || !FMath::IsFinite(Pocket.FlowRateM3PerSecond) || PocketCells.Contains(Pocket.Cell))
        {
            return false;
        }
        PocketCells.Add(Pocket.Cell);
    }
    return true;
}

bool UUndergroundHazardSubsystem::ExportSnapshot(FUndergroundHazardSnapshot& OutSnapshot) const
{
    OutSnapshot = FUndergroundHazardSnapshot();
    if (RegisteredSupports.Num() > MaxSnapshotSupports || WaterPockets.Num() > MaxSnapshotWaterPockets)
    {
        return false;
    }

    FUndergroundHazardSnapshot Candidate;
    for (const TPair<FGuid, TWeakObjectPtr<AUndergroundSupportActor>>& Pair : RegisteredSupports)
    {
        const AUndergroundSupportActor* Support = Pair.Value.Get();
        if (!IsValid(Support))
        {
            continue;
        }

        FUndergroundSupportSaveRecord Row;
        Row.SupportId = Support->GetSupportId();
        Row.LocationCm = Support->GetActorLocation();
        Row.Rotation = Support->GetActorRotation();
        Row.Type = Support->SupportType;
        Row.MaximumIntegrity = Support->MaximumIntegrity;
        Row.Integrity = Support->GetSupportIntegrity();
        Row.MoistureDamagePerM3 = Support->MoistureDamagePerM3;
        Candidate.Supports.Add(Row);
    }

    Candidate.Supports.Sort([](const FUndergroundSupportSaveRecord& A, const FUndergroundSupportSaveRecord& B)
    {
        return A.SupportId.ToString(EGuidFormats::Digits) < B.SupportId.ToString(EGuidFormats::Digits);
    });

    for (const TPair<FIntVector, FUndergroundWaterPocket>& Pair : WaterPockets)
    {
        Candidate.WaterPockets.Add(Pair.Value);
    }
    Candidate.WaterPockets.Sort([](const FUndergroundWaterPocket& A, const FUndergroundWaterPocket& B)
    {
        return IsLessCell(A.Cell, B.Cell);
    });

    if (!ValidateSnapshot(Candidate))
    {
        return false;
    }
    OutSnapshot = MoveTemp(Candidate);
    return true;
}

bool UUndergroundHazardSubsystem::ImportSnapshot(const FUndergroundHazardSnapshot& Snapshot)
{
    if (!ValidateSnapshot(Snapshot))
    {
        return false;
    }

    UWorld* World = GetWorld();
    UExcavationVolumeSubsystem* Excavation = World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!World || !Excavation || !SupportActorClass)
    {
        return false;
    }

    TArray<AUndergroundSupportActor*> SpawnCandidates;
    SpawnCandidates.Reserve(Snapshot.Supports.Num());
    for (const FUndergroundSupportSaveRecord& Row : Snapshot.Supports)
    {
        AUndergroundSupportActor* Candidate = World->SpawnActorDeferred<AUndergroundSupportActor>(
            SupportActorClass.Get(), FTransform(Row.Rotation, Row.LocationCm), nullptr, nullptr,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (!Candidate)
        {
            for (AUndergroundSupportActor* Spawned : SpawnCandidates)
            {
                if (IsValid(Spawned)) Spawned->Destroy();
            }
            return false;
        }

        Candidate->MaximumIntegrity = Row.MaximumIntegrity;
        Candidate->MoistureDamagePerM3 = Row.MoistureDamagePerM3;
        Candidate->ConfigureSupport(Row.SupportId, Row.Type, Row.Integrity);
        SpawnCandidates.Add(Candidate);
    }

    TArray<AUndergroundSupportActor*> ExistingSupports;
    for (const TPair<FGuid, TWeakObjectPtr<AUndergroundSupportActor>>& Pair : RegisteredSupports)
    {
        if (AUndergroundSupportActor* Existing = Pair.Value.Get())
        {
            ExistingSupports.Add(Existing);
        }
    }
    RegisteredSupports.Reset();
    for (AUndergroundSupportActor* Existing : ExistingSupports)
    {
        if (IsValid(Existing)) Existing->Destroy();
    }

    for (int32 Index = 0; Index < SpawnCandidates.Num(); ++Index)
    {
        AUndergroundSupportActor* Candidate = SpawnCandidates[Index];
        const FUndergroundSupportSaveRecord& Row = Snapshot.Supports[Index];
        Candidate->FinishSpawning(FTransform(Row.Rotation, Row.LocationCm));
        RegisterSupportActor(Candidate);
    }

    WaterPockets.Reset();
    for (const FUndergroundWaterPocket& Pocket : Snapshot.WaterPockets)
    {
        WaterPockets.Add(Pocket.Cell, Pocket);
    }

    HandleExcavationReset();
    return true;
}

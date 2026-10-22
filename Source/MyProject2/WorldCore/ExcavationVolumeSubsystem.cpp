#include "ExcavationVolumeSubsystem.h"
#include "ExcavationBaseDensitySource.h"

#include <cmath>

namespace
{
    bool IsFiniteVector(const FVector& Value)
    {
        return FMath::IsFinite(Value.X)
            && FMath::IsFinite(Value.Y)
            && FMath::IsFinite(Value.Z);
    }

    bool IsValidDensity(float Density)
    {
        return FMath::IsFinite(Density) && Density >= 0.0f && Density <= 1.0f;
    }

    bool IsValidEditType(EExcavationEditType EditType)
    {
        return EditType == EExcavationEditType::Carve
            || EditType == EExcavationEditType::Fill;
    }

    bool IsValidShapeType(EExcavationShapeType ShapeType)
    {
        return ShapeType == EExcavationShapeType::Sphere
            || ShapeType == EExcavationShapeType::Capsule
            || ShapeType == EExcavationShapeType::Box;
    }

    int32 FloorDivide(int32 Value, int32 PositiveDivisor)
    {
        const int32 Quotient = Value / PositiveDivisor;
        const int32 Remainder = Value % PositiveDivisor;
        return Remainder < 0 ? Quotient - 1 : Quotient;
    }

    int32 PositiveModulo(int32 Value, int32 PositiveDivisor)
    {
        const int32 Remainder = Value % PositiveDivisor;
        return Remainder < 0 ? Remainder + PositiveDivisor : Remainder;
    }

    FIntVector ChunkForCellAtSize(const FIntVector& Cell, int32 CellsPerChunkAxis)
    {
        return FIntVector(
            FloorDivide(Cell.X, CellsPerChunkAxis),
            FloorDivide(Cell.Y, CellsPerChunkAxis),
            FloorDivide(Cell.Z, CellsPerChunkAxis));
    }

    bool IsValidCellCoordinate(double WorldCoordinateCm, double CellSizeCm, int32& OutCell)
    {
        if (!FMath::IsFinite(WorldCoordinateCm)
            || !FMath::IsFinite(CellSizeCm)
            || CellSizeCm <= 0.0)
        {
            return false;
        }

        const double Scaled = WorldCoordinateCm / CellSizeCm;
        if (!FMath::IsFinite(Scaled))
        {
            return false;
        }

        const double Floored = std::floor(Scaled);
        if (Floored < static_cast<double>(TNumericLimits<int32>::Lowest())
            || Floored > static_cast<double>(TNumericLimits<int32>::Max()))
        {
            return false;
        }

        OutCell = static_cast<int32>(Floored);
        return true;
    }

    double DistanceSquaredToSegment(const FVector& Point, const FVector& SegmentStart, const FVector& SegmentEnd)
    {
        const FVector Segment = SegmentEnd - SegmentStart;
        const double SegmentLengthSquared = Segment.SizeSquared();
        if (SegmentLengthSquared <= KINDA_SMALL_NUMBER)
        {
            return FVector::DistSquared(Point, SegmentStart);
        }

        const double T = FMath::Clamp(
            FVector::DotProduct(Point - SegmentStart, Segment) / SegmentLengthSquared,
            0.0,
            1.0);
        const FVector ClosestPoint = SegmentStart + Segment * T;
        return FVector::DistSquared(Point, ClosestPoint);
    }
}

bool UExcavationVolumeSubsystem::IsGridConfigurationCompatible() const
{
    if (!FMath::IsFinite(CellSizeCm)
        || CellSizeCm < 1.0f
        || CellsPerChunkAxis < 4
        || MaxCellsPerEdit <= 0
        || !IsValidDensity(DefaultBaseDensity))
    {
        return false;
    }

    if (CellOverrides.Num() > 0)
    {
        if ((LockedCellSizeCm > 0.0f && !FMath::IsNearlyEqual(CellSizeCm, LockedCellSizeCm))
            || (LockedCellsPerChunkAxis > 0 && CellsPerChunkAxis != LockedCellsPerChunkAxis))
        {
            return false;
        }
    }

    return true;
}

void UExcavationVolumeSubsystem::Deinitialize()
{
    CellOverrides.Reset();
    CellsByChunk.Reset();
    BaseDensitySourceObject = nullptr;
    FallbackBaseDensitySourceObject = nullptr;
    LockedCellSizeCm = 0.0f;
    LockedCellsPerChunkAxis = 0;
    Super::Deinitialize();
}

bool UExcavationVolumeSubsystem::TryWorldToCell(const FVector& WorldPositionCm, FIntVector& OutCell) const
{
    if (!IsFiniteVector(WorldPositionCm) || !IsGridConfigurationCompatible())
    {
        return false;
    }

    FIntVector Candidate;
    if (!IsValidCellCoordinate(WorldPositionCm.X, CellSizeCm, Candidate.X)
        || !IsValidCellCoordinate(WorldPositionCm.Y, CellSizeCm, Candidate.Y)
        || !IsValidCellCoordinate(WorldPositionCm.Z, CellSizeCm, Candidate.Z))
    {
        return false;
    }

    OutCell = Candidate;
    return true;
}

FVector UExcavationVolumeSubsystem::CellToWorldCenter(const FIntVector& Cell) const
{
    if (!IsGridConfigurationCompatible())
    {
        return FVector::ZeroVector;
    }

    const double HalfCell = static_cast<double>(CellSizeCm) * 0.5;
    return FVector(
        (static_cast<double>(Cell.X) * CellSizeCm) + HalfCell,
        (static_cast<double>(Cell.Y) * CellSizeCm) + HalfCell,
        (static_cast<double>(Cell.Z) * CellSizeCm) + HalfCell);
}

bool UExcavationVolumeSubsystem::TryGetDensityOverrideAtWorld(
    const FVector& WorldPositionCm,
    float& OutDensity) const
{
    FIntVector Cell;
    if (!TryWorldToCell(WorldPositionCm, Cell))
    {
        return false;
    }

    return TryGetDensityOverrideAtCell(Cell, OutDensity);
}

bool UExcavationVolumeSubsystem::TryGetDensityOverrideAtCell(
    const FIntVector& Cell,
    float& OutDensity) const
{
    if (!IsGridConfigurationCompatible())
    {
        return false;
    }

    const FExcavationCellOverride* Override = CellOverrides.Find(Cell);
    if (!Override || !IsValidDensity(Override->Density))
    {
        return false;
    }

    OutDensity = Override->Density;
    return true;
}

bool UExcavationVolumeSubsystem::TrySampleDensityAtWorld(
    const FVector& WorldPositionCm,
    float& OutDensity) const
{
    FIntVector Cell;
    if (!TryWorldToCell(WorldPositionCm, Cell))
    {
        return false;
    }

    if (TryGetDensityOverrideAtCell(Cell, OutDensity))
    {
        return true;
    }

    UObject* DensitySource = IsValid(BaseDensitySourceObject)
        ? BaseDensitySourceObject.Get()
        : FallbackBaseDensitySourceObject.Get();
    if (DensitySource != nullptr)
    {
        if (!IsValid(DensitySource)
            || !DensitySource->GetClass()->ImplementsInterface(
                UExcavationBaseDensitySource::StaticClass()))
        {
            return false;
        }

        const float SampledDensity =
            IExcavationBaseDensitySource::Execute_SampleBaseDensityAtWorld(
                DensitySource,
                WorldPositionCm);
        if (!IsValidDensity(SampledDensity))
        {
            return false;
        }

        OutDensity = SampledDensity;
        return true;
    }

    if (!IsValidDensity(DefaultBaseDensity))
    {
        return false;
    }

    OutDensity = DefaultBaseDensity;
    return true;
}

bool UExcavationVolumeSubsystem::TrySampleBaseMaterialIdAtWorld(
    const FVector& WorldPositionCm,
    int32& OutMaterialId) const
{
    OutMaterialId = 0;
    if (!IsFiniteVector(WorldPositionCm) || !IsGridConfigurationCompatible())
    {
        return false;
    }

    UObject* MaterialSource = IsValid(BaseDensitySourceObject)
        ? BaseDensitySourceObject.Get()
        : FallbackBaseDensitySourceObject.Get();
    if (!IsValid(MaterialSource))
    {
        // Scalar-density-only worlds have one default material layer.
        return true;
    }

    if (!MaterialSource->GetClass()->ImplementsInterface(UExcavationBaseDensitySource::StaticClass()))
    {
        return false;
    }

    const int32 MaterialId =
        IExcavationBaseDensitySource::Execute_SampleBaseMaterialIdAtWorld(
            MaterialSource, WorldPositionCm);
    if (MaterialId < 0 || MaterialId > 31)
    {
        return false;
    }

    OutMaterialId = MaterialId;
    return true;
}

bool UExcavationVolumeSubsystem::RegisterBaseDensitySource(UObject* Source)
{
    if (!IsValid(Source)
        || !Source->GetClass()->ImplementsInterface(UExcavationBaseDensitySource::StaticClass()))
    {
        return false;
    }

    BaseDensitySourceObject = Source;
    return true;
}

bool UExcavationVolumeSubsystem::RegisterFallbackBaseDensitySource(UObject* Source)
{
    if (!IsValid(Source)
        || !Source->GetClass()->ImplementsInterface(UExcavationBaseDensitySource::StaticClass()))
    {
        return false;
    }

    FallbackBaseDensitySourceObject = Source;
    return true;
}

void UExcavationVolumeSubsystem::UnregisterFallbackBaseDensitySource(UObject* Source)
{
    if (Source == nullptr || FallbackBaseDensitySourceObject == Source)
    {
        FallbackBaseDensitySourceObject = nullptr;
    }
}

void UExcavationVolumeSubsystem::UnregisterBaseDensitySource(UObject* Source)
{
    if (Source == nullptr || BaseDensitySourceObject == Source)
    {
        BaseDensitySourceObject = nullptr;
    }
}

bool UExcavationVolumeSubsystem::ExportSnapshot(FExcavationVolumeSnapshot& OutSnapshot) const
{
    OutSnapshot = FExcavationVolumeSnapshot();

    if (!IsGridConfigurationCompatible()
        || MaxSnapshotOverrideCells <= 0
        || CellOverrides.Num() > MaxSnapshotOverrideCells)
    {
        return false;
    }

    FExcavationVolumeSnapshot Candidate;
    Candidate.SchemaVersion = 1;
    Candidate.CellSizeCm = CellSizeCm;
    Candidate.CellsPerChunkAxis = CellsPerChunkAxis;
    Candidate.DefaultBaseDensity = DefaultBaseDensity;
    Candidate.Overrides.Reserve(CellOverrides.Num());

    for (const TPair<FIntVector, FExcavationCellOverride>& Pair : CellOverrides)
    {
        if (!IsValidDensity(Pair.Value.Density))
        {
            return false;
        }

        FExcavationCellSnapshot Row;
        Row.Cell = Pair.Key;
        Row.Density = Pair.Value.Density;
        Candidate.Overrides.Add(Row);
    }

    Candidate.Overrides.Sort([](const FExcavationCellSnapshot& A, const FExcavationCellSnapshot& B)
    {
        if (A.Cell.X != B.Cell.X) return A.Cell.X < B.Cell.X;
        if (A.Cell.Y != B.Cell.Y) return A.Cell.Y < B.Cell.Y;
        return A.Cell.Z < B.Cell.Z;
    });

    OutSnapshot = MoveTemp(Candidate);
    return true;
}

bool UExcavationVolumeSubsystem::ImportSnapshot(
    const FExcavationVolumeSnapshot& Snapshot,
    bool bReplaceExistingState)
{
    if (Snapshot.SchemaVersion != 1
        || !FMath::IsFinite(Snapshot.CellSizeCm)
        || Snapshot.CellSizeCm < 1.0f
        || Snapshot.CellsPerChunkAxis < 4
        || !IsValidDensity(Snapshot.DefaultBaseDensity)
        || MaxSnapshotOverrideCells <= 0
        || Snapshot.Overrides.Num() > MaxSnapshotOverrideCells
        || (!bReplaceExistingState && CellOverrides.Num() > 0))
    {
        return false;
    }

    TMap<FIntVector, FExcavationCellOverride> NewOverrides;
    TMap<FIntVector, TSet<FIntVector>> NewCellsByChunk;
    NewOverrides.Reserve(Snapshot.Overrides.Num());
    NewCellsByChunk.Reserve(Snapshot.Overrides.Num());

    // Imported cells share a baseline owner ID. Edit receipts are transient,
    // so no old operation is allowed to roll back across a save/load boundary.
    const FGuid ImportedBaselineId = FGuid::NewGuid();

    for (const FExcavationCellSnapshot& Row : Snapshot.Overrides)
    {
        if (!IsValidDensity(Row.Density) || NewOverrides.Contains(Row.Cell))
        {
            return false;
        }

        FExcavationCellOverride Override;
        Override.Density = Row.Density;
        Override.LastOperationId = ImportedBaselineId;
        NewOverrides.Add(Row.Cell, Override);
        NewCellsByChunk.FindOrAdd(
            ChunkForCellAtSize(Row.Cell, Snapshot.CellsPerChunkAxis)).Add(Row.Cell);
    }

    // All data and index validation completed; commit the replacement as one swap.
    CellOverrides = MoveTemp(NewOverrides);
    CellsByChunk = MoveTemp(NewCellsByChunk);
    CellSizeCm = Snapshot.CellSizeCm;
    CellsPerChunkAxis = Snapshot.CellsPerChunkAxis;
    DefaultBaseDensity = Snapshot.DefaultBaseDensity;

    if (CellOverrides.Num() > 0)
    {
        LockedCellSizeCm = CellSizeCm;
        LockedCellsPerChunkAxis = CellsPerChunkAxis;
    }
    else
    {
        LockedCellSizeCm = 0.0f;
        LockedCellsPerChunkAxis = 0;
    }

    OnVolumeReset.Broadcast();
    if (UWorld* World = GetWorld())
    {
        if (UWorldMechanicEventSubsystem* Events = World->GetSubsystem<UWorldMechanicEventSubsystem>())
        {
            FWorldMechanicEvent Event;
            Event.Type = EWorldMechanicEventType::VoxelWorldReset;
            Event.ScalarValue = static_cast<float>(Snapshot.Overrides.Num());
            Event.SourceTag = TEXT("ExcavationVolumeSubsystem");
            Event.bHighPriority = true;
            Events->EnqueueEvent(MoveTemp(Event));
        }
    }
    return true;
}

bool UExcavationVolumeSubsystem::ResolveDensityAtCell(
    const FIntVector& Cell,
    float BaseDensity,
    float& OutDensity) const
{
    if (!IsGridConfigurationCompatible() || !IsValidDensity(BaseDensity))
    {
        return false;
    }

    const FExcavationCellOverride* Override = CellOverrides.Find(Cell);
    const float ResolvedDensity = Override ? Override->Density : BaseDensity;
    if (!IsValidDensity(ResolvedDensity))
    {
        return false;
    }

    OutDensity = ResolvedDensity;
    return true;
}

bool UExcavationVolumeSubsystem::IsCellOverridden(const FIntVector& Cell) const
{
    return IsGridConfigurationCompatible() && CellOverrides.Contains(Cell);
}

bool UExcavationVolumeSubsystem::TryGetChunkCoordinateForCell(
    const FIntVector& Cell,
    FIntVector& OutChunkCoordinate) const
{
    if (!IsGridConfigurationCompatible())
    {
        return false;
    }

    OutChunkCoordinate = ChunkForCellAtSize(Cell, CellsPerChunkAxis);
    return true;
}

TArray<FIntVector> UExcavationVolumeSubsystem::GetOverrideCellsInChunk(
    const FIntVector& ChunkCoordinate) const
{
    TArray<FIntVector> Result;
    if (!IsGridConfigurationCompatible())
    {
        return Result;
    }

    const TSet<FIntVector>* IndexedCells = CellsByChunk.Find(ChunkCoordinate);
    if (!IndexedCells)
    {
        return Result;
    }

    Result = IndexedCells->Array();
    Result.Sort([](const FIntVector& A, const FIntVector& B)
    {
        if (A.X != B.X) return A.X < B.X;
        if (A.Y != B.Y) return A.Y < B.Y;
        return A.Z < B.Z;
    });
    return Result;
}

TArray<FIntVector> UExcavationVolumeSubsystem::GetAffectedChunksForEdit(
    const FExcavationEditRecord& Record) const
{
    TArray<FIntVector> Result;
    if (!Record.bCommitted || Record.SchemaVersion != 1 || !IsGridConfigurationCompatible())
    {
        return Result;
    }

    TSet<FIntVector> AffectedChunks;
    const int32 ChunkSize = CellsPerChunkAxis;
    for (const FExcavationCellChange& Change : Record.Changes)
    {
        const FIntVector Chunk = ChunkForCellAtSize(Change.Cell, ChunkSize);
        const int32 LocalX = PositiveModulo(Change.Cell.X, ChunkSize);
        const int32 LocalY = PositiveModulo(Change.Cell.Y, ChunkSize);
        const int32 LocalZ = PositiveModulo(Change.Cell.Z, ChunkSize);

        const int32 MinOffsetX = LocalX <= 1 ? -1 : 0;
        const int32 MaxOffsetX = LocalX >= ChunkSize - 2 ? 1 : 0;
        const int32 MinOffsetY = LocalY <= 1 ? -1 : 0;
        const int32 MaxOffsetY = LocalY >= ChunkSize - 2 ? 1 : 0;
        const int32 MinOffsetZ = LocalZ <= 1 ? -1 : 0;
        const int32 MaxOffsetZ = LocalZ >= ChunkSize - 2 ? 1 : 0;

        for (int32 OffsetX = MinOffsetX; OffsetX <= MaxOffsetX; ++OffsetX)
        {
            for (int32 OffsetY = MinOffsetY; OffsetY <= MaxOffsetY; ++OffsetY)
            {
                for (int32 OffsetZ = MinOffsetZ; OffsetZ <= MaxOffsetZ; ++OffsetZ)
                {
                    AffectedChunks.Add(Chunk + FIntVector(OffsetX, OffsetY, OffsetZ));
                }
            }
        }
    }

    Result = AffectedChunks.Array();
    Result.Sort([](const FIntVector& A, const FIntVector& B)
    {
        if (A.X != B.X) return A.X < B.X;
        if (A.Y != B.Y) return A.Y < B.Y;
        return A.Z < B.Z;
    });
    return Result;
}

bool UExcavationVolumeSubsystem::ApplySphereEdit(
    const FVector& CenterCm,
    float RadiusCm,
    EExcavationEditType EditType,
    FExcavationEditRecord& OutRecord)
{
    OutRecord = FExcavationEditRecord();
    if (!IsFiniteVector(CenterCm) || !FMath::IsFinite(RadiusCm) || RadiusCm <= 0.0f)
    {
        return false;
    }

    const FVector RadiusVector(RadiusCm, RadiusCm, RadiusCm);
    return ApplyBoundedEdit(
        CenterCm - RadiusVector,
        CenterCm + RadiusVector,
        EExcavationShapeType::Sphere,
        CenterCm,
        FVector::ZeroVector,
        FVector::ZeroVector,
        RadiusCm,
        EditType,
        OutRecord);
}

bool UExcavationVolumeSubsystem::ApplyCapsuleEdit(
    const FVector& StartCm,
    const FVector& EndCm,
    float RadiusCm,
    EExcavationEditType EditType,
    FExcavationEditRecord& OutRecord)
{
    OutRecord = FExcavationEditRecord();
    if (!IsFiniteVector(StartCm) || !IsFiniteVector(EndCm)
        || !FMath::IsFinite(RadiusCm) || RadiusCm <= 0.0f)
    {
        return false;
    }

    const FVector RadiusVector(RadiusCm, RadiusCm, RadiusCm);
    const FVector MinEndpoint(
        FMath::Min(StartCm.X, EndCm.X),
        FMath::Min(StartCm.Y, EndCm.Y),
        FMath::Min(StartCm.Z, EndCm.Z));
    const FVector MaxEndpoint(
        FMath::Max(StartCm.X, EndCm.X),
        FMath::Max(StartCm.Y, EndCm.Y),
        FMath::Max(StartCm.Z, EndCm.Z));
    const FVector MinBounds = MinEndpoint - RadiusVector;
    const FVector MaxBounds = MaxEndpoint + RadiusVector;

    return ApplyBoundedEdit(
        MinBounds,
        MaxBounds,
        EExcavationShapeType::Capsule,
        StartCm,
        EndCm,
        FVector::ZeroVector,
        RadiusCm,
        EditType,
        OutRecord);
}

bool UExcavationVolumeSubsystem::ApplyBoxEdit(
    const FVector& MinWorldCm,
    const FVector& MaxWorldCm,
    EExcavationEditType EditType,
    FExcavationEditRecord& OutRecord)
{
    OutRecord = FExcavationEditRecord();
    if (!IsFiniteVector(MinWorldCm) || !IsFiniteVector(MaxWorldCm)
        || MaxWorldCm.X < MinWorldCm.X
        || MaxWorldCm.Y < MinWorldCm.Y
        || MaxWorldCm.Z < MinWorldCm.Z)
    {
        return false;
    }

    const FVector Extent = (MaxWorldCm - MinWorldCm) * 0.5;
    const FVector Center = MinWorldCm + Extent;
    return ApplyBoundedEdit(
        MinWorldCm,
        MaxWorldCm,
        EExcavationShapeType::Box,
        Center,
        FVector::ZeroVector,
        Extent,
        0.0f,
        EditType,
        OutRecord);
}

bool UExcavationVolumeSubsystem::ApplyBoundedEdit(
    const FVector& MinBoundsCm,
    const FVector& MaxBoundsCm,
    EExcavationShapeType ShapeType,
    const FVector& CenterCm,
    const FVector& EndCm,
    const FVector& HalfExtentCm,
    float RadiusCm,
    EExcavationEditType EditType,
    FExcavationEditRecord& OutRecord)
{
    OutRecord = FExcavationEditRecord();

    if (!IsFiniteVector(MinBoundsCm)
        || !IsFiniteVector(MaxBoundsCm)
        || !IsFiniteVector(CenterCm)
        || !IsFiniteVector(EndCm)
        || !IsFiniteVector(HalfExtentCm)
        || !IsValidShapeType(ShapeType)
        || !IsValidEditType(EditType)
        || !IsGridConfigurationCompatible()
        || (ShapeType != EExcavationShapeType::Box && (!FMath::IsFinite(RadiusCm) || RadiusCm <= 0.0f))
        || (ShapeType == EExcavationShapeType::Box
            && (HalfExtentCm.X < 0.0 || HalfExtentCm.Y < 0.0 || HalfExtentCm.Z < 0.0))
        || MaxBoundsCm.X < MinBoundsCm.X
        || MaxBoundsCm.Y < MinBoundsCm.Y
        || MaxBoundsCm.Z < MinBoundsCm.Z)
    {
        return false;
    }

    FIntVector MinCell;
    FIntVector MaxCell;
    if (!TryWorldToCell(MinBoundsCm, MinCell)
        || !TryWorldToCell(MaxBoundsCm, MaxCell))
    {
        return false;
    }

    const int64 CountX = static_cast<int64>(MaxCell.X) - MinCell.X + 1;
    const int64 CountY = static_cast<int64>(MaxCell.Y) - MinCell.Y + 1;
    const int64 CountZ = static_cast<int64>(MaxCell.Z) - MinCell.Z + 1;
    if (CountX <= 0 || CountY <= 0 || CountZ <= 0
        || CountX > MaxCellsPerEdit
        || CountY > MaxCellsPerEdit / CountX
        || CountZ > MaxCellsPerEdit / (CountX * CountY))
    {
        return false;
    }

    const float TargetDensity = EditType == EExcavationEditType::Carve ? 0.0f : 1.0f;
    const double RadiusSquared = static_cast<double>(RadiusCm) * RadiusCm;
    FExcavationEditRecord CandidateRecord;
    CandidateRecord.OperationId = FGuid::NewGuid();
    CandidateRecord.EditType = EditType;
    CandidateRecord.ShapeType = ShapeType;
    CandidateRecord.CenterCm = CenterCm;
    CandidateRecord.EndCm = EndCm;
    CandidateRecord.HalfExtentCm = HalfExtentCm;
    CandidateRecord.RadiusCm = RadiusCm;
    CandidateRecord.Changes.Reserve(static_cast<int32>(FMath::Min<int64>(
        CountX * CountY * CountZ,
        MaxCellsPerEdit)));

    // Build the entire change set first. Invalid/oversized edits never mutate state.
    for (int32 X = MinCell.X; ; ++X)
    {
        for (int32 Y = MinCell.Y; ; ++Y)
        {
            for (int32 Z = MinCell.Z; ; ++Z)
            {
                const FIntVector Cell(X, Y, Z);
                const FVector CellCenter = CellToWorldCenter(Cell);
                bool bInsideShape = false;

                switch (ShapeType)
                {
                    case EExcavationShapeType::Sphere:
                        bInsideShape = FVector::DistSquared(CellCenter, CenterCm) <= RadiusSquared;
                        break;

                    case EExcavationShapeType::Capsule:
                        bInsideShape = DistanceSquaredToSegment(CellCenter, CenterCm, EndCm) <= RadiusSquared;
                        break;

                    case EExcavationShapeType::Box:
                    {
                        const FVector Delta = CellCenter - CenterCm;
                        bInsideShape = FMath::Abs(Delta.X) <= HalfExtentCm.X
                            && FMath::Abs(Delta.Y) <= HalfExtentCm.Y
                            && FMath::Abs(Delta.Z) <= HalfExtentCm.Z;
                        break;
                    }

                    default:
                        return false;
                }

                if (bInsideShape)
                {
                    float ResolvedBefore = 0.0f;
                    if (!TrySampleDensityAtWorld(CellCenter, ResolvedBefore) || !IsValidDensity(ResolvedBefore))
                    {
                        return false;
                    }

                    // Avoid storing deltas in already-empty/full cells. Besides reducing
                    // sparse-save size, this keeps "dig in air" from creating fake edits.
                    if (FMath::IsNearlyEqual(ResolvedBefore, TargetDensity, 0.0001f))
                    {
                        if (Z == MaxCell.Z) break;
                        continue;
                    }

                    const FExcavationCellOverride* Existing = CellOverrides.Find(Cell);
                    FExcavationCellChange Change;
                    Change.Cell = Cell;
                    Change.bHadPreviousOverride = Existing != nullptr;
                    Change.PreviousDensity = Existing ? Existing->Density : DefaultBaseDensity;
                    Change.PreviousOperationId = Existing ? Existing->LastOperationId : FGuid();
                    Change.NewDensity = TargetDensity;
                    const double CellSizeMeters = static_cast<double>(CellSizeCm) / 100.0;
                    CandidateRecord.SolidVolumeDeltaM3 += static_cast<float>(
                        (ResolvedBefore - TargetDensity) * CellSizeMeters * CellSizeMeters * CellSizeMeters);
                    CandidateRecord.Changes.Add(Change);
                }

                if (Z == MaxCell.Z) break;
            }
            if (Y == MaxCell.Y) break;
        }
        if (X == MaxCell.X) break;
    }

    if (CandidateRecord.Changes.IsEmpty())
    {
        return false;
    }
    CandidateRecord.MovedCellCount = CandidateRecord.Changes.Num();
    CandidateRecord.BlockedCellCount = 0; // Atomic edit preflight rejects protected cells before mutation.

    if (CellOverrides.Num() == 0)
    {
        LockedCellSizeCm = CellSizeCm;
        LockedCellsPerChunkAxis = CellsPerChunkAxis;
    }

    for (const FExcavationCellChange& Change : CandidateRecord.Changes)
    {
        FExcavationCellOverride NewOverride;
        NewOverride.Density = Change.NewDensity;
        NewOverride.LastOperationId = CandidateRecord.OperationId;
        CellOverrides.Add(Change.Cell, NewOverride);
        CellsByChunk.FindOrAdd(ChunkForCellAtSize(Change.Cell, LockedCellsPerChunkAxis)).Add(Change.Cell);
    }

    CandidateRecord.bCommitted = true;
    OutRecord = MoveTemp(CandidateRecord);
    OnVolumeChanged.Broadcast(OutRecord, false);

    if (UWorld* World = GetWorld())
    {
        if (UWorldMechanicEventSubsystem* Events = World->GetSubsystem<UWorldMechanicEventSubsystem>())
        {
            FBox ChangedBounds(ForceInit);
            for (const FExcavationCellChange& Change : OutRecord.Changes)
            {
                ChangedBounds += CellToWorldCenter(Change.Cell);
            }
            FWorldMechanicEvent Event;
            Event.Type = EWorldMechanicEventType::VoxelEditCommitted;
            Event.OperationId = OutRecord.OperationId;
            Event.SourceTag = EditType == EExcavationEditType::Carve ? FName(TEXT("Carve")) : FName(TEXT("Fill"));
            Event.ScalarValue = static_cast<float>(OutRecord.Changes.Num());
            if (ChangedBounds.IsValid)
            {
                Event.PositionCm = ChangedBounds.GetCenter();
                Event.ExtentCm = ChangedBounds.GetExtent() + FVector(CellSizeCm * 0.5f);
            }
            Event.bHighPriority = true;
            Events->EnqueueEvent(MoveTemp(Event));
        }
    }
    return true;
}

bool UExcavationVolumeSubsystem::RollbackEdit(const FExcavationEditRecord& Record)
{
    if (!Record.bCommitted
        || !Record.OperationId.IsValid()
        || Record.SchemaVersion != 1
        || Record.Changes.IsEmpty()
        || !IsGridConfigurationCompatible())
    {
        return false;
    }

    TSet<FIntVector> SeenCells;
    SeenCells.Reserve(Record.Changes.Num());

    // Validate every cell before changing anything. Newer edits own their cells
    // even when they happened to write the same density as an older edit.
    for (const FExcavationCellChange& Change : Record.Changes)
    {
        if (SeenCells.Contains(Change.Cell)
            || !IsValidDensity(Change.NewDensity)
            || (Change.bHadPreviousOverride && !IsValidDensity(Change.PreviousDensity)))
        {
            return false;
        }
        SeenCells.Add(Change.Cell);

        const FExcavationCellOverride* Current = CellOverrides.Find(Change.Cell);
        if (!Current
            || Current->LastOperationId != Record.OperationId
            || !FMath::IsNearlyEqual(Current->Density, Change.NewDensity))
        {
            return false;
        }
    }

    for (const FExcavationCellChange& Change : Record.Changes)
    {
        const int32 IndexChunkSize = LockedCellsPerChunkAxis > 0
            ? LockedCellsPerChunkAxis
            : CellsPerChunkAxis;
        const FIntVector Chunk = ChunkForCellAtSize(Change.Cell, IndexChunkSize);

        if (Change.bHadPreviousOverride)
        {
            FExcavationCellOverride Restored;
            Restored.Density = Change.PreviousDensity;
            Restored.LastOperationId = Change.PreviousOperationId;
            CellOverrides.Add(Change.Cell, Restored);
            CellsByChunk.FindOrAdd(Chunk).Add(Change.Cell);
        }
        else
        {
            CellOverrides.Remove(Change.Cell);
            if (TSet<FIntVector>* IndexedCells = CellsByChunk.Find(Chunk))
            {
                IndexedCells->Remove(Change.Cell);
                if (IndexedCells->Num() == 0)
                {
                    CellsByChunk.Remove(Chunk);
                }
            }
        }
    }

    if (CellOverrides.Num() == 0)
    {
        LockedCellSizeCm = 0.0f;
        LockedCellsPerChunkAxis = 0;
        CellsByChunk.Reset();
    }

    OnVolumeChanged.Broadcast(Record, true);

    if (UWorld* World = GetWorld())
    {
        if (UWorldMechanicEventSubsystem* Events = World->GetSubsystem<UWorldMechanicEventSubsystem>())
        {
            FBox ChangedBounds(ForceInit);
            for (const FExcavationCellChange& Change : Record.Changes)
            {
                ChangedBounds += CellToWorldCenter(Change.Cell);
            }
            FWorldMechanicEvent Event;
            Event.Type = EWorldMechanicEventType::VoxelEditRolledBack;
            Event.OperationId = Record.OperationId;
            Event.SourceTag = TEXT("ExcavationVolumeSubsystem");
            Event.ScalarValue = static_cast<float>(Record.Changes.Num());
            if (ChangedBounds.IsValid)
            {
                Event.PositionCm = ChangedBounds.GetCenter();
                Event.ExtentCm = ChangedBounds.GetExtent() + FVector(CellSizeCm * 0.5f);
            }
            Event.bHighPriority = true;
            Events->EnqueueEvent(MoveTemp(Event));
        }
    }
    return true;
}

#include "ExcavationVolumeSubsystem.h"

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
        if (!FMath::IsFinite(WorldCoordinateCm) || !FMath::IsFinite(CellSizeCm) || CellSizeCm <= 0.0)
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
}

void UExcavationVolumeSubsystem::Deinitialize()
{
    CellOverrides.Reset();
    CellsByChunk.Reset();
    LockedCellSizeCm = 0.0f;
    LockedCellsPerChunkAxis = 0;
    Super::Deinitialize();
}

bool UExcavationVolumeSubsystem::TryWorldToCell(const FVector& WorldPositionCm, FIntVector& OutCell) const
{
    if (!IsFiniteVector(WorldPositionCm) || !FMath::IsFinite(CellSizeCm) || CellSizeCm <= 0.0f)
    {
        return false;
    }
    if (CellsPerChunkAxis < 4
        || (CellOverrides.Num() > 0
            && ((LockedCellSizeCm > 0.0f && !FMath::IsNearlyEqual(CellSizeCm, LockedCellSizeCm))
                || (LockedCellsPerChunkAxis > 0 && CellsPerChunkAxis != LockedCellsPerChunkAxis))))
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
    if (!FMath::IsFinite(CellSizeCm) || CellSizeCm <= 0.0f
        || CellsPerChunkAxis < 4
        || (CellOverrides.Num() > 0
            && ((LockedCellSizeCm > 0.0f && !FMath::IsNearlyEqual(CellSizeCm, LockedCellSizeCm))
                || (LockedCellsPerChunkAxis > 0 && CellsPerChunkAxis != LockedCellsPerChunkAxis))))
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
    const FExcavationCellOverride* Override = CellOverrides.Find(Cell);
    if (!Override || !IsValidDensity(Override->Density))
    {
        return false;
    }

    OutDensity = Override->Density;
    return true;
}

bool UExcavationVolumeSubsystem::IsCellOverridden(const FIntVector& Cell) const
{
    return CellOverrides.Contains(Cell);
}

bool UExcavationVolumeSubsystem::TryGetChunkCoordinateForCell(
    const FIntVector& Cell,
    FIntVector& OutChunkCoordinate) const
{
    if (CellsPerChunkAxis < 4
        || (CellOverrides.Num() > 0 && LockedCellsPerChunkAxis > 0
            && CellsPerChunkAxis != LockedCellsPerChunkAxis))
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
    if (CellsPerChunkAxis < 4
        || (CellOverrides.Num() > 0 && LockedCellsPerChunkAxis > 0
            && CellsPerChunkAxis != LockedCellsPerChunkAxis))
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
    if (!Record.bCommitted || Record.SchemaVersion != 1 || CellsPerChunkAxis < 4
        || (CellOverrides.Num() > 0 && LockedCellsPerChunkAxis > 0
            && CellsPerChunkAxis != LockedCellsPerChunkAxis))
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

    if (!IsFiniteVector(CenterCm)
        || !FMath::IsFinite(RadiusCm)
        || RadiusCm <= 0.0f
        || !FMath::IsFinite(CellSizeCm)
        || CellSizeCm <= 0.0f
        || CellsPerChunkAxis < 4
        || MaxCellsPerEdit <= 0
        || (EditType != EExcavationEditType::Carve && EditType != EExcavationEditType::Fill))
    {
        return false;
    }

    const FVector RadiusVector(RadiusCm, RadiusCm, RadiusCm);
    FIntVector MinCell;
    FIntVector MaxCell;
    if (!TryWorldToCell(CenterCm - RadiusVector, MinCell)
        || !TryWorldToCell(CenterCm + RadiusVector, MaxCell))
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
    CandidateRecord.CenterCm = CenterCm;
    CandidateRecord.RadiusCm = RadiusCm;
    CandidateRecord.Changes.Reserve(static_cast<int32>(FMath::Min<int64>(
        CountX * CountY * CountZ,
        MaxCellsPerEdit)));

    // Build the complete change set before touching the authoritative map.
    // This makes all validation/cap failures side-effect free.
    for (int32 X = MinCell.X; ; ++X)
    {
        for (int32 Y = MinCell.Y; ; ++Y)
        {
            for (int32 Z = MinCell.Z; ; ++Z)
            {
                const FIntVector Cell(X, Y, Z);
                const FVector CellCenter = CellToWorldCenter(Cell);
                if (FVector::DistSquared(CellCenter, CenterCm) <= RadiusSquared)
                {
                    const FExcavationCellOverride* Existing = CellOverrides.Find(Cell);
                    // Even if the density is unchanged, this edit takes ownership
                    // of the cell. That preserves correct LIFO rollback semantics
                    // for overlapping/repeated edits of the same shape.
                    FExcavationCellChange Change;
                    Change.Cell = Cell;
                    Change.bHadPreviousOverride = Existing != nullptr;
                    Change.PreviousDensity = Existing ? Existing->Density : 1.0f;
                    Change.PreviousOperationId = Existing ? Existing->LastOperationId : FGuid();
                    Change.NewDensity = TargetDensity;
                    CandidateRecord.Changes.Add(Change);
                }

                if (Z == MaxCell.Z) break;
            }
            if (Y == MaxCell.Y) break;
        }
        if (X == MaxCell.X) break;
    }

    if (CellOverrides.Num() == 0 && CandidateRecord.Changes.Num() > 0)
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

    // An idempotent edit that changes no cells is still a valid committed operation.
    CandidateRecord.bCommitted = true;
    OutRecord = MoveTemp(CandidateRecord);
    return true;
}

bool UExcavationVolumeSubsystem::RollbackEdit(const FExcavationEditRecord& Record)
{
    if (!Record.bCommitted || !Record.OperationId.IsValid() || Record.SchemaVersion != 1)
    {
        return false;
    }

    TSet<FIntVector> SeenCells;
    SeenCells.Reserve(Record.Changes.Num());

    // Validate every cell first so rollback is all-or-nothing. If a later edit
    // owns even one affected cell, this older edit cannot overwrite that work.
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
        if (!Current || Current->LastOperationId != Record.OperationId)
        {
            return false;
        }
    }

    for (const FExcavationCellChange& Change : Record.Changes)
    {
        if (Change.bHadPreviousOverride)
        {
            FExcavationCellOverride Restored;
            Restored.Density = Change.PreviousDensity;
            Restored.LastOperationId = Change.PreviousOperationId;
            CellOverrides.Add(Change.Cell, Restored);
        }
        else
        {
            CellOverrides.Remove(Change.Cell);
            const int32 ChunkSizeForIndex = LockedCellsPerChunkAxis > 0
                ? LockedCellsPerChunkAxis
                : CellsPerChunkAxis;
            const FIntVector Chunk = ChunkForCellAtSize(Change.Cell, ChunkSizeForIndex);
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

    return true;
}

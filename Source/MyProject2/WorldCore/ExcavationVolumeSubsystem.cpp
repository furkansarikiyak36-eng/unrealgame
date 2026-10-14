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
    Super::Deinitialize();
}

bool UExcavationVolumeSubsystem::TryWorldToCell(const FVector& WorldPositionCm, FIntVector& OutCell) const
{
    if (!IsFiniteVector(WorldPositionCm) || !FMath::IsFinite(CellSizeCm) || CellSizeCm <= 0.0f)
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
    if (!FMath::IsFinite(CellSizeCm) || CellSizeCm <= 0.0f)
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
                    if (!Existing || !FMath::IsNearlyEqual(Existing->Density, TargetDensity))
                    {
                        FExcavationCellChange Change;
                        Change.Cell = Cell;
                        Change.bHadPreviousOverride = Existing != nullptr;
                        Change.PreviousDensity = Existing ? Existing->Density : 1.0f;
                        Change.PreviousOperationId = Existing ? Existing->LastOperationId : FGuid();
                        Change.NewDensity = TargetDensity;
                        CandidateRecord.Changes.Add(Change);
                    }
                }

                if (Z == MaxCell.Z) break;
            }
            if (Y == MaxCell.Y) break;
        }
        if (X == MaxCell.X) break;
    }

    for (const FExcavationCellChange& Change : CandidateRecord.Changes)
    {
        FExcavationCellOverride NewOverride;
        NewOverride.Density = Change.NewDensity;
        NewOverride.LastOperationId = CandidateRecord.OperationId;
        CellOverrides.Add(Change.Cell, NewOverride);
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
        }
    }

    return true;
}

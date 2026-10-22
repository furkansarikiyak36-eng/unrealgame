#include "VoxelEditService.h"

#include "WorldCore/Classification/LandscapeBuilderSubsystem.h"
#include "WorldCore/ExcavationVolumeSubsystem.h"

bool UVoxelEditService::EnsureServices(ULandscapeBuilderSubsystem*& OutBuilder, UExcavationVolumeSubsystem*& OutVolume)
{
    OutBuilder = nullptr;
    OutVolume = nullptr;
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }
    OutBuilder = World->GetSubsystem<ULandscapeBuilderSubsystem>();
    OutVolume = World->GetSubsystem<UExcavationVolumeSubsystem>();
    if (!OutBuilder || !OutVolume)
    {
        return false;
    }
    if (!OutBuilder->HasBuiltSnapshot())
    {
        OutBuilder->BuildClassificationSnapshot();
    }
    return true;
}

bool UVoxelEditService::EnsureServices(ULandscapeBuilderSubsystem*& OutBuilder, UExcavationVolumeSubsystem*& OutVolume)
{
    OutBuilder = nullptr;
    OutVolume = nullptr;
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }
    OutBuilder = World->GetSubsystem<ULandscapeBuilderSubsystem>();
    OutVolume = World->GetSubsystem<UExcavationVolumeSubsystem>();
    if (!OutBuilder || !OutVolume)
    {
        return false;
    }
    if (!OutBuilder->HasBuiltSnapshot())
    {
        OutBuilder->BuildClassificationSnapshot();
    }
    return true;
}

bool UVoxelEditService::ValidateEditableShape(EExcavationShapeType ShapeType, FVector CenterCm,
    FVector EndCm, FVector HalfExtentCm, float RadiusCm, FString& OutFailure)
{
    ULandscapeBuilderSubsystem* Builder = nullptr;
    UExcavationVolumeSubsystem* Volume = nullptr;
    if (!EnsureServices(Builder, Volume))
    {
        OutFailure = TEXT("World classification or voxel data subsystem is unavailable.");
        return false;
    }

    const float CellSize = Volume->CellSizeCm;
    if (!FMath::IsFinite(CellSize) || CellSize < 1.0f)
    {
        OutFailure = TEXT("Voxel cell size is invalid.");
        return false;
    }

    FVector MinCm;
    FVector MaxCm;
    if (ShapeType == EExcavationShapeType::Sphere)
    {
        const FVector R(RadiusCm);
        MinCm = CenterCm - R;
        MaxCm = CenterCm + R;
    }
    else if (ShapeType == EExcavationShapeType::Capsule)
    {
        const FVector R(RadiusCm);
        MinCm = FVector(FMath::Min(CenterCm.X, EndCm.X), FMath::Min(CenterCm.Y, EndCm.Y),
            FMath::Min(CenterCm.Z, EndCm.Z)) - R;
        MaxCm = FVector(FMath::Max(CenterCm.X, EndCm.X), FMath::Max(CenterCm.Y, EndCm.Y),
            FMath::Max(CenterCm.Z, EndCm.Z)) + R;
    }
    else if (ShapeType == EExcavationShapeType::Box)
    {
        MinCm = CenterCm - HalfExtentCm;
        MaxCm = CenterCm + HalfExtentCm;
    }
    else
    {
        OutFailure = TEXT("Unsupported edit shape.");
        return false;
    }

    FIntVector MinCell, MaxCell;
    if (!Volume->TryWorldToCell(MinCm, MinCell) || !Volume->TryWorldToCell(MaxCm, MaxCell))
    {
        OutFailure = TEXT("Edit bounds cannot be mapped to voxel cells.");
        return false;
    }

    const int64 CountX = static_cast<int64>(MaxCell.X) - MinCell.X + 1;
    const int64 CountY = static_cast<int64>(MaxCell.Y) - MinCell.Y + 1;
    const int64 CountZ = static_cast<int64>(MaxCell.Z) - MinCell.Z + 1;
    if (CountX <= 0 || CountY <= 0 || CountZ <= 0
        || CountX > Volume->MaxCellsPerEdit
        || CountY > Volume->MaxCellsPerEdit / CountX
        || CountZ > Volume->MaxCellsPerEdit / (CountX * CountY))
    {
        OutFailure = TEXT("Edit is too large to preflight safely.");
        return false;
    }

    const double RadiusSquared = static_cast<double>(RadiusCm) * RadiusCm;
    auto DistanceSquaredToSegment = [](const FVector& Point, const FVector& A, const FVector& B)
    {
        const FVector Segment = B - A;
        const double LengthSquared = Segment.SizeSquared();
        if (LengthSquared <= KINDA_SMALL_NUMBER)
        {
            return FVector::DistSquared(Point, A);
        }
        const double T = FMath::Clamp(FVector::DotProduct(Point - A, Segment) / LengthSquared, 0.0, 1.0);
        return FVector::DistSquared(Point, A + Segment * T);
    };

    // Match the edit core's cell-centre inclusion rule. Only voxels that the edit
    // would actually touch are required to be editable, preventing edits through static rock.
    for (int32 X = MinCell.X; ; ++X)
    {
        for (int32 Y = MinCell.Y; ; ++Y)
        {
            for (int32 Z = MinCell.Z; ; ++Z)
            {
                const FIntVector Cell(X, Y, Z);
                const FVector P = Volume->CellToWorldCenter(Cell);
                bool bInside = false;
                switch (ShapeType)
                {
                case EExcavationShapeType::Sphere:
                    bInside = FVector::DistSquared(P, CenterCm) <= RadiusSquared;
                    break;
                case EExcavationShapeType::Capsule:
                    bInside = DistanceSquaredToSegment(P, CenterCm, EndCm) <= RadiusSquared;
                    break;
                case EExcavationShapeType::Box:
                    bInside = FMath::Abs(P.X - CenterCm.X) <= HalfExtentCm.X
                        && FMath::Abs(P.Y - CenterCm.Y) <= HalfExtentCm.Y
                        && FMath::Abs(P.Z - CenterCm.Z) <= HalfExtentCm.Z;
                    break;
                default:
                    break;
                }

                if (bInside && Builder->QueryModeAtWorld(P).Mode != EWorldTerrainMode::EditableVoxel)
                {
                    const EWorldTerrainMode Mode = Builder->QueryModeAtWorld(P).Mode;
                    OutFailure = Mode == EWorldTerrainMode::StaticGeometry
                        ? TEXT("Edit intersects protected static geometry.")
                        : TEXT("Edit intersects a region not classified as editable voxel terrain.");
                    return false;
                }

                if (Z == MaxCell.Z) break;
            }
            if (Y == MaxCell.Y) break;
        }
        if (X == MaxCell.X) break;
    }
    return true;
}

bool UVoxelEditService::CarveSphere(FVector CenterCm, float RadiusCm,
    FExcavationEditRecord& OutRecord, FString& OutFailure)
{
    OutRecord = FExcavationEditRecord();
    OutFailure.Reset();
    if (!FMath::IsFinite(CenterCm.X) || !FMath::IsFinite(CenterCm.Y) || !FMath::IsFinite(CenterCm.Z)
        || !FMath::IsFinite(RadiusCm) || RadiusCm <= 0.0f)
    {
        OutFailure = TEXT("Sphere centre/radius is invalid.");
        return false;
    }

    if (!ValidateEditableShape(EExcavationShapeType::Sphere, CenterCm, FVector::ZeroVector,
        FVector::ZeroVector, RadiusCm, OutFailure))
    {
        return false;
    }

    UWorld* World = GetWorld();
    UExcavationVolumeSubsystem* Volume = World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!Volume || !Volume->ApplySphereEdit(CenterCm, RadiusCm, EExcavationEditType::Carve, OutRecord))
    {
        OutFailure = TEXT("Voxel edit rejected by grid limits or transaction validation.");
        return false;
    }
    return true;
}

bool UVoxelEditService::CarveCapsule(FVector StartCm, FVector EndCm, float RadiusCm,
    FExcavationEditRecord& OutRecord, FString& OutFailure)
{
    OutRecord = FExcavationEditRecord();
    OutFailure.Reset();
    if (!FMath::IsFinite(StartCm.X) || !FMath::IsFinite(StartCm.Y) || !FMath::IsFinite(StartCm.Z)
        || !FMath::IsFinite(EndCm.X) || !FMath::IsFinite(EndCm.Y) || !FMath::IsFinite(EndCm.Z)
        || !FMath::IsFinite(RadiusCm) || RadiusCm <= 0.0f)
    {
        OutFailure = TEXT("Capsule endpoints/radius are invalid.");
        return false;
    }

    if (!ValidateEditableShape(EExcavationShapeType::Capsule, StartCm, EndCm,
        FVector::ZeroVector, RadiusCm, OutFailure))
    {
        return false;
    }

    UWorld* World = GetWorld();
    UExcavationVolumeSubsystem* Volume = World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!Volume || !Volume->ApplyCapsuleEdit(StartCm, EndCm, RadiusCm, EExcavationEditType::Carve, OutRecord))
    {
        OutFailure = TEXT("Voxel capsule edit failed validation or exceeded its cell budget.");
        return false;
    }
    return true;
}

bool UVoxelEditService::CarveBox(FVector MinCm, FVector MaxCm,
    FExcavationEditRecord& OutRecord, FString& OutFailure)
{
    OutRecord = FExcavationEditRecord();
    OutFailure.Reset();
    if (!FMath::IsFinite(MinCm.X) || !FMath::IsFinite(MinCm.Y) || !FMath::IsFinite(MinCm.Z)
        || !FMath::IsFinite(MaxCm.X) || !FMath::IsFinite(MaxCm.Y) || !FMath::IsFinite(MaxCm.Z)
        || MaxCm.X <= MinCm.X || MaxCm.Y <= MinCm.Y || MaxCm.Z <= MinCm.Z)
    {
        OutFailure = TEXT("Box bounds must be finite and have positive volume.");
        return false;
    }

    const FVector Extent = (MaxCm - MinCm) * 0.5f;
    const FVector Center = MinCm + Extent;
    if (!ValidateEditableShape(EExcavationShapeType::Box, Center, FVector::ZeroVector,
        Extent, 0.0f, OutFailure))
    {
        return false;
    }

    UWorld* World = GetWorld();
    UExcavationVolumeSubsystem* Volume = World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!Volume || !Volume->ApplyBoxEdit(MinCm, MaxCm, EExcavationEditType::Carve, OutRecord))
    {
        OutFailure = TEXT("Voxel box edit failed validation or exceeded its cell budget.");
        return false;
    }
    return true;
}

bool UVoxelEditService::FillSphere(FVector CenterCm, float RadiusCm,
    FExcavationEditRecord& OutRecord, FString& OutFailure)
{
    OutRecord = FExcavationEditRecord();
    OutFailure.Reset();
    if (!FMath::IsFinite(CenterCm.X) || !FMath::IsFinite(CenterCm.Y) || !FMath::IsFinite(CenterCm.Z)
        || !FMath::IsFinite(RadiusCm) || RadiusCm <= 0.0f)
    {
        OutFailure = TEXT("Sphere centre/radius is invalid.");
        return false;
    }

    if (!ValidateEditableShape(EExcavationShapeType::Sphere, CenterCm, FVector::ZeroVector,
        FVector::ZeroVector, RadiusCm, OutFailure))
    {
        return false;
    }

    UWorld* World = GetWorld();
    UExcavationVolumeSubsystem* Volume = World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!Volume || !Volume->ApplySphereEdit(CenterCm, RadiusCm, EExcavationEditType::Fill, OutRecord))
    {
        OutFailure = TEXT("Voxel fill failed validation or exceeded its cell budget.");
        return false;
    }
    return true;
}

#include "WorldRuntimeProfile.h"
#include "ExcavationVolumeSubsystem.h"

float UWorldRuntimeProfileSettings::GetCellSizeCm() const
{
    return ActiveProfile == EWorldRuntimeProfile::HighEnd
        ? FMath::Max(1.0f, HighEndCellSizeCm)
        : FMath::Max(1.0f, LowMidCellSizeCm);
}

int32 UWorldRuntimeProfileSettings::GetCellsPerChunkAxis() const
{
    return ActiveProfile == EWorldRuntimeProfile::HighEnd
        ? FMath::Max(4, HighEndCellsPerChunkAxis)
        : FMath::Max(4, LowMidCellsPerChunkAxis);
}

int32 UWorldRuntimeProfileSettings::GetMaxMeshJobsInFlight() const
{
    return ActiveProfile == EWorldRuntimeProfile::HighEnd
        ? FMath::Clamp(HighEndMaxMeshJobsInFlight, 1, 16)
        : FMath::Clamp(LowMidMaxMeshJobsInFlight, 1, 8);
}

float UWorldRuntimeProfileSettings::GetChunkSizeCm() const
{
    return GetCellSizeCm() * GetCellsPerChunkAxis();
}

bool UWorldRuntimeProfileSettings::ApplyTo(UExcavationVolumeSubsystem* Volume) const
{
    if (!IsValid(Volume))
    {
        return false;
    }

    if (Volume->GetOverrideCellCount() > 0)
    {
        return FMath::IsNearlyEqual(Volume->CellSizeCm, GetCellSizeCm())
            && Volume->CellsPerChunkAxis == GetCellsPerChunkAxis();
    }

    Volume->CellSizeCm = GetCellSizeCm();
    Volume->CellsPerChunkAxis = GetCellsPerChunkAxis();
    Volume->MaxCellsPerEdit = FMath::Max(1, MaxCellsPerEdit);
    Volume->MaxSnapshotOverrideCells = FMath::Max(1, MaxSnapshotOverrideCells);
    // No provider must not make the whole world solid by accident.
    Volume->DefaultBaseDensity = 0.0f;
    return true;
}

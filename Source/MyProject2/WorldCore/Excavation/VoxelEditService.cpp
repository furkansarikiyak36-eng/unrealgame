#include "VoxelEditService.h"

#include "LandscapeBuilderSubsystem.h"
#include "ExcavationVolumeSubsystem.h"
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
    if (OutBuilder->GetRegionCount() == 0)
    {
        OutBuilder->BuildClassificationSnapshot();
    }
    return true;
}

bool UVoxelEditService::IsEditablePoint(FVector PointCm, FString& OutFailure)
{
    ULandscapeBuilderSubsystem* Builder = nullptr;
    UExcavationVolumeSubsystem* Volume = nullptr;
    if (!EnsureServices(Builder, Volume))
    {
        OutFailure = TEXT("World classification or voxel data subsystem is unavailable.");
        return false;
    }

    const FWorldClassificationResult Classification = Builder->QueryModeAtWorld(PointCm);
    if (Classification.Mode != EWorldTerrainMode::EditableVoxel)
    {
        OutFailure = Classification.Mode == EWorldTerrainMode::StaticGeometry
            ? TEXT("This region is protected static geometry.")
            : TEXT("This region has not been classified as editable voxel terrain.");
        return false;
    }

    return true;
}

bool UVoxelEditService::CarveSphere(FVector CenterCm, float RadiusCm, FExcavationEditRecord& OutRecord, FString& OutFailure)
{
    OutRecord = FExcavationEditRecord();
    OutFailure.Reset();
    if (!FMath::IsFinite(RadiusCm) || RadiusCm <= 0.0f)
    {
        OutFailure = TEXT("Radius must be positive and finite.");
        return false;
    }

    const FVector R(RadiusCm, RadiusCm, RadiusCm);
    const FVector Probes[] = {
        CenterCm, CenterCm + FVector(RadiusCm,0,0), CenterCm - FVector(RadiusCm,0,0),
        CenterCm + FVector(0,RadiusCm,0), CenterCm - FVector(0,RadiusCm,0),
        CenterCm + FVector(0,0,RadiusCm), CenterCm - FVector(0,0,RadiusCm)
    };
    for (const FVector& Probe : Probes)
    {
        if (!IsEditablePoint(Probe, OutFailure)) return false;
    }

    ULandscapeBuilderSubsystem* Builder = nullptr;
    UExcavationVolumeSubsystem* Volume = nullptr;
    if (!EnsureServices(Builder, Volume))
    {
        OutFailure = TEXT("Voxel subsystem unavailable.");
        return false;
    }
    if (!Volume->ApplySphereEdit(CenterCm, RadiusCm, EExcavationEditType::Carve, OutRecord))
    {
        OutFailure = TEXT("Voxel edit rejected by grid limits or transaction validation.");
        return false;
    }
    return true;
}

bool UVoxelEditService::CarveCapsule(FVector StartCm, FVector EndCm, float RadiusCm, FExcavationEditRecord& OutRecord, FString& OutFailure)
{
    OutRecord = FExcavationEditRecord();
    OutFailure.Reset();
    if (!FMath::IsFinite(RadiusCm) || RadiusCm <= 0.0f)
    {
        OutFailure = TEXT("Radius must be positive and finite.");
        return false;
    }
    const FVector Mid = (StartCm + EndCm) * 0.5f;
    const FVector R(RadiusCm, RadiusCm, RadiusCm);
    const FVector Probes[] = {
        StartCm, EndCm, Mid,
        Mid + FVector(RadiusCm,0,0), Mid - FVector(RadiusCm,0,0),
        Mid + FVector(0,RadiusCm,0), Mid - FVector(0,RadiusCm,0),
        Mid + FVector(0,0,RadiusCm), Mid - FVector(0,0,RadiusCm)
    };
    for (const FVector& Probe : Probes)
    {
        if (!IsEditablePoint(Probe, OutFailure)) return false;
    }

    ULandscapeBuilderSubsystem* Builder = nullptr;
    UExcavationVolumeSubsystem* Volume = nullptr;
    if (!EnsureServices(Builder, Volume) || !Volume->ApplyCapsuleEdit(StartCm, EndCm, RadiusCm, EExcavationEditType::Carve, OutRecord))
    {
        OutFailure = TEXT("Voxel capsule edit failed validation or exceeded its cell budget.");
        return false;
    }
    return true;
}

bool UVoxelEditService::CarveBox(FVector MinCm, FVector MaxCm, FExcavationEditRecord& OutRecord, FString& OutFailure)
{
    OutRecord = FExcavationEditRecord();
    OutFailure.Reset();
    if (MaxCm.X <= MinCm.X || MaxCm.Y <= MinCm.Y || MaxCm.Z <= MinCm.Z)
    {
        OutFailure = TEXT("Box bounds must have positive volume.");
        return false;
    }

    for (int32 X = 0; X < 3; ++X)
    {
        for (int32 Y = 0; Y < 3; ++Y)
        {
            for (int32 Z = 0; Z < 3; ++Z)
            {
                const FVector P(
                    FMath::Lerp(MinCm.X, MaxCm.X, X * 0.5f),
                    FMath::Lerp(MinCm.Y, MaxCm.Y, Y * 0.5f),
                    FMath::Lerp(MinCm.Z, MaxCm.Z, Z * 0.5f));
                if (!IsEditablePoint(P, OutFailure)) return false;
            }
        }
    }

    ULandscapeBuilderSubsystem* Builder = nullptr;
    UExcavationVolumeSubsystem* Volume = nullptr;
    if (!EnsureServices(Builder, Volume) || !Volume->ApplyBoxEdit(MinCm, MaxCm, EExcavationEditType::Carve, OutRecord))
    {
        OutFailure = TEXT("Voxel box edit failed validation or exceeded its cell budget.");
        return false;
    }
    return true;
}

bool UVoxelEditService::FillSphere(FVector CenterCm, float RadiusCm, FExcavationEditRecord& OutRecord, FString& OutFailure)
{
    OutRecord = FExcavationEditRecord();
    OutFailure.Reset();
    ULandscapeBuilderSubsystem* Builder = nullptr;
    UExcavationVolumeSubsystem* Volume = nullptr;
    if (!EnsureServices(Builder, Volume))
    {
        OutFailure = TEXT("Voxel subsystem unavailable.");
        return false;
    }
    if (!Volume->ApplySphereEdit(CenterCm, RadiusCm, EExcavationEditType::Fill, OutRecord))
    {
        OutFailure = TEXT("Voxel fill failed validation or exceeded its cell budget.");
        return false;
    }
    return true;
}

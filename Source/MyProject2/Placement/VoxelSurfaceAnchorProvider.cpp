#include "VoxelSurfaceAnchorProvider.h"

#include "WorldScanSystem.h"
#include "LandscapeBuilderSubsystem.h"
#include "WorldCore/Classification/LandscapeBuilderSubsystem.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"

bool UVoxelSurfaceAnchorProvider::TraceEditableSurface(FVector StartCm, FVector EndCm,
    float MaxAngleFromUpDegrees, FVoxelSurfaceAnchor& OutAnchor, FString& OutFailure) const
{
    OutAnchor = FVoxelSurfaceAnchor();
    OutFailure.Reset();
    UWorld* World = GetWorld();
    if (!World || !FMath::IsFinite(MaxAngleFromUpDegrees) || MaxAngleFromUpDegrees < 0.0f)
    {
        OutFailure = TEXT("Invalid world or surface angle threshold.");
        return false;
    }

    WorldScanSystem::FScanResult Scan;
    if (!WorldScanSystem::Trace(World, StartCm, EndCm, nullptr, Scan, ECC_Visibility, true))
    {
        OutFailure = TEXT("Scan did not hit a surface.");
        return false;
    }

    WorldScanSystem::FWorldAnalysis Analysis;
    if (!WorldScanSystem::AnalyzeHit(Scan, Analysis))
    {
        OutFailure = TEXT("Hit analysis failed.");
        return false;
    }

    ULandscapeBuilderSubsystem* Builder = World->GetSubsystem<ULandscapeBuilderSubsystem>();
    if (!Builder)
    {
        OutFailure = TEXT("Landscape classification is unavailable.");
        return false;
    }
    const FWorldClassificationResult Classification = Builder->QueryModeAtWorld(Analysis.Point);
    if (Classification.Mode != EWorldTerrainMode::EditableVoxel)
    {
        OutFailure = Classification.Mode == EWorldTerrainMode::StaticGeometry
            ? TEXT("Target is protected static geometry.")
            : TEXT("Target is not in a classified editable voxel region.");
        return false;
    }

    const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(
        FVector::DotProduct(Analysis.Normal.GetSafeNormal(), FVector::UpVector), -1.0f, 1.0f)));
    if (Angle > MaxAngleFromUpDegrees)
    {
        OutFailure = TEXT("Surface normal is outside the allowed angle.");
        return false;
    }

    OutAnchor.PointCm = Analysis.Point;
    OutAnchor.Normal = Analysis.Normal.GetSafeNormal();
    OutAnchor.DistanceCm = Analysis.HitDistanceCm;
    OutAnchor.TargetActor = Analysis.Actor.Get();
    OutAnchor.TargetComponent = Analysis.Component.Get();
    OutAnchor.bValid = true;
    return true;
}

bool UVoxelSurfaceAnchorProvider::MakeSurfaceAlignedTransform(const FVoxelSurfaceAnchor& Anchor,
    FVector LocalOffsetCm, FTransform& OutTransform) const
{
    OutTransform = FTransform::Identity;
    if (!Anchor.bValid || Anchor.Normal.IsNearlyZero()
        || !FMath::IsFinite(LocalOffsetCm.X) || !FMath::IsFinite(LocalOffsetCm.Y) || !FMath::IsFinite(LocalOffsetCm.Z))
    {
        return false;
    }

    const FRotationMatrix Rotation = FRotationMatrix::MakeFromZ(Anchor.Normal.GetSafeNormal());
    const FVector Location = Anchor.PointCm + Rotation.TransformVector(LocalOffsetCm);
    OutTransform = FTransform(Rotation.ToQuat(), Location, FVector::OneVector);
    return true;
}

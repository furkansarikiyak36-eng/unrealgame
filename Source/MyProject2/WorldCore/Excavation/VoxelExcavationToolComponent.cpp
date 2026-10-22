#include "VoxelExcavationToolComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "WorldScanSystem.h"
#include "WorldCore/Excavation/VoxelEditService.h"
#include "WorldCore/ExcavationVolumeSubsystem.h"
#include "Survival/ExcavationSurvivalAdapterComponent.h"

UVoxelExcavationToolComponent::UVoxelExcavationToolComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UVoxelExcavationToolComponent::PayStaminaOrRollback(
    const FExcavationEditRecord& Record, float Cost, FString& OutFailure)
{
    AActor* Owner = GetOwner();
    UWorld* World = GetWorld();
    if (!Owner || !World)
    {
        OutFailure = TEXT("Excavation owner or world is invalid.");
        return false;
    }

    UExcavationSurvivalAdapterComponent* SurvivalAdapter =
        Owner->FindComponentByClass<UExcavationSurvivalAdapterComponent>();
    if (!SurvivalAdapter)
    {
        if (bRequirePlayerSurvivalAdapter)
        {
            if (UExcavationVolumeSubsystem* Volume = World->GetSubsystem<UExcavationVolumeSubsystem>())
            {
                Volume->RollbackEdit(Record);
            }
            OutFailure = TEXT("Required survival adapter is missing; voxel edit was rolled back.");
            return false;
        }
        return true;
    }

    if (SurvivalAdapter->TryPayWorkCost(Cost, OutFailure))
    {
        return true;
    }

    if (UExcavationVolumeSubsystem* Volume = World->GetSubsystem<UExcavationVolumeSubsystem>())
    {
        Volume->RollbackEdit(Record);
    }
    OutFailure += TEXT(" Voxel edit was rolled back.");
    return false;
}

bool UVoxelExcavationToolComponent::ExcavateFromView(float ReachCm, float RadiusCm, FString& OutFailure)
{
    OutFailure.Reset();
    AActor* Owner = GetOwner();
    UWorld* World = GetWorld();
    if (!Owner || !World || !FMath::IsFinite(ReachCm) || !FMath::IsFinite(RadiusCm)
        || ReachCm <= 0.0f || RadiusCm <= 0.0f)
    {
        OutFailure = TEXT("Invalid excavation owner, reach or radius.");
        return false;
    }

    FVector ViewLocation;
    FRotator ViewRotation;
    if (const UCameraComponent* Camera = Owner->FindComponentByClass<UCameraComponent>())
    {
        ViewLocation = Camera->GetComponentLocation();
        ViewRotation = Camera->GetComponentRotation();
    }
    else
    {
        Owner->GetActorEyesViewPoint(ViewLocation, ViewRotation);
    }

    WorldScanSystem::FScanResult Scan;
    if (!WorldScanSystem::Trace(World, ViewLocation, ViewLocation + ViewRotation.Vector() * ReachCm,
        Owner, Scan, ECC_Visibility, true))
    {
        OutFailure = TEXT("No excavation surface was hit.");
        return false;
    }

    WorldScanSystem::FWorldAnalysis Analysis;
    if (!WorldScanSystem::AnalyzeHit(Scan, Analysis))
    {
        OutFailure = TEXT("Could not analyze the scanned surface.");
        return false;
    }

    // Shift the sphere into the hit surface so a camera trace on the boundary removes
    // a shallow scoop instead of placing half of the operation entirely in empty space.
    const FVector InwardCenter = Analysis.Point - Analysis.Normal.GetSafeNormal() * (RadiusCm * 0.55f);
    UWorld* OwningWorld = GetWorld();
    UVoxelEditService* Edits = OwningWorld ? OwningWorld->GetSubsystem<UVoxelEditService>() : nullptr;
    if (!Edits)
    {
        OutFailure = TEXT("Voxel edit service is unavailable.");
        return false;
    }

    FExcavationEditRecord Record;
    if (!Edits->CarveSphere(InwardCenter, RadiusCm, Record, OutFailure))
    {
        return false;
    }

    return PayStaminaOrRollback(Record, FMath::Max(0.0f, DigStaminaCost), OutFailure);
}

bool UVoxelExcavationToolComponent::ExcavateTunnelBetween(
    FVector StartCm, FVector EndCm, float RadiusCm, FString& OutFailure)
{
    OutFailure.Reset();
    if (!FMath::IsFinite(StartCm.X) || !FMath::IsFinite(StartCm.Y) || !FMath::IsFinite(StartCm.Z)
        || !FMath::IsFinite(EndCm.X) || !FMath::IsFinite(EndCm.Y) || !FMath::IsFinite(EndCm.Z)
        || !FMath::IsFinite(RadiusCm) || RadiusCm <= 0.0f)
    {
        OutFailure = TEXT("Tunnel endpoints or radius are invalid.");
        return false;
    }

    UWorld* World = GetWorld();
    UVoxelEditService* Edits = World ? World->GetSubsystem<UVoxelEditService>() : nullptr;
    if (!Edits)
    {
        OutFailure = TEXT("Voxel edit service is unavailable.");
        return false;
    }

    FExcavationEditRecord Record;
    if (!Edits->CarveCapsule(StartCm, EndCm, RadiusCm, Record, OutFailure))
    {
        return false;
    }

    const float LengthMeters = FVector::Distance(StartCm, EndCm) / 100.0f;
    const float Cost = FMath::Max(0.0f, LengthMeters * TunnelStaminaCostPerMeter);
    return PayStaminaOrRollback(Record, Cost, OutFailure);
}

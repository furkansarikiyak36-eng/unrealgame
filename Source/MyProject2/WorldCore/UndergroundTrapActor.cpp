#include "UndergroundTrapActor.h"

#include "Components/BoxComponent.h"
#include "WorldCore/ExcavationVolumeSubsystem.h"
#include "UndergroundHazardSubsystem.h"
#include "Engine/World.h"

AUndergroundTrapActor::AUndergroundTrapActor()
{
    TriggerVolume->SetBoxExtent(FVector(100.0f, 100.0f, 70.0f));
}

bool AUndergroundTrapActor::HasValidCavityClearance() const
{
    if (!bRequireCavityClearance)
    {
        return true;
    }

    UWorld* World = GetWorld();
    UExcavationVolumeSubsystem* Excavation =
        World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!Excavation || !TriggerVolume || !FMath::IsFinite(MinimumCavityClearanceCm)
        || MinimumCavityClearanceCm <= 0.0f)
    {
        return false;
    }

    const float CellSize = FMath::Max(1.0f, Excavation->CellSizeCm);
    const FVector Origin = TriggerVolume->GetComponentLocation();
    const int32 Samples = FMath::Clamp(
        FMath::CeilToInt(MinimumCavityClearanceCm / CellSize), 1, 32);

    for (int32 Index = 0; Index < Samples; ++Index)
    {
        const FVector SamplePoint = Origin + FVector(0.0, 0.0, (Index + 1) * CellSize);
        float Density = 1.0f;
        if (!Excavation->TrySampleDensityAtWorld(SamplePoint, Density) || Density >= 0.5f)
        {
            return false;
        }
    }

    return true;
}

void AUndergroundTrapActor::Trigger(AActor* Victim)
{
    if (!HasValidCavityClearance())
    {
        return;
    }

    Super::Trigger(Victim);
}

bool AUndergroundTrapActor::TriggerFromWaterPocket(AActor* Victim, float MinimumWaterVolumeM3)
{
    if (!HasValidCavityClearance() || !FMath::IsFinite(MinimumWaterVolumeM3)
        || MinimumWaterVolumeM3 < 0.0f)
    {
        return false;
    }

    UWorld* World = GetWorld();
    UExcavationVolumeSubsystem* Excavation =
        World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    UUndergroundHazardSubsystem* Hazards =
        World ? World->GetSubsystem<UUndergroundHazardSubsystem>() : nullptr;
    if (!Excavation || !Hazards || !TriggerVolume)
    {
        return false;
    }

    FIntVector Cell;
    if (!Excavation->TryWorldToCell(TriggerVolume->GetComponentLocation(), Cell))
    {
        return false;
    }

    FUndergroundWaterPocket Pocket;
    if (!Hazards->TryGetWaterPocketAtCell(Cell, Pocket)
        || Pocket.VolumeM3 < MinimumWaterVolumeM3)
    {
        return false;
    }

    const bool bWasArmed = bArmed;
    Trigger(Victim);
    return bWasArmed && !bArmed;
}

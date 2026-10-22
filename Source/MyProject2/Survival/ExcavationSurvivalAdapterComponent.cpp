#include "ExcavationSurvivalAdapterComponent.h"

#include "SurvivalComponent.h"

UExcavationSurvivalAdapterComponent::UExcavationSurvivalAdapterComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

USurvivalComponent* UExcavationSurvivalAdapterComponent::FindSurvivalComponent() const
{
    AActor* Owner = GetOwner();
    return IsValid(Owner) ? Owner->FindComponentByClass<USurvivalComponent>() : nullptr;
}

bool UExcavationSurvivalAdapterComponent::TryPayWorkCost(float EffortUnits, FString& OutFailure) const
{
    OutFailure.Reset();
    if (!FMath::IsFinite(EffortUnits) || EffortUnits < 0.0f
        || !FMath::IsFinite(StaminaCostPerEffortUnit) || StaminaCostPerEffortUnit < 0.0f)
    {
        OutFailure = TEXT("Invalid excavation effort cost.");
        return false;
    }

    if (EffortUnits <= 0.0f)
    {
        return true;
    }

    USurvivalComponent* Survival = FindSurvivalComponent();
    if (!Survival)
    {
        OutFailure = TEXT("The owner has no USurvivalComponent.");
        return false;
    }

    const float Cost = EffortUnits * StaminaCostPerEffortUnit;
    if (!Survival->TryUseStamina(Cost))
    {
        OutFailure = TEXT("Not enough stamina for this action.");
        return false;
    }
    return true;
}

bool UExcavationSurvivalAdapterComponent::SetLocalEnvironment(
    float AmbientTemperatureC, float Wetness01, float ShelterHeatC) const
{
    USurvivalComponent* Survival = FindSurvivalComponent();
    if (!Survival || !FMath::IsFinite(AmbientTemperatureC)
        || !FMath::IsFinite(Wetness01) || !FMath::IsFinite(ShelterHeatC))
    {
        return false;
    }

    Survival->SetEnvironmentalExposure(AmbientTemperatureC, Wetness01, ShelterHeatC);
    return true;
}

// PlasterComponent.cpp
#include "PlasterComponent.h"

UPlasterComponent::UPlasterComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UPlasterComponent::ApplyPlaster(UPlasterDefinition* Def, float Thickness)
{
    Definition = Def;
    AppliedThickness = FMath::Max(0.001f, Thickness);
    CureProgress = 0.0f;
    DamagePercent = 0.0f;
    bIsCured = false;
}

void UPlasterComponent::Cure(float DeltaHours)
{
    if (!Definition || bIsCured) return;
    CureProgress += FMath::Max(0.0f, DeltaHours) / FMath::Max(1.0f, Definition->CureTimeHours);
    if (CureProgress >= 1.0f)
    {
        CureProgress = 1.0f;
        bIsCured = true;
    }
}

void UPlasterComponent::ApplyWaterDamage(float Intensity, float DurationHours)
{
    if (!Definition) return;
    const float Damage = Definition->Properties.WaterDamageRate * Intensity * DurationHours * 0.1f;
    DamagePercent = FMath::Clamp(DamagePercent + Damage, 0.0f, 1.0f);
    if (OnPlasterConditionChanged.IsBound())
        OnPlasterConditionChanged.Broadcast(DamagePercent);
}

void UPlasterComponent::ApplyImpactDamage(float Magnitude)
{
    if (!Definition) return;
    const float Damage = (Magnitude / FMath::Max(1.0f, Definition->Properties.ImpactResistance)) * 0.1f;
    DamagePercent = FMath::Clamp(DamagePercent + Damage, 0.0f, 1.0f);
    if (OnPlasterConditionChanged.IsBound())
        OnPlasterConditionChanged.Broadcast(DamagePercent);
}

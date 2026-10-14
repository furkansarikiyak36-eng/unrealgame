// FireMaterialRule.cpp
#include "FireMaterialRule.h"
#include "MaterialCore/MaterialDefinition.h"

UFireMaterialRule::UFireMaterialRule()
{
    TargetClass = EMaterialClass::Wood;
    BaseThreshold = 0.1f;
    DamageMultiplier = 1.0f;
}

bool UFireMaterialRule::CanApplyTo(const UMaterialDefinition* Material) const
{
    return Material && bIsEnabled && Material->CanBurn();
}

float UFireMaterialRule::EvaluateDamage(const UMaterialDefinition* Material, const FMaterialInteractionContext& Context) const
{
    if (!Material || !bIsEnabled || !Material->CanBurn()) return 0.0f;
    if (Context.AmbientTemperature < MinTemperatureToIgnite) return 0.0f;

    // Flammability * (T - T_ignite) / T_ignite
    const float TempFactor = (Context.AmbientTemperature - MinTemperatureToIgnite) / MinTemperatureToIgnite;
    const float FireIntensity = Material->Properties.Flammability * TempFactor * BurnRateMultiplier;
    const float DurationDays = FMath::Max(0.0f, Context.DurationHours) / 24.0f;
    return FMath::Max(0.0f, FireIntensity * DurationDays);
}

EMaterialReactionType UFireMaterialRule::DetermineReaction(const UMaterialDefinition* Material, const FMaterialInteractionContext& Context) const
{
    if (!Material) return EMaterialReactionType::None;
    const float Damage = EvaluateDamage(Material, Context);
    if (Damage <= 0.0f) return EMaterialReactionType::None;
    if (Damage > CharredThreshold) return EMaterialReactionType::Char;
    return EMaterialReactionType::Burn;
}

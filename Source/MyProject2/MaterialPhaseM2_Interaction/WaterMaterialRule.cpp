// WaterMaterialRule.cpp
#include "WaterMaterialRule.h"
#include "MaterialCore/MaterialDefinition.h"

UWaterMaterialRule::UWaterMaterialRule()
{
    TargetClass = EMaterialClass::Plaster;
    BaseThreshold = 0.0f;
    DamageMultiplier = 1.0f;
}

bool UWaterMaterialRule::CanApplyTo(const UMaterialDefinition* Material) const
{
    return Material && bIsEnabled && Material->IsWaterSensitive();
}

float UWaterMaterialRule::EvaluateDamage(const UMaterialDefinition* Material, const FMaterialInteractionContext& Context) const
{
    if (!Material || !bIsEnabled || !Material->IsWaterSensitive()) return 0.0f;
    if (Context.Moisture < MinWaterContact) return 0.0f;

    const float Absorption = Material->Properties.WaterAbsorption * Context.Moisture;
    const float DurationDays = FMath::Max(0.0f, Context.DurationHours) / 24.0f;
    const float Damage = Material->Properties.WaterDamageRate * Absorption * DurationDays;
    return FMath::Max(0.0f, Damage);
}

EMaterialReactionType UWaterMaterialRule::DetermineReaction(const UMaterialDefinition* Material, const FMaterialInteractionContext& Context) const
{
    if (!Material || !bIsEnabled || EvaluateDamage(Material, Context) <= 0.0f) return EMaterialReactionType::None;
    if (Material->MaterialClass == EMaterialClass::Plaster) return EMaterialReactionType::Dissolve;
    if (Material->MaterialClass == EMaterialClass::Wood && Context.Moisture > SwellThreshold)
        return EMaterialReactionType::Swell;
    return EMaterialReactionType::None;
}

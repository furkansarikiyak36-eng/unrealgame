// TimeMaterialRule.cpp
#include "TimeMaterialRule.h"
#include "MaterialCore/MaterialDefinition.h"

UTimeMaterialRule::UTimeMaterialRule()
{
    TargetClass = EMaterialClass::Wood;
    BaseThreshold = 0.0f;
    DamageMultiplier = 1.0f;
}

bool UTimeMaterialRule::CanApplyTo(const UMaterialDefinition* Material) const
{
    return Material && bIsEnabled && (Material->CanRot() || Material->MaterialClass == EMaterialClass::Metal);
}

float UTimeMaterialRule::EvaluateDamage(const UMaterialDefinition* Material, const FMaterialInteractionContext& Context) const
{
    if (!Material || !bIsEnabled || (!Material->CanRot() && Material->MaterialClass != EMaterialClass::Metal)) return 0.0f;

    // Sıcaklık ve nem faktörleri
    const float TempFactor = FMath::Clamp((Context.AmbientTemperature + 5.0f) / 25.0f, 0.1f, 2.0f);
    const float MoistFactor = FMath::Clamp(Context.Moisture * 2.0f, 0.1f, 2.0f);

    const float RatePerDay = Material->MaterialClass == EMaterialClass::Metal
        ? Material->Properties.WaterDamageRate : Material->Properties.DecayRate;
    const float DurationDays = FMath::Max(0.0f, Context.DurationHours) / 24.0f;
    return RatePerDay * TempFactor * MoistFactor * DurationDays * DamageMultiplier;
}

EMaterialReactionType UTimeMaterialRule::DetermineReaction(const UMaterialDefinition* Material, const FMaterialInteractionContext& Context) const
{
    if (!Material || !bIsEnabled || EvaluateDamage(Material, Context) <= 0.0f) return EMaterialReactionType::None;
    if (Material->CanRot()) return EMaterialReactionType::Rot;
    if (Material->MaterialClass == EMaterialClass::Metal) return EMaterialReactionType::Rust;
    return EMaterialReactionType::Erode;
}

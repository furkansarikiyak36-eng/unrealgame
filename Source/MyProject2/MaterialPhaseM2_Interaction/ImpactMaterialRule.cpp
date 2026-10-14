// ImpactMaterialRule.cpp
#include "ImpactMaterialRule.h"
#include "MaterialCore/MaterialDefinition.h"

UImpactMaterialRule::UImpactMaterialRule()
{
    TargetClass = EMaterialClass::Stone;
    BaseThreshold = 5.0f;
    DamageMultiplier = 1.0f;
}

float UImpactMaterialRule::EvaluateDamage(const UMaterialDefinition* Material, const FMaterialInteractionContext& Context) const
{
    if (!Material || !bIsEnabled) return 0.0f;
    const float Resistance = Material->Properties.ImpactResistance;
    const float Effective = FMath::Max(0.0f, Context.Magnitude - Resistance * 0.5f);
    return Effective * DamageMultiplier * (1.0f + Material->Properties.Brittleness);
}

EMaterialReactionType UImpactMaterialRule::DetermineReaction(const UMaterialDefinition* Material, const FMaterialInteractionContext& Context) const
{
    if (!Material || !bIsEnabled) return EMaterialReactionType::None;
    const float Damage = EvaluateDamage(Material, Context);
    const float NormalizedDamage = Damage / FMath::Max(1.0f, Material->Properties.ImpactResistance);

    if (NormalizedDamage >= ShatterThreshold) return EMaterialReactionType::Shatter;
    if (NormalizedDamage >= CrackThreshold) return EMaterialReactionType::Crack;
    if (NormalizedDamage >= 0.1f) return EMaterialReactionType::Deform;
    return EMaterialReactionType::None;
}


bool UImpactMaterialRule::CanApplyTo(const UMaterialDefinition* Material) const
{
    return Material != nullptr && bIsEnabled;
}

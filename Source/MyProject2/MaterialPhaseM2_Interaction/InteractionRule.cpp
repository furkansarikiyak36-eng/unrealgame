// InteractionRule.cpp
#include "InteractionRule.h"
#include "MaterialCore/MaterialDefinition.h"

float UInteractionRule::EvaluateDamage(const UMaterialDefinition* Material, const FMaterialInteractionContext& Context) const
{
    if (!Material || !bIsEnabled) return 0.0f;
    return FMath::Max(0.0f, Context.Magnitude - BaseThreshold) * DamageMultiplier;
}

EMaterialReactionType UInteractionRule::DetermineReaction(const UMaterialDefinition* Material, const FMaterialInteractionContext& Context) const
{
    return EMaterialReactionType::None;
}

bool UInteractionRule::CanApplyTo(const UMaterialDefinition* Material) const
{
    return Material && Material->MaterialClass == TargetClass;
}

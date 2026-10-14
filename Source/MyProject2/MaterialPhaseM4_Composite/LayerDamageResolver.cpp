// LayerDamageResolver.cpp
#include "LayerDamageResolver.h"
#include "CompositeStructureComponent.h"
#include "StructureLayerComponent.h"

EStructureLayerRole ULayerDamageResolver::ResolveDamageTarget(EMaterialReactionType Reaction)
{
    switch (Reaction)
    {
        case EMaterialReactionType::Dissolve: return EStructureLayerRole::Cladding;
        case EMaterialReactionType::Shatter:  return EStructureLayerRole::Cladding;
        case EMaterialReactionType::Burn:
        case EMaterialReactionType::Char:     return EStructureLayerRole::Skeleton;
        case EMaterialReactionType::Rot:      return EStructureLayerRole::Skeleton;
        case EMaterialReactionType::Rust:     return EStructureLayerRole::Connector;
        default: return EStructureLayerRole::Cladding;
    }
}

bool ULayerDamageResolver::ShouldDamagePropagateToSkeleton(EStructureLayerCondition CladdingCondition)
{
    return CladdingCondition == EStructureLayerCondition::Collapsed;
}

float ULayerDamageResolver::ComputeLoadOnFoundation(UCompositeStructureComponent* Structure)
{
    if (!Structure) return 0.0f;
    return Structure->GetTotalLoadBearing();
}

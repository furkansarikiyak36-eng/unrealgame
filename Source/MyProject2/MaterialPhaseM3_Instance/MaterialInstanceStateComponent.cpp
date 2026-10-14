// MaterialInstanceStateComponent.cpp
#include "MaterialInstanceStateComponent.h"
#include "MaterialCore/MaterialDefinition.h"

UMaterialInstanceStateComponent::UMaterialInstanceStateComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UMaterialInstanceStateComponent::InitializeState(UMaterialDefinition* Def)
{
    if (!Def) return;
    Definition = Def;
    State.MaterialID = Def->MaterialID;
    State.Reset();
}

void UMaterialInstanceStateComponent::ApplyDamage(float Amount, EMaterialReactionType Reaction)
{
    State.ApplyDamage(Amount);
    State.ActiveReactions.AddUnique(Reaction);
}

void UMaterialInstanceStateComponent::UpdateFromInteraction(const FMaterialInteractionContext& Context, float Damage)
{
    State.CurrentMoisture = FMath::Lerp(State.CurrentMoisture, Context.Moisture, 0.3f);
    State.CurrentTemperature = Context.AmbientTemperature;
    State.ApplyDamage(Damage);
}


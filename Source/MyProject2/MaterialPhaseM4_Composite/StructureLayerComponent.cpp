// StructureLayerComponent.cpp
#include "StructureLayerComponent.h"

UStructureLayerComponent::UStructureLayerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UStructureLayerComponent::InitializeLayer(EStructureLayerRole Role, FName MaterialID, float Thickness)
{
    Layer.Role = Role;
    Layer.MaterialID = MaterialID;
    Layer.Thickness = Thickness;
    Layer.Condition = EStructureLayerCondition::Pristine;
    Layer.CurrentDamage = 0.0f;
    Layer.LoadBearing = 1.0f;
    Layer.InstanceState.MaterialID = MaterialID;
    Layer.InstanceState.Reset();
    LastCondition = Layer.Condition;
}

void UStructureLayerComponent::ApplyDamageToLayer(float Amount, EMaterialReactionType Reaction)
{
    Layer.ApplyLayerDamage(Amount);
    Layer.InstanceState.ApplyDamage(Amount);
    Layer.InstanceState.ActiveReactions.AddUnique(Reaction);

    if (Layer.Condition != LastCondition)
    {
        LastCondition = Layer.Condition;
        if (OnLayerConditionChanged.IsBound())
            OnLayerConditionChanged.Broadcast(Layer.Role, Layer.Condition);
    }
}

// CompositeStructureComponent.cpp
#include "CompositeStructureComponent.h"
#include "StructureLayerComponent.h"

UCompositeStructureComponent::UCompositeStructureComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

UStructureLayerComponent* UCompositeStructureComponent::AddLayer(EStructureLayerRole Role, FName MaterialID, float Thickness)
{
    if (!GetOwner()) return nullptr;

    UStructureLayerComponent* NewLayer = NewObject<UStructureLayerComponent>(GetOwner());
    if (!NewLayer) return nullptr;
    GetOwner()->AddInstanceComponent(NewLayer);
    NewLayer->InitializeLayer(Role, MaterialID, Thickness);
    NewLayer->RegisterComponent();
    Layers.Add(NewLayer);
    return NewLayer;
}

UStructureLayerComponent* UCompositeStructureComponent::FindLayer(EStructureLayerRole Role) const
{
    for (UStructureLayerComponent* L : Layers)
        if (L && L->GetLayer().Role == Role) return L;
    return nullptr;
}

void UCompositeStructureComponent::ApplyDamageToLayerRole(EStructureLayerRole Role, float Amount, EMaterialReactionType Reaction)
{
    if (UStructureLayerComponent* Layer = FindLayer(Role))
        Layer->ApplyDamageToLayer(Amount, Reaction);
}

void UCompositeStructureComponent::ApplyDamageToAllLayers(float Amount, EMaterialReactionType Reaction)
{
    for (UStructureLayerComponent* L : Layers)
        if (L) L->ApplyDamageToLayer(Amount, Reaction);
}

float UCompositeStructureComponent::GetTotalLoadBearing() const
{
    float Total = 0.0f;
    for (UStructureLayerComponent* L : Layers)
        if (L) Total += L->GetLayer().LoadBearing;
    return Total;
}

bool UCompositeStructureComponent::IsStructureStable() const
{
    // Taşıyıcı katmanlar hâlâ sağlam mı?
    const UStructureLayerComponent* Foundation = FindLayer(EStructureLayerRole::Foundation);
    const UStructureLayerComponent* Skeleton = FindLayer(EStructureLayerRole::Skeleton);

    if (Foundation && !Foundation->GetLayer().IsStructurallySound()) return false;
    if (Skeleton && !Skeleton->GetLayer().IsStructurallySound()) return false;
    return true;
}

EStructureLayerCondition UCompositeStructureComponent::GetWorstCondition() const
{
    EStructureLayerCondition Worst = EStructureLayerCondition::Pristine;
    for (UStructureLayerComponent* L : Layers)
    {
        if (!L) continue;
        if ((uint8)L->GetCondition() > (uint8)Worst)
            Worst = L->GetCondition();
    }
    return Worst;
}

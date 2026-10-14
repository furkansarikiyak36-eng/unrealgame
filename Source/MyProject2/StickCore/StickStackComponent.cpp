#include "StickStackComponent.h"
#include "StickPhaseS1_Core/StickActor.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
UStickStackComponent::UStickStackComponent() { PrimaryComponentTick.bCanEverTick = false; }
int32 UStickStackComponent::AddToStack(AStickActor* Stick)
{
    if (!Stick || IsFull()) return StackedSticks.Num();
    ACharacter* Char = Cast<ACharacter>(GetOwner()); if (!Char) return StackedSticks.Num();
    if (!Stick->SetStructuralState_Implementation(EStructuralNodeState::Carried)) return StackedSticks.Num();
    Stick->AttachToComponent(Char->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachSocket);
    StackedSticks.Add(Stick); return StackedSticks.Num();
}
AStickActor* UStickStackComponent::TakeFromStack()
{
    if (StackedSticks.IsEmpty()) return nullptr;
    AStickActor* Stick = StackedSticks.Pop(); if (Stick) { Stick->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform); Stick->SetStructuralState_Implementation(EStructuralNodeState::Loose); } return Stick;
}
int32 UStickStackComponent::AddBundle(const FStickBundle& Bundle)
{
    // Bundle entries are inventory counts, not spawned actors. Return accepted count; call AddToStack separately for physical actors.
    return FMath::Clamp(Bundle.Count, 0, FMath::Max(0, MaxStackSize - StackedSticks.Num()));
}

#include "StoneCarryComponent.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "StoneCore/StoneDefinition.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

UStoneCarryComponent::UStoneCarryComponent() { PrimaryComponentTick.bCanEverTick = false; }

bool UStoneCarryComponent::PickUp(AStoneActor* Stone, EStoneCarryMode Mode)
{
    if (!Stone || CarriedStone || !GetOwner()) return false;
    UStoneDefinition* Def = Stone->GetDefinition();
    if (!Def) return false;
    if (Mode == EStoneCarryMode::SingleHand && Def->Mass > MaxSingleHandMass) return false;
    if (Mode == EStoneCarryMode::TwoHands && Def->Mass > MaxTwoHandMass) return false;
    if (Mode == EStoneCarryMode::Stacked || Mode == EStoneCarryMode::Dragging) return false;
    if (!Stone->SetStructuralState_Implementation(EStructuralNodeState::Carried)) return false;
    if (ACharacter* Char = Cast<ACharacter>(GetOwner()))
    {
        const FName Socket = Mode == EStoneCarryMode::TwoHands ? LeftHandSocket :
            (Mode == EStoneCarryMode::Shouldered ? ShoulderSocket : HandSocket);
        Stone->AttachToComponent(Char->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket);
    }
    CarriedStone = Stone; CurrentMode = Mode; return true;
}

bool UStoneCarryComponent::Drop()
{
    if (!CarriedStone) return false;
    CarriedStone->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    CarriedStone->SetStructuralState_Implementation(EStructuralNodeState::Loose);
    CarriedStone = nullptr; return true;
}

int32 UStoneCarryComponent::AddToStack(AStoneActor* SmallStone)
{
    if (!SmallStone || StackedStones.Num() >= MaxStackSize || !GetOwner()) return StackedStones.Num();
    if (!SmallStone->SetStructuralState_Implementation(EStructuralNodeState::Carried)) return StackedStones.Num();
    if (ACharacter* Char = Cast<ACharacter>(GetOwner()))
        SmallStone->AttachToComponent(Char->GetMesh(), FAttachmentTransformRules::KeepWorldTransform, HandSocket);
    StackedStones.Add(SmallStone); return StackedStones.Num();
}

AStoneActor* UStoneCarryComponent::TakeFromStack()
{
    if (StackedStones.Num() == 0) return nullptr;
    AStoneActor* Stone = StackedStones.Pop().Get();
    if (Stone)
    {
        Stone->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        Stone->SetStructuralState_Implementation(EStructuralNodeState::Loose);
    }
    return Stone;
}

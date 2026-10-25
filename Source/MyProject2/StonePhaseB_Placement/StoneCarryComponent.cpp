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
        USkeletalMeshComponent* CharacterMesh = Char->GetMesh();
        if (CharacterMesh && CharacterMesh->DoesSocketExist(Socket))
        {
            Stone->AttachToComponent(
                CharacterMesh,
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                Socket);
        }
        else if (USceneComponent* OwnerRoot = Char->GetRootComponent())
        {
            // The current first-person prototype may not assign a full skeletal mesh/socket rig.
            // Keep the physical stone visible just in front of the player instead of snapping it
            // into the capsule origin or a nonexistent hand socket.
            const FVector CarryLocation = Char->GetActorLocation()
                + Char->GetActorForwardVector() * 48.0f
                + FVector(0.0f, 0.0f, 38.0f);
            Stone->SetActorLocation(CarryLocation, false, nullptr, ETeleportType::TeleportPhysics);
            Stone->AttachToComponent(
                OwnerRoot,
                FAttachmentTransformRules::KeepWorldTransform);
        }
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

bool UStoneCarryComponent::ReleaseForPlacement(AStoneActor* Stone)
{
    if (!IsValid(Stone) || CarriedStone != Stone)
    {
        return false;
    }

    // Placement has already transitioned this same physical stone to Installed.
    // Do not detach or mutate its state here; only clear the carry owner's reference.
    CarriedStone = nullptr;
    CurrentMode = EStoneCarryMode::SingleHand;
    return true;
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

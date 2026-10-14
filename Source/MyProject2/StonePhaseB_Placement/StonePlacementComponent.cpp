#include "StonePlacementComponent.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

UStonePlacementComponent::UStonePlacementComponent() { PrimaryComponentTick.bCanEverTick = true; }
void UStonePlacementComponent::EnterAimMode() { bAiming = true; }
void UStonePlacementComponent::ExitAimMode() { bAiming = false; }
void UStonePlacementComponent::CycleFace() { /* TODO: cycle through valid faces/sockets from UStoneDefinition. */ }
void UStonePlacementComponent::RotateStone(float Delta) { TargetTransform.SetRotation(FQuat(FVector::UpVector, FMath::DegreesToRadians(Delta * 15.0f)) * TargetTransform.GetRotation()); }
bool UStonePlacementComponent::ConfirmPlacement()
{
    if (!PreviewStone) return false;
    PreviewStone->SetActorTransform(TargetTransform);
    if (!PreviewStone->SetStructuralState_Implementation(EStructuralNodeState::Installed)) return false;
    PlacementHistory.Add(PreviewStone); PreviewStone = nullptr; return true;
}
bool UStonePlacementComponent::UndoLastPlacement()
{
    if (PlacementHistory.Num() == 0) return false;
    AStoneActor* Last = PlacementHistory.Pop().Get();
    return Last && Last->SetStructuralState_Implementation(EStructuralNodeState::Loose);
}
void UStonePlacementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!bAiming || !GetOwner() || !GetWorld()) return;
    const FVector Start = GetOwner()->GetActorLocation();
    const FVector End = Start + GetOwner()->GetActorForwardVector() * ReachDistance;
    FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(StonePlacementTrace), false, GetOwner());
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        TargetTransform.SetLocation(Hit.ImpactPoint);
        TargetTransform.SetRotation(Hit.ImpactNormal.Rotation().Quaternion());
    }
}

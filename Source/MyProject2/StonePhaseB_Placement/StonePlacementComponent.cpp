#include "StonePlacementComponent.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "StoneCore/StoneDefinition.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "Math/RotationMatrix.h"

namespace
{
    void CollectValidPlacementOptions(
        const UStoneDefinition* Definition,
        TArray<int32>& OutFaceIndices,
        TArray<int32>& OutSocketIndices)
    {
        OutFaceIndices.Reset();
        OutSocketIndices.Reset();
        if (!Definition)
        {
            return;
        }

        for (int32 Index = 0; Index < Definition->Faces.Num(); ++Index)
        {
            const FStoneFace& Face = Definition->Faces[Index];
            if (Face.FaceName.IsNone() || Face.LocalNormal.ContainsNaN() || Face.LocalNormal.IsNearlyZero())
            {
                continue;
            }
            if (Face.OccupiedBy.IsValid())
            {
                continue;
            }
            OutFaceIndices.Add(Index);
        }

        for (int32 Index = 0; Index < Definition->Sockets.Num(); ++Index)
        {
            const FStoneSocket& Socket = Definition->Sockets[Index];
            if (Socket.SocketName.IsNone() || Socket.LocalPosition.ContainsNaN() ||
                Socket.LocalRotation.ContainsNaN() || !FMath::IsFinite(Socket.SocketRadius) ||
                Socket.SocketRadius <= 0.0f)
            {
                continue;
            }
            if (Socket.Occupant.IsValid())
            {
                continue;
            }
            OutSocketIndices.Add(Index);
        }
    }
}

UStonePlacementComponent::UStonePlacementComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

bool UStonePlacementComponent::BeginPlacement(AStoneActor* Stone)
{
    if (PreviewStone || !IsValid(Stone) || !Stone->GetDefinition() || !GetOwner())
    {
        return false;
    }

    const EStructuralNodeState State = Stone->GetStructuralState_Implementation();
    if (State != EStructuralNodeState::Loose && State != EStructuralNodeState::Carried)
    {
        return false;
    }

    OriginalActorTransform = Stone->GetActorTransform();
    OriginalStructuralState = State;
    OriginalAttachComponent.Reset();
    OriginalAttachSocket = NAME_None;

    if (USceneComponent* Root = Stone->GetRootComponent())
    {
        OriginalAttachComponent = Root->GetAttachParent();
        OriginalAttachSocket = Root->GetAttachSocketName();
    }

    if (State == EStructuralNodeState::Loose &&
        !Stone->SetStructuralState_Implementation(EStructuralNodeState::Carried))
    {
        return false;
    }

    // If the stone was held, detach it from the hand/shoulder while preserving its state.
    Stone->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

    PreviewStone = Stone;
    bAiming = true;
    bHasPlacementTarget = false;
    PlacementOptionIndex = INDEX_NONE;
    ManualYawDegrees = 0.0f;
    TargetTransform = OriginalActorTransform;
    TargetSurfacePoint = OriginalActorTransform.GetLocation();
    TargetSurfaceNormal = FVector::UpVector;
    return true;
}

bool UStonePlacementComponent::CancelPlacement()
{
    AStoneActor* Stone = PreviewStone;
    if (!IsValid(Stone))
    {
        PreviewStone = nullptr;
        bAiming = false;
        bHasPlacementTarget = false;
        PlacementOptionIndex = INDEX_NONE;
        return false;
    }

    Stone->SetActorTransform(OriginalActorTransform, false, nullptr, ETeleportType::TeleportPhysics);

    if (USceneComponent* AttachParent = OriginalAttachComponent.Get())
    {
        if (Stone->GetStructuralState_Implementation() != OriginalStructuralState &&
            !Stone->SetStructuralState_Implementation(OriginalStructuralState))
        {
            return false;
        }
        Stone->AttachToComponent(
            AttachParent,
            FAttachmentTransformRules::SnapToTargetNotIncludingScale,
            OriginalAttachSocket);
    }
    else if (Stone->GetStructuralState_Implementation() != OriginalStructuralState)
    {
        if (!Stone->SetStructuralState_Implementation(OriginalStructuralState))
        {
            return false;
        }
    }

    PreviewStone = nullptr;
    bAiming = false;
    bHasPlacementTarget = false;
    PlacementOptionIndex = INDEX_NONE;
    ManualYawDegrees = 0.0f;
    OriginalAttachComponent.Reset();
    OriginalAttachSocket = NAME_None;
    return true;
}

void UStonePlacementComponent::EnterAimMode()
{
    bAiming = true;
}

void UStonePlacementComponent::ExitAimMode()
{
    if (PreviewStone)
    {
        CancelPlacement();
        return;
    }

    bAiming = false;
    bHasPlacementTarget = false;
    PlacementOptionIndex = INDEX_NONE;
    ManualYawDegrees = 0.0f;
}

bool UStonePlacementComponent::UpdatePlacementTarget(FVector SurfacePoint, FVector SurfaceNormal)
{
    if (!IsValid(PreviewStone) || !bAiming || SurfacePoint.ContainsNaN() ||
        SurfaceNormal.ContainsNaN() || SurfaceNormal.IsNearlyZero())
    {
        bHasPlacementTarget = false;
        return false;
    }

    TargetSurfacePoint = SurfacePoint;
    TargetSurfaceNormal = SurfaceNormal.GetSafeNormal();
    TargetTransform.SetLocation(TargetSurfacePoint);
    TargetTransform.SetRotation(FRotationMatrix::MakeFromZ(TargetSurfaceNormal).ToQuat());
    bHasPlacementTarget = true;
    ApplySelectedPlacementOption();
    return true;
}

void UStonePlacementComponent::CycleFace()
{
    if (!PreviewStone)
    {
        return;
    }

    TArray<int32> FaceIndices;
    TArray<int32> SocketIndices;
    CollectValidPlacementOptions(PreviewStone->GetDefinition(), FaceIndices, SocketIndices);

    const int32 OptionCount = FaceIndices.Num() + SocketIndices.Num();
    if (OptionCount <= 0)
    {
        PlacementOptionIndex = INDEX_NONE;
        return;
    }

    PlacementOptionIndex = PlacementOptionIndex == INDEX_NONE
        ? 0
        : (PlacementOptionIndex + 1) % OptionCount;

    ApplySelectedPlacementOption();
}

void UStonePlacementComponent::RotateStone(float Delta)
{
    if (!FMath::IsFinite(Delta) || FMath::IsNearlyZero(Delta))
    {
        return;
    }

    ManualYawDegrees = FMath::Fmod(ManualYawDegrees + Delta * 15.0f, 360.0f);
    ApplySelectedPlacementOption();
}

void UStonePlacementComponent::ApplySelectedPlacementOption()
{
    if (!PreviewStone || !bHasPlacementTarget)
    {
        return;
    }

    UStoneDefinition* Definition = PreviewStone->GetDefinition();
    TArray<int32> FaceIndices;
    TArray<int32> SocketIndices;
    CollectValidPlacementOptions(Definition, FaceIndices, SocketIndices);

    const int32 OptionCount = FaceIndices.Num() + SocketIndices.Num();
    if (OptionCount > 0 && (PlacementOptionIndex < 0 || PlacementOptionIndex >= OptionCount))
    {
        PlacementOptionIndex = 0;
    }
    if (OptionCount <= 0)
    {
        PlacementOptionIndex = INDEX_NONE;
    }

    FQuat BaseRotation = FRotationMatrix::MakeFromZ(TargetSurfaceNormal.GetSafeNormal()).ToQuat();
    FVector PlacementLocation = TargetSurfacePoint;
    bool bSelectedSocket = false;
    const FStoneSocket* SelectedSocket = nullptr;

    if (PlacementOptionIndex != INDEX_NONE && PlacementOptionIndex < FaceIndices.Num())
    {
        const FStoneFace& Face = Definition->Faces[FaceIndices[PlacementOptionIndex]];
        const FVector WorldFaceNormal = BaseRotation.RotateVector(Face.LocalNormal.GetSafeNormal());
        const FQuat AlignRotation = FQuat::FindBetweenNormals(
            WorldFaceNormal.GetSafeNormal(), -TargetSurfaceNormal.GetSafeNormal());
        BaseRotation = AlignRotation * BaseRotation;
    }
    else if (PlacementOptionIndex != INDEX_NONE && PlacementOptionIndex >= FaceIndices.Num())
    {
        const int32 SocketOptionIndex = PlacementOptionIndex - FaceIndices.Num();
        if (SocketOptionIndex >= 0 && SocketOptionIndex < SocketIndices.Num())
        {
            SelectedSocket = &Definition->Sockets[SocketIndices[SocketOptionIndex]];
            const FVector LocalSocketNormal = SelectedSocket->LocalRotation.RotateVector(FVector::UpVector);
            const FVector WorldSocketNormal = BaseRotation.RotateVector(LocalSocketNormal.GetSafeNormal());
            const FQuat AlignRotation = FQuat::FindBetweenNormals(
                WorldSocketNormal.GetSafeNormal(), -TargetSurfaceNormal.GetSafeNormal());
            BaseRotation = AlignRotation * BaseRotation;
            bSelectedSocket = true;
        }
    }

    const FQuat YawRotation(
        TargetSurfaceNormal.GetSafeNormal(), FMath::DegreesToRadians(ManualYawDegrees));
    const FQuat FinalRotation = YawRotation * BaseRotation;
    if (bSelectedSocket && SelectedSocket)
    {
        PlacementLocation = TargetSurfacePoint - FinalRotation.RotateVector(SelectedSocket->LocalPosition);
    }

    TargetTransform.SetLocation(PlacementLocation);
    TargetTransform.SetRotation(FinalRotation);
    PreviewStone->SetActorTransform(TargetTransform, false, nullptr, ETeleportType::TeleportPhysics);
}

bool UStonePlacementComponent::ConfirmPlacement()
{
    if (!IsValid(PreviewStone) || !bAiming || !bHasPlacementTarget)
    {
        return false;
    }

    AStoneActor* StoneToPlace = PreviewStone;
    StoneToPlace->SetActorTransform(TargetTransform, false, nullptr, ETeleportType::TeleportPhysics);
    if (!StoneToPlace->SetStructuralState_Implementation(EStructuralNodeState::Installed))
    {
        CancelPlacement();
        return false;
    }

    PlacementHistory.Add(StoneToPlace);
    PreviewStone = nullptr;
    bAiming = false;
    bHasPlacementTarget = false;
    PlacementOptionIndex = INDEX_NONE;
    ManualYawDegrees = 0.0f;
    OriginalAttachComponent.Reset();
    OriginalAttachSocket = NAME_None;
    return true;
}

bool UStonePlacementComponent::UndoLastPlacement()
{
    if (PlacementHistory.Num() == 0)
    {
        return false;
    }

    AStoneActor* Last = PlacementHistory.Pop().Get();
    return Last && Last->SetStructuralState_Implementation(EStructuralNodeState::Loose);
}

void UStonePlacementComponent::TickComponent(
    float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!bAiming || !PreviewStone || !GetOwner() || !GetWorld())
    {
        return;
    }

    const FVector Start = GetOwner()->GetActorLocation();
    const FVector End = Start + GetOwner()->GetActorForwardVector() * ReachDistance;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(StonePlacementTrace), false, GetOwner());

    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        UpdatePlacementTarget(Hit.ImpactPoint, Hit.ImpactNormal);
    }
    else
    {
        bHasPlacementTarget = false;
        TargetTransform = OriginalActorTransform;
        PreviewStone->SetActorTransform(OriginalActorTransform, false, nullptr, ETeleportType::TeleportPhysics);
    }
}

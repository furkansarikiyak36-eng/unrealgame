#include "StonePlacementComponent.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "StoneCore/StoneDefinition.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

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

void UStonePlacementComponent::EnterAimMode()
{
    bAiming = true;
    PlacementOptionIndex = INDEX_NONE;
    ManualYawDegrees = 0.0f;
}

void UStonePlacementComponent::ExitAimMode()
{
    bAiming = false;
    PlacementOptionIndex = INDEX_NONE;
    ManualYawDegrees = 0.0f;
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
    if (!PreviewStone)
    {
        return;
    }

    UStoneDefinition* Definition = PreviewStone->GetDefinition();
    TArray<int32> FaceIndices;
    TArray<int32> SocketIndices;
    CollectValidPlacementOptions(Definition, FaceIndices, SocketIndices);

    const int32 OptionCount = FaceIndices.Num() + SocketIndices.Num();
    if (OptionCount <= 0)
    {
        PlacementOptionIndex = INDEX_NONE;
        TargetTransform.SetLocation(TargetSurfacePoint);
        TargetTransform.SetRotation(
            FQuat(TargetSurfaceNormal.GetSafeNormal(), FMath::DegreesToRadians(ManualYawDegrees)) *
            TargetSurfaceNormal.Rotation().Quaternion());
        return;
    }

    if (PlacementOptionIndex < 0 || PlacementOptionIndex >= OptionCount)
    {
        PlacementOptionIndex = 0;
    }

    FQuat BaseRotation = TargetSurfaceNormal.Rotation().Quaternion();
    FVector PlacementLocation = TargetSurfacePoint;

    if (PlacementOptionIndex < FaceIndices.Num())
    {
        const FStoneFace& Face = Definition->Faces[FaceIndices[PlacementOptionIndex]];
        const FVector WorldFaceNormal = BaseRotation.RotateVector(Face.LocalNormal.GetSafeNormal());
        const FQuat AlignRotation = FQuat::FindBetweenNormals(
            WorldFaceNormal.GetSafeNormal(), -TargetSurfaceNormal.GetSafeNormal());
        BaseRotation = AlignRotation * BaseRotation;
    }
    else
    {
        const int32 SocketOptionIndex = PlacementOptionIndex - FaceIndices.Num();
        const FStoneSocket& Socket = Definition->Sockets[SocketIndices[SocketOptionIndex]];
        const FVector LocalSocketNormal = Socket.LocalRotation.RotateVector(FVector::UpVector);
        const FVector WorldSocketNormal = BaseRotation.RotateVector(LocalSocketNormal.GetSafeNormal());
        const FQuat AlignRotation = FQuat::FindBetweenNormals(
            WorldSocketNormal.GetSafeNormal(), -TargetSurfaceNormal.GetSafeNormal());
        BaseRotation = AlignRotation * BaseRotation;

        const FQuat YawRotation(
            TargetSurfaceNormal.GetSafeNormal(), FMath::DegreesToRadians(ManualYawDegrees));
        const FQuat FinalSocketRotation = YawRotation * BaseRotation;
        PlacementLocation = TargetSurfacePoint - FinalSocketRotation.RotateVector(Socket.LocalPosition);
        TargetTransform.SetLocation(PlacementLocation);
        TargetTransform.SetRotation(FinalSocketRotation);
        return;
    }

    const FQuat YawRotation(
        TargetSurfaceNormal.GetSafeNormal(), FMath::DegreesToRadians(ManualYawDegrees));
    TargetTransform.SetLocation(PlacementLocation);
    TargetTransform.SetRotation(YawRotation * BaseRotation);
}

bool UStonePlacementComponent::ConfirmPlacement()
{
    if (!PreviewStone)
    {
        return false;
    }

    PreviewStone->SetActorTransform(TargetTransform);
    if (!PreviewStone->SetStructuralState_Implementation(EStructuralNodeState::Installed))
    {
        return false;
    }

    PlacementHistory.Add(PreviewStone);
    PreviewStone = nullptr;
    PlacementOptionIndex = INDEX_NONE;
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
    if (!bAiming || !GetOwner() || !GetWorld())
    {
        return;
    }

    const FVector Start = GetOwner()->GetActorLocation();
    const FVector End = Start + GetOwner()->GetActorForwardVector() * ReachDistance;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(StonePlacementTrace), false, GetOwner());

    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        TargetSurfacePoint = Hit.ImpactPoint;
        TargetSurfaceNormal = Hit.ImpactNormal.GetSafeNormal();
        if (TargetSurfaceNormal.IsNearlyZero())
        {
            TargetSurfaceNormal = FVector::UpVector;
        }

        TargetTransform.SetLocation(TargetSurfacePoint);
        TargetTransform.SetRotation(TargetSurfaceNormal.Rotation().Quaternion());
        ApplySelectedPlacementOption();
    }
}

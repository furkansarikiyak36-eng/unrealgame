#include "StoneAlignmentComponent.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "StoneCore/StoneDefinition.h"
UStoneAlignmentComponent::UStoneAlignmentComponent() { PrimaryComponentTick.bCanEverTick = false; }
FTransform UStoneAlignmentComponent::ComputeAlignedTransform(AStoneActor* Moving, const FVector& DesiredLocation, const FVector& SurfaceNormal) const
{
    FTransform Result = FTransform::Identity;
    if (!Moving || !Moving->GetDefinition()) return Result;
    const UStoneDefinition* Def = Moving->GetDefinition();
    const FVector Normal = (SurfaceNormal.IsNearlyZero() ? FVector(FVector::UpVector) : SurfaceNormal.GetSafeNormal());
    const FVector FinalLocation = DesiredLocation + Normal * (Def->Dimensions.Z * 0.5f + VerticalOffset);
    Result.SetLocation(FinalLocation);
    Result.SetRotation(Normal.Rotation().Quaternion());
    return Result;
}
bool UStoneAlignmentComponent::AlignToNeighbors(AStoneActor* Stone)
{
    // TODO: use transformed FStoneSocket data and nearest compatible neighbour sockets.
    return Stone != nullptr;
}

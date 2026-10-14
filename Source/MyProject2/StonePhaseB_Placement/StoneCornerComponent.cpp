#include "StoneCornerComponent.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "Engine/World.h"
UStoneCornerComponent::UStoneCornerComponent() { PrimaryComponentTick.bCanEverTick = false; }
bool UStoneCornerComponent::BuildCorner(const FVector& CornerLocation, const FVector& DirA, const FVector& DirB, int32 Layers)
{
    if (Layers <= 0 || !StoneClass || !GetWorld()) return false;
    const FVector Bisector = (DirA.GetSafeNormal() + DirB.GetSafeNormal()).GetSafeNormal();
    if (Bisector.IsNearlyZero()) return false;
    bool bSpawnedAny = false;
    for (int32 L = 0; L < Layers; ++L)
    {
        const FVector Loc = CornerLocation + FVector(0, 0, L * 20.0f);
        if (AStoneActor* Stone = GetWorld()->SpawnActor<AStoneActor>(StoneClass, Loc, Bisector.Rotation()))
        { Stone->SetStructuralState_Implementation(EStructuralNodeState::Installed); bSpawnedAny = true; }
    }
    return bSpawnedAny;
}

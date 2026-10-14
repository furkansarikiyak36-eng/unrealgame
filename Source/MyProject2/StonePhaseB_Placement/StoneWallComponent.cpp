#include "StoneWallComponent.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "Engine/World.h"
UStoneWallComponent::UStoneWallComponent() { PrimaryComponentTick.bCanEverTick = false; }
bool UStoneWallComponent::BuildWall(const FVector& Start, const FVector& End, float Height, int32 Layers)
{
    if (Layers <= 0 || !StoneClass || StoneSpacing <= 0.0f || LayerHeight <= 0.0f || Height <= 0.0f || !GetWorld()) return false;
    const FVector Dir = (End - Start).GetSafeNormal();
    const float Length = FVector::Dist(Start, End);
    const int32 StonesPerLayer = FMath::Max(1, FMath::CeilToInt(Length / StoneSpacing));
    const int32 LayersToBuild = FMath::Min(Layers, FMath::CeilToInt(Height / LayerHeight));
    bool bSpawnedAny = false;
    for (int32 L = 0; L < LayersToBuild; ++L)
        for (int32 S = 0; S < StonesPerLayer; ++S)
        {
            const FVector Loc = Start + Dir * (S * StoneSpacing) + FVector(0, 0, L * LayerHeight);
            if (AStoneActor* Stone = GetWorld()->SpawnActor<AStoneActor>(StoneClass, Loc, Dir.Rotation()))
            { Stone->SetStructuralState_Implementation(EStructuralNodeState::Installed); WallStones.Add(Stone); bSpawnedAny = true; }
        }
    return bSpawnedAny;
}
bool UStoneWallComponent::IncreaseThickness(AStoneActor* WallStone)
{
    if (!WallStone || !StoneClass || !GetWorld()) return false;
    const FVector Normal = WallStone->GetActorRightVector();
    const FVector NewLoc = WallStone->GetActorLocation() + Normal * StoneSpacing;
    if (AStoneActor* NewStone = GetWorld()->SpawnActor<AStoneActor>(StoneClass, NewLoc, WallStone->GetActorRotation()))
    { NewStone->SetStructuralState_Implementation(EStructuralNodeState::Installed); WallStones.Add(NewStone); return true; }
    return false;
}

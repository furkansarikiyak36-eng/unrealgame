#include "StonePathComponent.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "Engine/World.h"
UStonePathComponent::UStonePathComponent() { PrimaryComponentTick.bCanEverTick = false; }
bool UStonePathComponent::BuildPath(const TArray<FVector>& Waypoints, float Width)
{
    if (Waypoints.Num() < 2 || !StoneClass || StoneSpacing <= 0.0f || Width <= 0.0f || !GetWorld()) return false;
    bool bSpawnedAny = false;
    for (int32 i = 0; i < Waypoints.Num() - 1; ++i)
    {
        const FVector Start = Waypoints[i], End = Waypoints[i + 1];
        const FVector Delta = End - Start;
        const float Len = Delta.Size();
        if (Len <= KINDA_SMALL_NUMBER) continue;
        const FVector Dir = Delta / Len;
        const int32 Count = FMath::CeilToInt(Len / StoneSpacing);
        const FVector Perp = FVector::CrossProduct(Dir, FVector::UpVector).GetSafeNormal();
        const int32 WidthCount = FMath::Max(1, FMath::CeilToInt(Width / StoneSpacing));
        for (int32 j = 0; j < Count; ++j)
            for (int32 w = 0; w < WidthCount; ++w)
            {
                const float Offset = (w - (WidthCount - 1) * 0.5f) * StoneSpacing;
                const FVector Loc = Start + Dir * (j * StoneSpacing) + Perp * Offset;
                if (AStoneActor* Stone = GetWorld()->SpawnActor<AStoneActor>(StoneClass, Loc, FRotator::ZeroRotator))
                { Stone->SetStructuralState_Implementation(EStructuralNodeState::Installed); bSpawnedAny = true; }
            }
    }
    return bSpawnedAny;
}

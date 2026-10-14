#include "StoneFoundationComponent.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "Engine/World.h"
UStoneFoundationComponent::UStoneFoundationComponent() { PrimaryComponentTick.bCanEverTick = false; }
bool UStoneFoundationComponent::BuildFoundation(const FVector& Center, float Width, float Depth)
{
    if (!StoneClass || !GetWorld() || Width <= 0.0f || Depth <= 0.0f || StoneSpacing <= 0.0f) return false;
    const int32 XCount = FMath::Max(1, FMath::CeilToInt(Width / StoneSpacing));
    const int32 YCount = FMath::Max(1, FMath::CeilToInt(Depth / StoneSpacing));
    bool bSpawnedAny = false;
    for (int32 X = 0; X < XCount; ++X)
        for (int32 Y = 0; Y < YCount; ++Y)
        {
            const FVector Loc = Center + FVector((X - (XCount - 1) * 0.5f) * StoneSpacing,
                (Y - (YCount - 1) * 0.5f) * StoneSpacing, 0.0f);
            FHitResult Hit;
            const FVector Start = Loc + FVector(0, 0, 500.0f), End = Loc - FVector(0, 0, 500.0f);
            FCollisionQueryParams Params(SCENE_QUERY_STAT(StoneFoundationTrace), false, GetOwner());
            FVector FinalLoc = Loc;
            if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params)) FinalLoc.Z = Hit.ImpactPoint.Z;
            if (AStoneActor* Stone = GetWorld()->SpawnActor<AStoneActor>(StoneClass, FinalLoc, FRotator::ZeroRotator))
            { Stone->SetStructuralState_Implementation(EStructuralNodeState::Installed); bSpawnedAny = true; }
        }
    return bSpawnedAny;
}

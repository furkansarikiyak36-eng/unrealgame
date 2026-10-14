#include "StonePillarComponent.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "StoneCore/StoneConnectionGraph.h"
#include "Engine/World.h"
UStonePillarComponent::UStonePillarComponent() { PrimaryComponentTick.bCanEverTick = false; }
AStoneActor* UStonePillarComponent::BuildPillar(const FVector& Base, float Height, int32 StoneCount)
{
    if (StoneCount <= 0 || !StoneClass || StoneHeight <= 0.0f || !GetWorld()) return nullptr;
    const int32 Count = Height > 0.0f ? FMath::Min(StoneCount, FMath::CeilToInt(Height / StoneHeight)) : StoneCount;
    AStoneActor* FirstStone = nullptr; AStoneActor* Previous = nullptr;
    for (int32 i = 0; i < Count; ++i)
    {
        AStoneActor* Stone = GetWorld()->SpawnActor<AStoneActor>(StoneClass, Base + FVector(0, 0, i * StoneHeight), FRotator::ZeroRotator);
        if (!Stone) continue;
        Stone->SetStructuralState_Implementation(EStructuralNodeState::Installed);
        if (Previous)
            if (UStoneConnectionGraph* G = Previous->FindComponentByClass<UStoneConnectionGraph>())
                G->ConnectStacked(Stone, TEXT("Top"), TEXT("Bottom"));
        if (!FirstStone) FirstStone = Stone;
        Previous = Stone;
    }
    return FirstStone;
}
bool UStonePillarComponent::IsStandalonePillar(AStoneActor* Pillar) const
{
    if (!Pillar) return false;
    if (UStoneConnectionGraph* G = Pillar->FindComponentByClass<UStoneConnectionGraph>())
        return G->GetConnectedNodes().Num() <= 1;
    return true;
}

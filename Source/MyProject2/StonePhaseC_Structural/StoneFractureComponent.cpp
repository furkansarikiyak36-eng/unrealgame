#include "StoneFractureComponent.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "Engine/World.h"
UStoneFractureComponent::UStoneFractureComponent() { PrimaryComponentTick.bCanEverTick = false; }
bool UStoneFractureComponent::Fracture(AStoneActor* Stone, const FVector& ImpactPoint)
{
    if (!Stone || !RubbleClass || !GetWorld()) return false;
    const int32 Low = FMath::Max(1, FMath::Min(MinPieces, MaxPieces));
    const int32 High = FMath::Max(Low, FMath::Max(MinPieces, MaxPieces));
    const int32 Pieces = FMath::RandRange(Low, High);
    const FVector Origin = ImpactPoint.IsNearlyZero() ? Stone->GetActorLocation() : ImpactPoint;
    for (int32 i = 0; i < Pieces; ++i)
    {
        const FVector Offset(FMath::RandRange(-40.f, 40.f), FMath::RandRange(-40.f, 40.f), FMath::RandRange(0.f, 30.f));
        if (AStoneActor* Rubble = GetWorld()->SpawnActor<AStoneActor>(RubbleClass, Origin + Offset, FRotator::ZeroRotator))
            Rubble->SetStructuralState_Implementation(EStructuralNodeState::Loose);
    }
    Stone->Destroy();
    return true;
}
bool UStoneFractureComponent::IsCracked(AStoneActor* Stone) const
{
    if (!Stone || Stone->GetMaxHealth() <= 0.0f) return false;
    return Stone->GetHealth() / Stone->GetMaxHealth() <= FMath::Clamp(CrackThreshold, 0.0f, 1.0f);
}

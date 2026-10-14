#include "StoneMiningComponent.h"
#include "RockBedActor.h"

UStoneMiningComponent::UStoneMiningComponent() { PrimaryComponentTick.bCanEverTick = false; }

bool UStoneMiningComponent::MineRock(ARockBedActor* RockBed, float Strength)
{
    if (!RockBed || !GetOwner() || Strength <= 0.0f) return false;
    const float Dist = FVector::Dist(GetOwner()->GetActorLocation(), RockBed->GetActorLocation());
    if (Dist > MiningRange) return false;
    return RockBed->MineChunk(Strength * MiningPower, GetOwner()) > 0;
}

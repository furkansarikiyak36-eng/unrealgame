#include "HeatRadiusComponent.h"
UHeatRadiusComponent::UHeatRadiusComponent() { PrimaryComponentTick.bCanEverTick = false; }
float UHeatRadiusComponent::GetHeatAtLocation(const FVector& Location) const
{
    if (!GetOwner() || HeatRadius <= 0.0f) return AmbientTemperature;
    const float Dist = FVector::Dist(GetOwner()->GetActorLocation(), Location);
    if (Dist >= HeatRadius) return AmbientTemperature;
    const float Alpha = 1.0f - Dist / HeatRadius;
    return FMath::Lerp(AmbientTemperature, MaxTemperature, Alpha);
}
bool UHeatRadiusComponent::IsInWarmZone(const FVector& Location) const
{ return GetOwner() && HeatRadius > 0.0f && FVector::Dist(GetOwner()->GetActorLocation(), Location) <= HeatRadius; }

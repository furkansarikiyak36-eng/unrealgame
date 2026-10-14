// MoistureComponent.cpp
#include "MoistureComponent.h"

UMoistureComponent::UMoistureComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UMoistureComponent::UpdateMoisture(float AmbientMoisture, float AbsorptionRate, float DeltaDays)
{
    // N(t+1) = N(t) + (N_ambient - N(t)) * AbsorptionRate * dt
    const float Delta = (AmbientMoisture - CurrentMoisture) * AbsorptionRate * DeltaDays;
    CurrentMoisture = FMath::Clamp(CurrentMoisture + Delta, 0.0f, 1.0f);
}

void UMoistureComponent::SoakInWater(float Amount)
{
    CurrentMoisture = FMath::Clamp(CurrentMoisture + Amount * SoakingRate, 0.0f, 1.0f);
}

void UMoistureComponent::Dry(float Rate, float DeltaDays)
{
    CurrentMoisture = FMath::Max(0.0f, CurrentMoisture - Rate * DryingRate * DeltaDays);
}

// TemperatureComponent.cpp
#include "TemperatureComponent.h"

UTemperatureComponent::UTemperatureComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UTemperatureComponent::UpdateTemperature(float Ambient, float Conductivity, float DeltaHours)
{
    AmbientTemperature = Ambient;
    // T(t+1) = T(t) + (T_ambient - T(t)) * Conductivity * dt / HeatCapacity
    const float Delta = (Ambient - CurrentTemperature) * Conductivity * DeltaHours / FMath::Max(0.1f, HeatCapacity);
    CurrentTemperature += Delta;
}

void UTemperatureComponent::ApplyHeat(float Amount)
{
    CurrentTemperature += Amount / FMath::Max(0.1f, HeatCapacity);
}

void UTemperatureComponent::ApplyCooling(float Amount)
{
    CurrentTemperature -= Amount / FMath::Max(0.1f, HeatCapacity);
}

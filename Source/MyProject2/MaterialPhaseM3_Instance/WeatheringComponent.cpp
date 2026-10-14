// WeatheringComponent.cpp
#include "WeatheringComponent.h"

UWeatheringComponent::UWeatheringComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UWeatheringComponent::UpdateWeathering(EWeatheringType Type, float Intensity, float DeltaDays)
{
    float Rate = 0.0f;
    switch (Type)
    {
        case EWeatheringType::Rain:  Rate = RainRate;  break;
        case EWeatheringType::Wind:  Rate = WindRate;  break;
        case EWeatheringType::Frost: Rate = FrostRate; break;
        default: return;
    }

    WeatheringLevel = FMath::Clamp(WeatheringLevel + Rate * Intensity * DeltaDays, 0.0f, 1.0f);

    if (Intensity > 0.5f) DominantType = Type;
}

void UWeatheringComponent::ApplyWeatheringDamage(float Amount)
{
    WeatheringLevel = FMath::Clamp(WeatheringLevel + Amount, 0.0f, 1.0f);
}

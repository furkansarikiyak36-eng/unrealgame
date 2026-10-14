// WeatheringComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MaterialCore/MaterialTypes.h"
#include "WeatheringComponent.generated.h"

UCLASS(ClassGroup=(Material), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UWeatheringComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWeatheringComponent();

    UFUNCTION(BlueprintCallable, Category = "Weathering")
    void UpdateWeathering(EWeatheringType Type, float Intensity, float DeltaDays);

    UFUNCTION(BlueprintCallable, Category = "Weathering")
    void ApplyWeatheringDamage(float Amount);

    UFUNCTION(BlueprintPure, Category = "Weathering")
    float GetWeatheringLevel() const { return WeatheringLevel; }

    UFUNCTION(BlueprintPure, Category = "Weathering")
    EWeatheringType GetDominantType() const { return DominantType; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite) float WeatheringLevel = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EWeatheringType DominantType = EWeatheringType::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float RainRate = 0.01f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float WindRate = 0.005f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float FrostRate = 0.02f;
};

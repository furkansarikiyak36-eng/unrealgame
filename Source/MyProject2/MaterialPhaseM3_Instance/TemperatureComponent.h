// TemperatureComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TemperatureComponent.generated.h"

UCLASS(ClassGroup=(Material), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UTemperatureComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTemperatureComponent();

    UFUNCTION(BlueprintCallable, Category = "Temperature")
    void UpdateTemperature(float AmbientTemperature, float Conductivity, float DeltaHours);

    UFUNCTION(BlueprintCallable, Category = "Temperature")
    void ApplyHeat(float Amount);

    UFUNCTION(BlueprintCallable, Category = "Temperature")
    void ApplyCooling(float Amount);

    UFUNCTION(BlueprintPure, Category = "Temperature")
    float GetTemperature() const { return CurrentTemperature; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite) float CurrentTemperature = 20.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float AmbientTemperature = 20.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float HeatCapacity = 1.0f;
};

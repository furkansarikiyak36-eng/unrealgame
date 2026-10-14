// MoistureComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MoistureComponent.generated.h"

UCLASS(ClassGroup=(Material), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UMoistureComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UMoistureComponent();

    UFUNCTION(BlueprintCallable, Category = "Moisture")
    void UpdateMoisture(float AmbientMoisture, float AbsorptionRate, float DeltaDays);

    UFUNCTION(BlueprintCallable, Category = "Moisture")
    void SoakInWater(float Amount);

    UFUNCTION(BlueprintCallable, Category = "Moisture")
    void Dry(float Rate, float DeltaDays);

    UFUNCTION(BlueprintPure, Category = "Moisture")
    float GetMoisture() const { return CurrentMoisture; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite) float CurrentMoisture = 0.3f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float DryingRate = 0.05f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float SoakingRate = 0.2f;
};

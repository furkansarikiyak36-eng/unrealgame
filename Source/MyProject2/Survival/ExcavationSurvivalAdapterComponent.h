#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ExcavationSurvivalAdapterComponent.generated.h"

class USurvivalComponent;

/** Adapter from digging/building actions to the project's existing survival component. */
UCLASS(ClassGroup = (Survival), meta = (BlueprintSpawnableComponent))
class MYPROJECT2_API UExcavationSurvivalAdapterComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UExcavationSurvivalAdapterComponent();

    UFUNCTION(BlueprintCallable, Category = "Survival|Excavation")
    bool TryPayWorkCost(float EffortUnits, FString& OutFailure) const;

    UFUNCTION(BlueprintCallable, Category = "Survival|Environment")
    bool SetLocalEnvironment(float AmbientTemperatureC, float Wetness01, float ShelterHeatC) const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival|Excavation", meta = (ClampMin = "0.0"))
    float StaminaCostPerEffortUnit = 1.0f;

private:
    USurvivalComponent* FindSurvivalComponent() const;
};

#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ExponentialDecayModel.generated.h"
UCLASS()
class MYPROJECT2_API UExponentialDecayModel : public UObject
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="Decay") static float ComputeDecay(float InitialValue,float RatePerDay,float DeltaDays);
    UFUNCTION(BlueprintPure, Category="Decay") static float ComputeEffectiveRate(float BaseRate,float Moisture,float Temperature);
    UFUNCTION(BlueprintPure, Category="Decay") static float ComputeLinearDecay(float InitialValue,float RatePerDay,float DeltaDays);
    UFUNCTION(BlueprintPure, Category="Decay") static float ComputeLogisticDecay(float InitialValue,float Capacity,float Rate,float DeltaDays);
};

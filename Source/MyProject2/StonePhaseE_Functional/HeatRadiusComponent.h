#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HeatRadiusComponent.generated.h"
UCLASS(ClassGroup=(Heat), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UHeatRadiusComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UHeatRadiusComponent();
    UFUNCTION(BlueprintCallable) float GetHeatAtLocation(const FVector& Location) const;
    UFUNCTION(BlueprintCallable) bool IsInWarmZone(const FVector& Location) const;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float HeatRadius = 500.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaxTemperature = 50.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float AmbientTemperature = 10.0f;
};

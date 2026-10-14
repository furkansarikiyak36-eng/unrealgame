#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StoneWeatheringComponent.generated.h"
class UMaterialInstanceDynamic;
UCLASS(ClassGroup=(Environment), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UStoneWeatheringComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UStoneWeatheringComponent();
    UFUNCTION(BlueprintCallable) void ApplyRain(float Intensity, float DeltaTime);
    UFUNCTION(BlueprintCallable) void ApplySnow(float Intensity, float DeltaTime);
    UFUNCTION(BlueprintCallable) float GetWetness() const { return Wetness; }
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Wetness = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float SnowCoverage = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float DryRate = 0.05f;
protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterials;
};

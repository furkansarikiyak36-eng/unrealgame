#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StoneThrowComponent.generated.h"

class AStoneActor;

UCLASS(ClassGroup=(Stone), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UStoneThrowComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UStoneThrowComponent();
    UFUNCTION(BlueprintCallable) void StartCharge();
    UFUNCTION(BlueprintCallable) void ReleaseCharge();
    UFUNCTION(BlueprintCallable) void SetTarget(AStoneActor* Stone);
    UFUNCTION(BlueprintCallable) void DistractAI(const FVector& ImpactLocation, float Radius);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaxChargeTime = 1.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaxForce = 1800.0f;
protected:
    float ChargeTime = 0.0f;
    UPROPERTY() TObjectPtr<AStoneActor> Target = nullptr;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LogPhysicsComponent.generated.h"
UCLASS(ClassGroup=(Log), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API ULogPhysicsComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    ULogPhysicsComponent();
    UFUNCTION(BlueprintCallable) void ApplyRollingImpulse(const FVector& Direction, float Strength);
    UFUNCTION(BlueprintCallable) void ApplyPush(const FVector& Direction, float Strength);
    UFUNCTION(BlueprintCallable) void EnterSleep();
    UFUNCTION(BlueprintCallable) void WakeUp();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Physics") float RollingResistance = 0.4f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Physics") float DragDamping = 2.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Physics") float SleepDelay = 5.0f;
protected:
    float TimeSinceMotion = 0.0f;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};

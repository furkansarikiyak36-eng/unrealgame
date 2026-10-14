#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LogTypes.h"
#include "LogStateMachine.generated.h"

UCLASS(ClassGroup=(Log), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API ULogStateMachine : public UActorComponent
{
    GENERATED_BODY()
public:
    ULogStateMachine();
    UFUNCTION(BlueprintCallable) bool TransitionTo(ELogState NewState, AActor* InstigatorActor = nullptr);
    UFUNCTION(BlueprintPure) ELogState GetState() const { return CurrentState; }
    UFUNCTION(BlueprintPure) bool CanTransitionTo(ELogState NewState) const;
    UFUNCTION(BlueprintPure) float GetTimeInState() const { return TimeInState; }
    UPROPERTY(BlueprintAssignable) FOnLogStateChanged OnStateChanged;
protected:
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) ELogState CurrentState = ELogState::Loose;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) ELogState PreviousState = ELogState::Loose;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) float TimeInState = 0.0f;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
private:
    bool IsTransitionLegal(ELogState From, ELogState To) const;
};

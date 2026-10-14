#pragma once
#include "Core/StructuralMaterialTypes.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MaterialCore/MaterialTypes.h"
#include "StoneStateMachine.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStoneStateChanged, AActor*, Stone, EStructuralNodeState, NewState);

UCLASS(ClassGroup=(Stone), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UStoneStateMachine : public UActorComponent
{
    GENERATED_BODY()
public:
    UStoneStateMachine();
    UFUNCTION(BlueprintCallable) bool TransitionTo(EStructuralNodeState NewState, AActor* Instigator = nullptr);
    UFUNCTION(BlueprintPure) EStructuralNodeState GetState() const { return CurrentState; }
    UFUNCTION(BlueprintPure) bool CanTransitionTo(EStructuralNodeState NewState) const;
    UPROPERTY(BlueprintAssignable) FOnStoneStateChanged OnStateChanged;
protected:
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) EStructuralNodeState CurrentState = EStructuralNodeState::Loose;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) EStructuralNodeState PreviousState = EStructuralNodeState::Loose;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) float TimeInState = 0.0f;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
private:
    bool IsTransitionLegal(EStructuralNodeState From, EStructuralNodeState To) const;
};

#pragma once
#include "Core/StructuralMaterialTypes.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MaterialCore/MaterialTypes.h"
#include "StickTypes.h"
#include "StickStateMachine.generated.h"

UCLASS(ClassGroup=(Stick), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UStickStateMachine : public UActorComponent
{
    GENERATED_BODY()
public:
    UStickStateMachine();
    UFUNCTION(BlueprintCallable) bool TransitionTo(EStructuralNodeState NewState, AActor* Instigator = nullptr);
    UFUNCTION(BlueprintPure) EStructuralNodeState GetState() const { return CurrentState; }
    UFUNCTION(BlueprintPure) bool CanTransitionTo(EStructuralNodeState NewState) const;
    UPROPERTY(BlueprintAssignable) FOnStickStateChanged OnStateChanged;
protected:
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) EStructuralNodeState CurrentState = EStructuralNodeState::Loose;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) EStructuralNodeState PreviousState = EStructuralNodeState::Loose;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) float TimeInState = 0.f;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
private:
    bool IsTransitionLegal(EStructuralNodeState From, EStructuralNodeState To) const;
};

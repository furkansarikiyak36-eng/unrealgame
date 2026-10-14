#include "StoneStateMachine.h"

UStoneStateMachine::UStoneStateMachine() { PrimaryComponentTick.bCanEverTick = true; }

bool UStoneStateMachine::IsTransitionLegal(EStructuralNodeState From, EStructuralNodeState To) const
{
    if (From == To || From == EStructuralNodeState::Destroyed) return false;
    switch (From)
    {
    case EStructuralNodeState::Loose:
        return To == EStructuralNodeState::Carried || To == EStructuralNodeState::Dragging ||
            To == EStructuralNodeState::Installed || To == EStructuralNodeState::Falling ||
            To == EStructuralNodeState::Fractured || To == EStructuralNodeState::Destroyed;
    case EStructuralNodeState::Carried:
        return To == EStructuralNodeState::Loose || To == EStructuralNodeState::Installed ||
            To == EStructuralNodeState::Falling || To == EStructuralNodeState::Destroyed;
    case EStructuralNodeState::Dragging:
        return To == EStructuralNodeState::Loose || To == EStructuralNodeState::Installed || To == EStructuralNodeState::Destroyed;
    case EStructuralNodeState::Installed:
        return To == EStructuralNodeState::Loose || To == EStructuralNodeState::Falling ||
            To == EStructuralNodeState::Fractured || To == EStructuralNodeState::Destroyed;
    case EStructuralNodeState::Falling:
        return To == EStructuralNodeState::Loose || To == EStructuralNodeState::Installed ||
            To == EStructuralNodeState::Fractured || To == EStructuralNodeState::Destroyed;
    case EStructuralNodeState::Fractured:
        return To == EStructuralNodeState::Loose || To == EStructuralNodeState::Destroyed;
    default: return false;
    }
}

bool UStoneStateMachine::CanTransitionTo(EStructuralNodeState NewState) const
{ return IsTransitionLegal(CurrentState, NewState); }

bool UStoneStateMachine::TransitionTo(EStructuralNodeState NewState, AActor* Instigator)
{
    if (!CanTransitionTo(NewState)) return false;
    PreviousState = CurrentState; CurrentState = NewState; TimeInState = 0.0f;
    OnStateChanged.Broadcast(GetOwner(), NewState);
    return true;
}

void UStoneStateMachine::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    TimeInState += DeltaTime;
}

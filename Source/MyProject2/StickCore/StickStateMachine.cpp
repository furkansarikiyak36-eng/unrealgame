#include "StickStateMachine.h"
UStickStateMachine::UStickStateMachine() { PrimaryComponentTick.bCanEverTick = true; }
bool UStickStateMachine::IsTransitionLegal(EStructuralNodeState From, EStructuralNodeState To) const
{
    if (From == To || From == EStructuralNodeState::Destroyed) return false;
    switch (From) {
    case EStructuralNodeState::Loose: return To == EStructuralNodeState::Carried || To == EStructuralNodeState::Dragging || To == EStructuralNodeState::Installed || To == EStructuralNodeState::Falling || To == EStructuralNodeState::Burning || To == EStructuralNodeState::Fractured || To == EStructuralNodeState::Destroyed;
    case EStructuralNodeState::Carried: return To == EStructuralNodeState::Loose || To == EStructuralNodeState::Installed || To == EStructuralNodeState::Falling || To == EStructuralNodeState::Destroyed;
    case EStructuralNodeState::Dragging: return To == EStructuralNodeState::Loose || To == EStructuralNodeState::Carried || To == EStructuralNodeState::Installed || To == EStructuralNodeState::Destroyed;
    case EStructuralNodeState::Installed: return To == EStructuralNodeState::Loose || To == EStructuralNodeState::Falling || To == EStructuralNodeState::Burning || To == EStructuralNodeState::Fractured || To == EStructuralNodeState::Destroyed;
    case EStructuralNodeState::Falling: return To == EStructuralNodeState::Loose || To == EStructuralNodeState::Installed || To == EStructuralNodeState::Burning || To == EStructuralNodeState::Fractured || To == EStructuralNodeState::Destroyed;
    case EStructuralNodeState::Burning: return To == EStructuralNodeState::Loose || To == EStructuralNodeState::Falling || To == EStructuralNodeState::Destroyed || To == EStructuralNodeState::Fractured;
    case EStructuralNodeState::Fractured: return To == EStructuralNodeState::Loose || To == EStructuralNodeState::Destroyed;
    default: return false;
    }
}
bool UStickStateMachine::CanTransitionTo(EStructuralNodeState S) const { return IsTransitionLegal(CurrentState, S); }
bool UStickStateMachine::TransitionTo(EStructuralNodeState S, AActor* Instigator)
{
    if (!CanTransitionTo(S)) return false;
    PreviousState = CurrentState; CurrentState = S; TimeInState = 0.f; OnStateChanged.Broadcast(GetOwner(), S); return true;
}
void UStickStateMachine::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{ Super::TickComponent(DeltaTime, TickType, ThisTickFunction); TimeInState += DeltaTime; }

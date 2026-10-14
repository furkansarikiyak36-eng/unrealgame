#include "Core/LogStateMachine.h"

ULogStateMachine::ULogStateMachine() { PrimaryComponentTick.bCanEverTick = true; PrimaryComponentTick.TickInterval = 0.25f; }

bool ULogStateMachine::IsTransitionLegal(ELogState From, ELogState To) const
{
    if (From == To || From == ELogState::Destroyed) return false;
    switch (From)
    {
    case ELogState::Loose: return To == ELogState::Carried || To == ELogState::Dragging || To == ELogState::Installed || To == ELogState::Falling || To == ELogState::Burning || To == ELogState::Destroyed;
    case ELogState::Carried: return To == ELogState::Loose || To == ELogState::Dragging || To == ELogState::Installed || To == ELogState::Falling || To == ELogState::Burning || To == ELogState::Destroyed;
    case ELogState::Dragging: return To == ELogState::Loose || To == ELogState::Carried || To == ELogState::Installed || To == ELogState::Falling || To == ELogState::Burning || To == ELogState::Destroyed;
    case ELogState::Installed: return To == ELogState::Loose || To == ELogState::Falling || To == ELogState::Burning || To == ELogState::Destroyed;
    case ELogState::Falling: return To == ELogState::Loose || To == ELogState::Installed || To == ELogState::Destroyed || To == ELogState::Burning;
    case ELogState::Burning: return To == ELogState::Destroyed || To == ELogState::Loose || To == ELogState::Falling;
    default: return false;
    }
}

bool ULogStateMachine::CanTransitionTo(ELogState NewState) const { return IsTransitionLegal(CurrentState, NewState); }

bool ULogStateMachine::TransitionTo(ELogState NewState, AActor* InstigatorActor)
{
    if (!CanTransitionTo(NewState)) return false;
    PreviousState = CurrentState; CurrentState = NewState; TimeInState = 0.0f;
    OnStateChanged.Broadcast(GetOwner(), NewState);
    return true;
}

void ULogStateMachine::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction); TimeInState += DeltaTime;
}

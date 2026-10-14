#include "Phase1_Physical/LogPhysicsComponent.h"
#include "Phase1_Physical/LogActor.h"
#include "Components/PrimitiveComponent.h"
ULogPhysicsComponent::ULogPhysicsComponent() { PrimaryComponentTick.bCanEverTick = true; PrimaryComponentTick.TickInterval = 0.1f; }
void ULogPhysicsComponent::ApplyRollingImpulse(const FVector& Direction, float Strength)
{
    if (ALogActor* Log = Cast<ALogActor>(GetOwner())) if (UPrimitiveComponent* P = Cast<UPrimitiveComponent>(Log->GetRootComponent()))
    { const FVector Dir = Direction.GetSafeNormal(); if (!Dir.IsNearlyZero()) P->AddImpulse(Dir * FMath::Max(0.0f, Strength), NAME_None, true); }
}
void ULogPhysicsComponent::ApplyPush(const FVector& Direction, float Strength)
{ if (ALogActor* L=Cast<ALogActor>(GetOwner())) if (UPrimitiveComponent* P=Cast<UPrimitiveComponent>(L->GetRootComponent())) P->AddImpulse(Direction.GetSafeNormal()*FMath::Max(0.0f,Strength), NAME_None, true); }
void ULogPhysicsComponent::EnterSleep() { if (ALogActor* L=Cast<ALogActor>(GetOwner())) if (UPrimitiveComponent* P=Cast<UPrimitiveComponent>(L->GetRootComponent())) P->PutRigidBodyToSleep(); }
void ULogPhysicsComponent::WakeUp() { TimeSinceMotion=0.0f; if (ALogActor* L=Cast<ALogActor>(GetOwner())) if (UPrimitiveComponent* P=Cast<UPrimitiveComponent>(L->GetRootComponent())) P->WakeRigidBody(); }
void ULogPhysicsComponent::TickComponent(float D, ELevelTick T, FActorComponentTickFunction* F)
{
    Super::TickComponent(D,T,F); ALogActor* L=Cast<ALogActor>(GetOwner()); if(!L||L->GetLogState()!=ELogState::Loose)return;
    UPrimitiveComponent* P=Cast<UPrimitiveComponent>(L->GetRootComponent()); if(!P||!P->IsSimulatingPhysics())return;
    if(P->GetComponentVelocity().SizeSquared()<1.0f){TimeSinceMotion+=D;if(TimeSinceMotion>=SleepDelay)P->PutRigidBodyToSleep();}else TimeSinceMotion=0.0f;
}

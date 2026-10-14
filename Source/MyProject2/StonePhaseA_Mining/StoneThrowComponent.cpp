#include "StoneThrowComponent.h"
#include "StoneActor.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

UStoneThrowComponent::UStoneThrowComponent() { PrimaryComponentTick.bCanEverTick = true; }
void UStoneThrowComponent::StartCharge() { ChargeTime = 0.0f; }
void UStoneThrowComponent::ReleaseCharge()
{
    if (!Target || !GetOwner()) return;
    const float Alpha = MaxChargeTime > 0.0f ? FMath::Clamp(ChargeTime / MaxChargeTime, 0.0f, 1.0f) : 1.0f;
    if (UPrimitiveComponent* P = Cast<UPrimitiveComponent>(Target->GetRootComponent()))
    {
        P->SetMobility(EComponentMobility::Movable);
        P->SetSimulatePhysics(true);
        P->AddImpulse(GetOwner()->GetActorForwardVector() * MaxForce * Alpha, NAME_None, true);
    }
    Target->SetStructuralState_Implementation(EStructuralNodeState::Falling);
    Target = nullptr;
    ChargeTime = 0.0f;
}
void UStoneThrowComponent::SetTarget(AStoneActor* Stone) { Target = Stone; }
void UStoneThrowComponent::DistractAI(const FVector& ImpactLocation, float Radius)
{
    // TODO: broadcast an AI-perception noise stimulus rather than enumerating every actor.
    (void)ImpactLocation;
    (void)Radius;
}
void UStoneThrowComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    ChargeTime = FMath::Min(ChargeTime + DeltaTime, FMath::Max(0.0f, MaxChargeTime));
}

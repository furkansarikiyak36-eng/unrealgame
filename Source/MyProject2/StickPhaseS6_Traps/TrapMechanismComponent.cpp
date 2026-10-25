#include "TrapMechanismComponent.h"

#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

UTrapMechanismComponent::UTrapMechanismComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTrapMechanismComponent::Arm()
{
	if (LifecycleState == ETrapLifecycleState::Consumed)
	{
		return;
	}
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CooldownTimer);
	}
	bArmed = true;
	LifecycleState = ETrapLifecycleState::Armed;
}

void UTrapMechanismComponent::Disarm()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CooldownTimer);
	}
	bArmed = false;
	if (LifecycleState != ETrapLifecycleState::Consumed)
	{
		LifecycleState = ETrapLifecycleState::Disarmed;
	}
}

bool UTrapMechanismComponent::Trigger(AActor* Victim)
{
	UWorld* World = GetWorld();
	if (!World || !bArmed || LifecycleState != ETrapLifecycleState::Armed)
	{
		return false;
	}

	const float Now = World->GetTimeSeconds();
	const float SafeCooldown = FMath::Max(0.f, Cooldown);
	if (Now - LastTriggerTime < SafeCooldown)
	{
		return false;
	}

	LastTriggerTime = Now;
	bArmed = false;
	LifecycleState = ETrapLifecycleState::Triggered;

	if (IsValid(Victim) && Victim != GetOwner() && Damage > 0.f)
	{
		UGameplayStatics::ApplyDamage(Victim, Damage, nullptr, GetOwner(), UDamageType::StaticClass());
	}
	OnTrapTriggered.Broadcast(IsValid(Victim) ? Victim : nullptr);

	if (bConsumeOnTrigger)
	{
		LifecycleState = ETrapLifecycleState::Consumed;
		return true;
	}

	LifecycleState = ETrapLifecycleState::Cooldown;
	if (SafeCooldown <= KINDA_SMALL_NUMBER)
	{
		FinishCooldown();
	}
	else
	{
		World->GetTimerManager().SetTimer(
			CooldownTimer, this, &UTrapMechanismComponent::FinishCooldown, SafeCooldown, false);
	}
	return true;
}

void UTrapMechanismComponent::FinishCooldown()
{
	if (LifecycleState != ETrapLifecycleState::Cooldown)
	{
		return;
	}
	bArmed = true;
	LifecycleState = ETrapLifecycleState::Armed;
}

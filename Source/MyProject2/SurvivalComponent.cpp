// Copyright Epic Games, Inc. All Rights Reserved.

#include "SurvivalComponent.h"

USurvivalComponent::USurvivalComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USurvivalComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (bDead)
	{
		return;
	}

	// Stamina: sprinting drains it, otherwise it refills after a short pause.
	if (bSprinting && CanSprint())
	{
		Stamina = FMath::Max(0.f, Stamina - SprintCostPerSecond * DeltaTime);
		StaminaIdleTime = 0.f;
		if (Stamina <= 0.f)
		{
			bExhausted = true;
		}
	}
	else
	{
		StaminaIdleTime += DeltaTime;
		if (StaminaIdleTime >= StaminaRegenDelay)
		{
			Stamina = FMath::Min(MaxValue, Stamina + StaminaRegenPerSecond * DeltaTime);
		}
	}
	if (bExhausted && Stamina >= ExhaustionRecovery)
	{
		bExhausted = false;
	}

	// Needs drain over real minutes.
	Hunger = FMath::Max(0.f, Hunger - MaxValue / (HungerMinutes * 60.f) * DeltaTime);
	Thirst = FMath::Max(0.f, Thirst - MaxValue / (ThirstMinutes * 60.f) * DeltaTime);

	// Empty needs only hurt after a grace period; healthy needs heal slowly.
	if (IsStarving())
	{
		StarvingTime += DeltaTime;
		if (StarvingTime >= StarvationGrace)
		{
			ApplyDamage(StarvationDamagePerSecond * DeltaTime);
		}
	}
	else
	{
		StarvingTime = 0.f;
		if (Hunger > MaxValue * 0.5f && Thirst > MaxValue * 0.5f)
		{
			Health = FMath::Min(MaxValue, Health + HealthRegenPerSecond * DeltaTime);
		}
	}

    // A lightweight exposure model. Region/weather systems provide the inputs;
    // this component remains the authoritative owner of player health.
    const float EffectiveAmbientC = AmbientTemperatureC + ShelterHeatC - Wetness01 * WetnessCoolingC;
    const float TemperatureAlpha = 1.0f - FMath::Exp(-FMath::Max(0.001f, TemperatureResponseRate) * DeltaTime);
    BodyTemperatureC = FMath::Lerp(BodyTemperatureC, EffectiveAmbientC, TemperatureAlpha);
    if (BodyTemperatureC < ColdDamageThresholdC)
    {
        const float ColdExposure = ColdDamageThresholdC - BodyTemperatureC;
        ApplyDamage(ColdExposure * ColdDamagePerDegreePerSecond * DeltaTime);
    }
}

bool USurvivalComponent::TryUseStamina(float Amount)
{
	if (bDead || Stamina < Amount)
	{
		return false;
	}
	Stamina -= Amount;
	StaminaIdleTime = 0.f;
	return true;
}

void USurvivalComponent::Eat(float Amount)
{
	Hunger = FMath::Clamp(Hunger + Amount, 0.f, MaxValue);
}

void USurvivalComponent::Drink(float Amount)
{
	Thirst = FMath::Clamp(Thirst + Amount, 0.f, MaxValue);
}

void USurvivalComponent::ApplyDamage(float Amount)
{
	if (bDead)
	{
		return;
	}
	Health = FMath::Max(0.f, Health - Amount);
	if (Health <= 0.f)
	{
		bDead = true;
		if (OnDeath)
		{
			OnDeath();
		}
	}
}

void USurvivalComponent::ResetAll()
{
	Health = Stamina = Hunger = Thirst = MaxValue;
	bDead = bExhausted = false;
	StarvingTime = StaminaIdleTime = 0.f;
}

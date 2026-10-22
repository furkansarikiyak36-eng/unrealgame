// Copyright Epic Games, Inc. All Rights Reserved.

#include "SurvivalComponent.h"
#include "WorldCore/Events/WorldMechanicEventSubsystem.h"
#include "Engine/World.h"

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
    BodyTemperatureC = 37.0f;
    AmbientTemperatureC = 18.0f;
    Wetness01 = 0.0f;
    ShelterHeatC = 0.0f;
}


void USurvivalComponent::SetEnvironmentalExposure(float AmbientTemperatureInC, float Wetness01In, float ShelterHeatInC)
{
    if (!FMath::IsFinite(AmbientTemperatureInC) || !FMath::IsFinite(Wetness01In)
        || !FMath::IsFinite(ShelterHeatInC))
    {
        return;
    }
    AmbientTemperatureC = FMath::Clamp(AmbientTemperatureInC, -100.0f, 100.0f);
    Wetness01 = FMath::Clamp(Wetness01In, 0.0f, 1.0f);
    ShelterHeatC = FMath::Clamp(ShelterHeatInC, 0.0f, 60.0f);

    if (UWorld* World = GetWorld())
    {
        if (UWorldMechanicEventSubsystem* Events = World->GetSubsystem<UWorldMechanicEventSubsystem>())
        {
            FWorldMechanicEvent Event;
            Event.Type = EWorldMechanicEventType::SurvivalEnvironmentChanged;
            Event.ScalarValue = AmbientTemperatureC;
            Event.SecondaryValue = Wetness01;
            Event.TertiaryValue = ShelterHeatC;
            Event.SourceTag = TEXT("SurvivalComponent");
            Event.SourceActor = GetOwner();
            Events->EnqueueEvent(MoveTemp(Event));
        }
    }
}

bool USurvivalComponent::ExportState(FSurvivalStateSnapshot& OutState) const
{
    OutState = FSurvivalStateSnapshot();
    if (bDead || !FMath::IsFinite(Health) || Health <= 0.0f)
    {
        return false;
    }

    OutState.Health = Health;
    OutState.Stamina = Stamina;
    OutState.Hunger = Hunger;
    OutState.Thirst = Thirst;
    OutState.BodyTemperatureC = BodyTemperatureC;
    OutState.AmbientTemperatureC = AmbientTemperatureC;
    OutState.Wetness01 = Wetness01;
    OutState.ShelterHeatC = ShelterHeatC;
    OutState.StaminaIdleTime = StaminaIdleTime;
    OutState.StarvingTime = StarvingTime;
    OutState.bExhausted = bExhausted;
    return true;
}

bool USurvivalComponent::ImportState(const FSurvivalStateSnapshot& InState)
{
    const float Values[] = { InState.Health, InState.Stamina, InState.Hunger, InState.Thirst,
        InState.BodyTemperatureC, InState.AmbientTemperatureC, InState.Wetness01,
        InState.ShelterHeatC, InState.StaminaIdleTime, InState.StarvingTime };
    for (float Value : Values)
    {
        if (!FMath::IsFinite(Value)) return false;
    }
    if (InState.Health <= 0.0f || InState.Wetness01 < 0.0f || InState.Wetness01 > 1.0f
        || InState.StaminaIdleTime < 0.0f || InState.StarvingTime < 0.0f)
    {
        return false;
    }

    Health = FMath::Clamp(InState.Health, 0.0f, MaxValue);
    Stamina = FMath::Clamp(InState.Stamina, 0.0f, MaxValue);
    Hunger = FMath::Clamp(InState.Hunger, 0.0f, MaxValue);
    Thirst = FMath::Clamp(InState.Thirst, 0.0f, MaxValue);
    BodyTemperatureC = FMath::Clamp(InState.BodyTemperatureC, -100.0f, 100.0f);
    AmbientTemperatureC = FMath::Clamp(InState.AmbientTemperatureC, -100.0f, 100.0f);
    Wetness01 = InState.Wetness01;
    ShelterHeatC = FMath::Clamp(InState.ShelterHeatC, 0.0f, 60.0f);
    StaminaIdleTime = InState.StaminaIdleTime;
    StarvingTime = InState.StarvingTime;
    bExhausted = InState.bExhausted;
    bDead = false;
    bSprinting = false;
    return true;
}

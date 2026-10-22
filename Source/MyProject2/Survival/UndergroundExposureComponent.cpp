// Copyright Epic Games, Inc. All Rights Reserved.
#include "Survival/UndergroundExposureComponent.h"
#include "SurvivalComponent.h"
#include "GameFramework/Actor.h"

namespace
{
	constexpr int32 CurrentExposureSchemaVersion = 1;
	constexpr float MinBodyTemperatureC = 25.0f;
	constexpr float MaxBodyTemperatureC = 45.0f;
	constexpr float ThermalLowThresholdC = 35.0f;
	constexpr float ThermalHighThresholdC = 39.2f;
	constexpr float DarknessThreshold01 = 0.15f;
	constexpr float MaxAccumulatedSeconds = 100000000.0f;
}

bool FSurvivalExposureSample::IsValidSample(FString& OutError) const
{
	OutError.Reset();
	if (!FMath::IsFinite(AmbientTemperatureC) || AmbientTemperatureC < -80.0f || AmbientTemperatureC > 80.0f)
	{
		OutError = TEXT("Ambient temperature must be finite and in [-80, 80] C.");
		return false;
	}
	if (!FMath::IsFinite(AmbientMoisture01) || AmbientMoisture01 < 0.0f || AmbientMoisture01 > 1.0f)
	{
		OutError = TEXT("Ambient moisture must be finite and in [0, 1].");
		return false;
	}
	if (!FMath::IsFinite(LightLevel01) || LightLevel01 < 0.0f || LightLevel01 > 1.0f)
	{
		OutError = TEXT("Light level must be finite and in [0, 1].");
		return false;
	}
	if (!FMath::IsFinite(WaterDepthCm) || WaterDepthCm < 0.0f || WaterDepthCm > 100000.0f)
	{
		OutError = TEXT("Water depth must be finite and in [0, 100000] cm.");
		return false;
	}
	return true;
}

UUndergroundExposureComponent::UUndergroundExposureComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UUndergroundExposureComponent::BeginPlay()
{
	Super::BeginPlay();
	ResolveSurvivalComponent();
}

void UUndergroundExposureComponent::ResolveSurvivalComponent()
{
	if (!Survival && GetOwner())
	{
		Survival = GetOwner()->FindComponentByClass<USurvivalComponent>();
	}
}

bool UUndergroundExposureComponent::SetEnvironmentSample(const FSurvivalExposureSample& Sample, FString& OutError)
{
	if (!Sample.IsValidSample(OutError))
	{
		return false;
	}
	CurrentEnvironment = Sample;
	return true;
}

float UUndergroundExposureComponent::CalculateStaminaCostMultiplier(float InWetness01, float InBodyTemperatureC, bool bInWater)
{
	if (!FMath::IsFinite(InWetness01) || !FMath::IsFinite(InBodyTemperatureC))
	{
		return 2.0f;
	}
	const float Wetness = FMath::Clamp(InWetness01, 0.0f, 1.0f);
	const float ColdPenalty = FMath::Clamp((35.5f - InBodyTemperatureC) * 0.08f, 0.0f, 0.45f);
	const float HeatPenalty = FMath::Clamp((InBodyTemperatureC - 38.5f) * 0.08f, 0.0f, 0.35f);
	const float WaterPenalty = bInWater ? 0.35f : 0.0f;
	return FMath::Clamp(1.0f + Wetness * 0.25f + ColdPenalty + HeatPenalty + WaterPenalty, 1.0f, 2.0f);
}

float UUndergroundExposureComponent::GetStaminaCostMultiplier() const
{
	return CalculateStaminaCostMultiplier(Wetness01, BodyTemperatureC, IsInWater());
}

float UUndergroundExposureComponent::GetVisibilityFactor() const
{
	const float Light = FMath::Clamp(CurrentEnvironment.LightLevel01, 0.0f, 1.0f);
	return CurrentEnvironment.bIsUnderground
		? FMath::Clamp(0.1f + Light * 0.9f, 0.1f, 1.0f)
		: FMath::Clamp(0.35f + Light * 0.65f, 0.35f, 1.0f);
}

void UUndergroundExposureComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!FMath::IsFinite(DeltaTime) || DeltaTime <= 0.0f)
	{
		return;
	}

	ResolveSurvivalComponent();
	// Forward-only and bounded: loads do not simulate offline time.
	const float SafeDelta = FMath::Clamp(DeltaTime, 0.0f, FMath::Clamp(MaxSimulationStepSeconds, 0.0f, 1.0f));
	if (SafeDelta <= 0.0f)
	{
		return;
	}

	const bool bWaterContact = IsInWater();
	if (bWaterContact)
	{
		ContinuousWaterExposureSeconds = FMath::Min(MaxAccumulatedSeconds, ContinuousWaterExposureSeconds + SafeDelta);
		Wetness01 = FMath::Min(1.0f, Wetness01 + FMath::Max(0.0f, WaterSoakPerSecond) * SafeDelta);
	}
	else
	{
		ContinuousWaterExposureSeconds = 0.0f;
		if (CurrentEnvironment.AmbientMoisture01 > 0.0f)
		{
			Wetness01 = FMath::Min(
				1.0f,
				Wetness01 + FMath::Max(0.0f, WetnessAbsorptionPerSecond)
					* CurrentEnvironment.AmbientMoisture01 * SafeDelta);
		}
		else
		{
			const float DryingScale = CurrentEnvironment.bIsUnderground ? 0.5f : 1.0f;
			Wetness01 = FMath::Max(
				0.0f,
				Wetness01 - FMath::Max(0.0f, DryingPerSecond) * DryingScale * SafeDelta);
		}
	}

	const float WetThermalScale = 1.0f + Wetness01 * 2.0f + (bWaterContact ? 2.0f : 0.0f);
	const float ExchangeRate = FMath::Clamp(TemperatureExchangeRate, 0.0f, 0.01f) * WetThermalScale;
	BodyTemperatureC = FMath::Clamp(
		BodyTemperatureC + (CurrentEnvironment.AmbientTemperatureC - BodyTemperatureC) * ExchangeRate * SafeDelta,
		MinBodyTemperatureC,
		MaxBodyTemperatureC);

	if (CurrentEnvironment.bIsUnderground && CurrentEnvironment.LightLevel01 < DarknessThreshold01)
	{
		DarknessExposureSeconds = FMath::Min(MaxAccumulatedSeconds, DarknessExposureSeconds + SafeDelta);
	}
	else
	{
		DarknessExposureSeconds = FMath::Max(0.0f, DarknessExposureSeconds - SafeDelta * 2.0f);
	}

	const bool bThermalStress = BodyTemperatureC < ThermalLowThresholdC || BodyTemperatureC > ThermalHighThresholdC;
	if (bThermalStress)
	{
		ThermalStressSeconds = FMath::Min(MaxAccumulatedSeconds, ThermalStressSeconds + SafeDelta);
		if (Survival && ThermalStressSeconds >= FMath::Max(0.0f, ThermalStressGraceSeconds))
		{
			const float DamagePerSecond = FMath::Clamp(ThermalHealthDamagePerSecond, 0.0f, 100.0f);
			if (DamagePerSecond > 0.0f)
			{
				Survival->ApplyDamage(DamagePerSecond * SafeDelta);
			}
		}
	}
	else
	{
		ThermalStressSeconds = FMath::Max(0.0f, ThermalStressSeconds - SafeDelta * 2.0f);
	}
}

FSurvivalExposureSnapshot UUndergroundExposureComponent::CaptureSnapshot() const
{
	FSurvivalExposureSnapshot Snapshot;
	Snapshot.SchemaVersion = CurrentExposureSchemaVersion;
	Snapshot.Environment = CurrentEnvironment;
	Snapshot.BodyTemperatureC = BodyTemperatureC;
	Snapshot.Wetness01 = Wetness01;
	Snapshot.ThermalStressSeconds = ThermalStressSeconds;
	Snapshot.DarknessExposureSeconds = DarknessExposureSeconds;
	Snapshot.ContinuousWaterExposureSeconds = ContinuousWaterExposureSeconds;
	return Snapshot;
}

bool UUndergroundExposureComponent::ValidateSnapshot(const FSurvivalExposureSnapshot& Snapshot, FString& OutError) const
{
	OutError.Reset();
	if (Snapshot.SchemaVersion != CurrentExposureSchemaVersion)
	{
		OutError = TEXT("Unsupported survival-exposure snapshot version.");
		return false;
	}

	FString SampleError;
	if (!Snapshot.Environment.IsValidSample(SampleError))
	{
		OutError = FString::Printf(TEXT("Exposure snapshot environment is invalid: %s"), *SampleError);
		return false;
	}
	const auto IsValidTimer = [](float Value)
	{
		return FMath::IsFinite(Value) && Value >= 0.0f && Value <= MaxAccumulatedSeconds;
	};
	if (!FMath::IsFinite(Snapshot.BodyTemperatureC)
		|| Snapshot.BodyTemperatureC < MinBodyTemperatureC
		|| Snapshot.BodyTemperatureC > MaxBodyTemperatureC
		|| !FMath::IsFinite(Snapshot.Wetness01)
		|| Snapshot.Wetness01 < 0.0f || Snapshot.Wetness01 > 1.0f
		|| !IsValidTimer(Snapshot.ThermalStressSeconds)
		|| !IsValidTimer(Snapshot.DarknessExposureSeconds)
		|| !IsValidTimer(Snapshot.ContinuousWaterExposureSeconds))
	{
		OutError = TEXT("Exposure snapshot contains invalid physiological values or timers.");
		return false;
	}
	return true;
}

bool UUndergroundExposureComponent::RestoreSnapshot(const FSurvivalExposureSnapshot& Snapshot, FString& OutError)
{
	if (!ValidateSnapshot(Snapshot, OutError))
	{
		return false;
	}
	// State-only restore. No Tick or damage callback is invoked here.
	CurrentEnvironment = Snapshot.Environment;
	BodyTemperatureC = Snapshot.BodyTemperatureC;
	Wetness01 = Snapshot.Wetness01;
	ThermalStressSeconds = Snapshot.ThermalStressSeconds;
	DarknessExposureSeconds = Snapshot.DarknessExposureSeconds;
	ContinuousWaterExposureSeconds = Snapshot.ContinuousWaterExposureSeconds;
	ResolveSurvivalComponent();
	return true;
}

void UUndergroundExposureComponent::RecordExertionHeat(float TemperatureRiseC)
{
	if (!FMath::IsFinite(TemperatureRiseC) || TemperatureRiseC <= 0.0f)
	{
		return;
	}
	BodyTemperatureC = FMath::Clamp(
		BodyTemperatureC + FMath::Min(TemperatureRiseC, 0.15f),
		MinBodyTemperatureC,
		MaxBodyTemperatureC);
}

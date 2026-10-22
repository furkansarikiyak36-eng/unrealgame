// Copyright Epic Games, Inc. All Rights Reserved.
#include "Survival/SurvivalExcavationAdapter.h"
#include "Survival/UndergroundExposureComponent.h"
#include "SurvivalComponent.h"
#include "GameFramework/Actor.h"

USurvivalExcavationAdapter::USurvivalExcavationAdapter()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USurvivalExcavationAdapter::BeginPlay()
{
	Super::BeginPlay();
	ResolveComponents();
}

void USurvivalExcavationAdapter::ResolveComponents()
{
	if (!GetOwner())
	{
		return;
	}
	if (!Survival)
	{
		Survival = GetOwner()->FindComponentByClass<USurvivalComponent>();
	}
	if (!Exposure)
	{
		Exposure = GetOwner()->FindComponentByClass<UUndergroundExposureComponent>();
	}
}

bool USurvivalExcavationAdapter::CalculateEffectiveCost(
	ESurvivalExcavationAction Action, float BaseCost, float Wetness01, float BodyTemperatureC, bool bInWater, float& OutEffectiveCost)
{
	OutEffectiveCost = 0.0f;
	if (!FMath::IsFinite(BaseCost) || BaseCost < 0.0f || BaseCost > 1000000.0f
		|| !FMath::IsFinite(Wetness01) || Wetness01 < 0.0f || Wetness01 > 1.0f
		|| !FMath::IsFinite(BodyTemperatureC) || BodyTemperatureC < 25.0f || BodyTemperatureC > 45.0f
		|| Action < ESurvivalExcavationAction::ToolSwing
		|| Action > ESurvivalExcavationAction::CarryLoad)
	{
		return false;
	}

	float ActionMultiplier = 1.0f;
	switch (Action)
	{
	case ESurvivalExcavationAction::ToolSwing: ActionMultiplier = 1.0f; break;
	case ESurvivalExcavationAction::HeavyDig: ActionMultiplier = 1.10f; break;
	case ESurvivalExcavationAction::PlaceShoring: ActionMultiplier = 1.0f; break;
	case ESurvivalExcavationAction::MoveShoring: ActionMultiplier = 0.85f; break;
	case ESurvivalExcavationAction::RepairShoring: ActionMultiplier = 0.90f; break;
	case ESurvivalExcavationAction::CarryLoad: ActionMultiplier = 1.0f; break;
	default: return false;
	}

	const float EnvironmentMultiplier =
		UUndergroundExposureComponent::CalculateStaminaCostMultiplier(Wetness01, BodyTemperatureC, bInWater);
	OutEffectiveCost = BaseCost * ActionMultiplier * EnvironmentMultiplier;
	return FMath::IsFinite(OutEffectiveCost) && OutEffectiveCost >= 0.0f && OutEffectiveCost <= 2000000.0f;
}

bool USurvivalExcavationAdapter::CanPayAction(
	ESurvivalExcavationAction Action, float BaseCost, float& OutEffectiveCost, FString& OutError) const
{
	OutError.Reset();
	OutEffectiveCost = 0.0f;
	if (!Survival)
	{
		OutError = TEXT("The owning actor has no SurvivalComponent.");
		return false;
	}

	const float Wetness = Exposure ? Exposure->GetWetness01() : 0.0f;
	const float BodyTemperature = Exposure ? Exposure->GetBodyTemperatureC() : 37.0f;
	const bool bInWater = Exposure ? Exposure->IsInWater() : false;
	if (!CalculateEffectiveCost(Action, BaseCost, Wetness, BodyTemperature, bInWater, OutEffectiveCost))
	{
		OutError = TEXT("Action cost or environmental state is invalid.");
		return false;
	}
	if (Survival->GetStamina() < OutEffectiveCost)
	{
		OutError = TEXT("Not enough stamina for this action.");
		return false;
	}
	return true;
}

bool USurvivalExcavationAdapter::TryPayAction(
	ESurvivalExcavationAction Action, float BaseCost, float& OutEffectiveCost, FString& OutError)
{
	if (!CanPayAction(Action, BaseCost, OutEffectiveCost, OutError))
	{
		return false;
	}
	if (!Survival->TryUseStamina(OutEffectiveCost))
	{
		OutError = TEXT("Stamina changed before the action could be paid.");
		return false;
	}

	if (Exposure && Action != ESurvivalExcavationAction::CarryLoad)
	{
		float HeatRise = 0.003f;
		switch (Action)
		{
		case ESurvivalExcavationAction::HeavyDig: HeatRise = 0.006f; break;
		case ESurvivalExcavationAction::PlaceShoring: HeatRise = 0.004f; break;
		case ESurvivalExcavationAction::MoveShoring: HeatRise = 0.005f; break;
		case ESurvivalExcavationAction::RepairShoring: HeatRise = 0.003f; break;
		default: break;
		}
		Exposure->RecordExertionHeat(HeatRise);
	}
	return true;
}

bool USurvivalExcavationAdapter::SetEnvironmentSample(const FSurvivalExposureSample& Sample, FString& OutError)
{
	OutError.Reset();
	ResolveComponents();
	if (!Exposure)
	{
		OutError = TEXT("The owning actor has no UndergroundExposureComponent.");
		return false;
	}
	return Exposure->SetEnvironmentSample(Sample, OutError);
}

float USurvivalExcavationAdapter::GetActionEnvironmentMultiplier() const
{
	return Exposure ? Exposure->GetStaminaCostMultiplier() : 1.0f;
}

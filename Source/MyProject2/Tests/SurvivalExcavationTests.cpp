// Copyright Epic Games, Inc. All Rights Reserved.
#if WITH_DEV_AUTOMATION_TESTS

#include "Survival/SurvivalExcavationAdapter.h"
#include "Survival/UndergroundExposureComponent.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSurvivalActionCostDeterminismTest,
	"MyProject2.World.Survival.ActionCostsAreValidatedAndEnvironmentScaled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSurvivalActionCostDeterminismTest::RunTest(const FString& Parameters)
{
	float BaseCostResult = 0.0f;
	TestTrue(TEXT("A normal tool swing cost should be accepted"),
		USurvivalExcavationAdapter::CalculateEffectiveCost(ESurvivalExcavationAction::ToolSwing, 8.0f, 0.0f, 37.0f, false, BaseCostResult));
	TestTrue(TEXT("Normal dry ambient cost remains the configured baseline"), FMath::IsNearlyEqual(BaseCostResult, 8.0f, 0.001f));

	float DigCost = 0.0f;
	TestTrue(TEXT("Heavy digging cost should be calculated"),
		USurvivalExcavationAdapter::CalculateEffectiveCost(ESurvivalExcavationAction::HeavyDig, 8.0f, 0.0f, 37.0f, false, DigCost));
	TestTrue(TEXT("Heavy digging has a defined extra action multiplier"), DigCost > BaseCostResult);

	float WetCost = 0.0f;
	TestTrue(TEXT("Wet/cold cost should be calculated"),
		USurvivalExcavationAdapter::CalculateEffectiveCost(ESurvivalExcavationAction::HeavyDig, 8.0f, 0.8f, 34.0f, true, WetCost));
	TestTrue(TEXT("Wet, cold water exposure costs more stamina"), WetCost > DigCost);

	float InvalidCost = 0.0f;
	TestFalse(TEXT("Negative action costs fail closed"),
		USurvivalExcavationAdapter::CalculateEffectiveCost(ESurvivalExcavationAction::ToolSwing, -1.0f, 0.0f, 37.0f, false, InvalidCost));
	TestFalse(TEXT("Non-finite costs fail closed"),
		USurvivalExcavationAdapter::CalculateEffectiveCost(ESurvivalExcavationAction::ToolSwing, TNumericLimits<float>::QuietNaN(), 0.0f, 37.0f, false, InvalidCost));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSurvivalEnvironmentSampleValidationTest,
	"MyProject2.World.Survival.ExposureSampleClampsAndRejectsInvalid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSurvivalEnvironmentSampleValidationTest::RunTest(const FString& Parameters)
{
	FSurvivalExposureSample Sample;
	FString Error;
	TestTrue(TEXT("Default environment sample is valid"), Sample.IsValidSample(Error));

	Sample.AmbientTemperatureC = 1000.0f;
	TestFalse(TEXT("Impossible ambient temperature is rejected"), Sample.IsValidSample(Error));
	TestFalse(TEXT("Rejected sample gives a reason"), Error.IsEmpty());

	Sample.AmbientTemperatureC = 12.0f;
	Sample.LightLevel01 = -0.1f;
	TestFalse(TEXT("Light values outside [0,1] are rejected"), Sample.IsValidSample(Error));

	Sample.LightLevel01 = 0.2f;
	Sample.AmbientMoisture01 = 1.2f;
	TestFalse(TEXT("Ambient moisture values outside [0,1] are rejected"), Sample.IsValidSample(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSurvivalExposureSnapshotRoundTripTest,
	"MyProject2.World.Survival.ExposureSnapshotRestoresWithoutCatchup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSurvivalExposureSnapshotRoundTripTest::RunTest(const FString& Parameters)
{
	UUndergroundExposureComponent* Component = NewObject<UUndergroundExposureComponent>();
	if (!TestNotNull(TEXT("Exposure component should be constructible without a world"), Component))
	{
		return false;
	}

	FSurvivalExposureSnapshot Source;
	Source.Environment.AmbientTemperatureC = 2.0f;
	Source.Environment.AmbientMoisture01 = 0.7f;
	Source.Environment.LightLevel01 = 0.05f;
	Source.Environment.bIsUnderground = true;
	Source.Environment.bInWater = true;
	Source.Environment.WaterDepthCm = 80.0f;
	Source.BodyTemperatureC = 34.0f;
	Source.Wetness01 = 0.85f;
	Source.ThermalStressSeconds = 60.0f;
	Source.DarknessExposureSeconds = 120.0f;
	Source.ContinuousWaterExposureSeconds = 30.0f;

	FString Error;
	TestTrue(TEXT("Snapshot state should restore"), Component->RestoreSnapshot(Source, Error));
	if (!Error.IsEmpty())
	{
		AddError(Error);
		return false;
	}
	const FSurvivalExposureSnapshot Restored = Component->CaptureSnapshot();
	TestTrue(TEXT("Body temperature round-trips"), FMath::IsNearlyEqual(Restored.BodyTemperatureC, Source.BodyTemperatureC));
	TestTrue(TEXT("Wetness round-trips"), FMath::IsNearlyEqual(Restored.Wetness01, Source.Wetness01));
	TestTrue(TEXT("Thermal timer is not advanced by restore"), FMath::IsNearlyEqual(Restored.ThermalStressSeconds, Source.ThermalStressSeconds));
	TestTrue(TEXT("Darkness timer is not advanced by restore"), FMath::IsNearlyEqual(Restored.DarknessExposureSeconds, Source.DarknessExposureSeconds));
	TestTrue(TEXT("Water timer is not advanced by restore"), FMath::IsNearlyEqual(Restored.ContinuousWaterExposureSeconds, Source.ContinuousWaterExposureSeconds));

	Source.SchemaVersion = 999;
	TestFalse(TEXT("Unknown save schema version is rejected"), Component->RestoreSnapshot(Source, Error));
	return true;
}
#endif // WITH_DEV_AUTOMATION_TESTS

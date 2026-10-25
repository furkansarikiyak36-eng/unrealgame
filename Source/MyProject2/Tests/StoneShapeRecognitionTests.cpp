#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "StonePhaseF_Environment/StoneShapeRecognitionSubsystem.h"

namespace
{
	TArray<FVector> MakeRing(int32 Count, float Radius, float GroundZ = 0.f)
	{
		TArray<FVector> Points;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float Angle = 2.f * PI * static_cast<float>(Index) / static_cast<float>(Count);
			Points.Add(FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, GroundZ));
		}
		return Points;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoneShapeRecognitionRegularRingTest,
	"MyProject2.Construction.StoneShapeRecognition.RegularRing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoneShapeRecognitionRegularRingTest::RunTest(const FString& Parameters)
{
	const TArray<FVector> Points = MakeRing(7, 50.f);
	FVector Center;
	float Radius = 0.f, Error = 0.f, MaxGap = 360.f;
	TestTrue(TEXT("seven evenly distributed stones form a hearth ring"),
		UStoneShapeRecognitionSubsystem::EvaluateHearthRing(Points, Center, Radius, Error, MaxGap));
	TestTrue(TEXT("center remains near origin"), Center.Size2D() < 0.1f);
	TestTrue(TEXT("radius is 50 cm"), FMath::IsNearlyEqual(Radius, 50.f, 0.1f));
	TestTrue(TEXT("radial error is near zero"), Error < 0.1f);
	TestTrue(TEXT("angular gap is below 60 degrees"), MaxGap < 60.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoneShapeRecognitionLineIsNotRingTest,
	"MyProject2.Construction.StoneShapeRecognition.LineRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoneShapeRecognitionLineIsNotRingTest::RunTest(const FString& Parameters)
{
	TArray<FVector> Points;
	for (int32 Index = 0; Index < 7; ++Index)
	{
		Points.Add(FVector((Index - 3) * 20.f, 0.f, 0.f));
	}
	FVector Center;
	float Radius = 0.f, Error = 0.f, MaxGap = 0.f;
	TestFalse(TEXT("a straight line cannot be interpreted as a hearth"),
		UStoneShapeRecognitionSubsystem::EvaluateHearthRing(Points, Center, Radius, Error, MaxGap));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoneShapeRecognitionUnevenGroundRejectedTest,
	"MyProject2.Construction.StoneShapeRecognition.UnevenGroundRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoneShapeRecognitionUnevenGroundRejectedTest::RunTest(const FString& Parameters)
{
	TArray<FVector> Points = MakeRing(7, 50.f);
	Points[3].Z = 40.f;
	FVector Center;
	float Radius = 0.f, Error = 0.f, MaxGap = 0.f;
	TestFalse(TEXT("stones on a sharply different height do not form one hearth"),
		UStoneShapeRecognitionSubsystem::EvaluateHearthRing(Points, Center, Radius, Error, MaxGap));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoneShapeRecognitionPartialArcRejectedTest,
	"MyProject2.Construction.StoneShapeRecognition.PartialArcRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoneShapeRecognitionPartialArcRejectedTest::RunTest(const FString& Parameters)
{
	TArray<FVector> Points;
	for (int32 Index = 0; Index < 7; ++Index)
	{
		const float Angle = FMath::DegreesToRadians(-25.f + Index * 8.f);
		Points.Add(FVector(FMath::Cos(Angle) * 50.f, FMath::Sin(Angle) * 50.f, 0.f));
	}
	FVector Center;
	float Radius = 0.f, Error = 0.f, MaxGap = 0.f;
	TestFalse(TEXT("a partial arc with a large uncovered angle is not a closed hearth"),
		UStoneShapeRecognitionSubsystem::EvaluateHearthRing(Points, Center, Radius, Error, MaxGap));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

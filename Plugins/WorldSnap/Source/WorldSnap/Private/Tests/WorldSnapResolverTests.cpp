// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "WorldSnapResolver.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldSnapResolverNearestCompatibleTest,
	"WorldSnap.Resolver.NearestCompatibleCandidate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldSnapResolverNearestCompatibleTest::RunTest(const FString& Parameters)
{
	TArray<WorldSnap::FPoint> Points;

	WorldSnap::FPoint First;
	First.Id = TEXT("Top_00");
	First.Group = TEXT("Top");
	First.Position = FVector(0.f, 0.f, 18.f);
	First.Normal = FVector::UpVector;
	First.Axis = FVector::ForwardVector;
	First.SourceIndex = 0;
	Points.Add(First);

	WorldSnap::FPoint Second = First;
	Second.Id = TEXT("Top_01");
	Second.Position = FVector(75.f, 0.f, 18.f);
	Second.SourceIndex = 1;
	Points.Add(Second);

	WorldSnap::FPoint WrongFace = Second;
	WrongFace.Id = TEXT("Front_01");
	WrongFace.Group = TEXT("Front");
	WrongFace.Position = FVector(75.f, 18.f, 18.f);
	WrongFace.Normal = FVector::RightVector;
	WrongFace.SourceIndex = 2;
	Points.Add(WrongFace);

	WorldSnap::FQuery Query;
	Query.HitPoint = FVector(67.f, 0.f, 18.f);
	Query.HitNormal = FVector::UpVector;
	Query.MaxDistanceCm = 125.f;
	Query.bUseAxisConstraint = true;
	Query.MaxAlongAxisErrorCm = 45.f;

	WorldSnap::FResolveResult Result;
	TestTrue(TEXT("a compatible candidate resolves"), FWorldSnapResolver::Resolve(Points, Query, Result));
	TestTrue(TEXT("nearest compatible module point wins"), Result.Point.Id == FName(TEXT("Top_01")));
	TestEqual(TEXT("caller source index is retained"), Result.Point.SourceIndex, 1);
	TestTrue(TEXT("result reports success"), Result.Status == WorldSnap::EResolveStatus::Resolved);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldSnapResolverTieIsDeterministicTest,
	"WorldSnap.Resolver.DeterministicTieBreak",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldSnapResolverTieIsDeterministicTest::RunTest(const FString& Parameters)
{
	TArray<WorldSnap::FPoint> Points;
	for (int32 Index = 0; Index < 2; ++Index)
	{
		WorldSnap::FPoint Point;
		Point.Id = FName(*FString::Printf(TEXT("Top_%02d"), Index));
		Point.Group = TEXT("Top");
		Point.Position = FVector(Index * 75.f, 0.f, 18.f);
		Point.Normal = FVector::UpVector;
		Point.Axis = FVector::ForwardVector;
		Point.SourceIndex = Index;
		Points.Add(Point);
	}

	WorldSnap::FQuery Query;
	Query.HitPoint = FVector(37.5f, 0.f, 18.f);
	Query.HitNormal = FVector::UpVector;
	Query.MaxDistanceCm = 125.f;
	Query.bUseAxisConstraint = true;
	Query.MaxAlongAxisErrorCm = 45.f;

	WorldSnap::FResolveResult Result;
	TestTrue(TEXT("tie still produces a result"), FWorldSnapResolver::Resolve(Points, Query, Result));
	TestTrue(TEXT("first generated candidate wins equal scores"), Result.Point.Id == FName(TEXT("Top_00")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldSnapResolverGroupFilterTest,
	"WorldSnap.Resolver.RequiredGroupFilter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldSnapResolverGroupFilterTest::RunTest(const FString& Parameters)
{
	TArray<WorldSnap::FPoint> Points;
	WorldSnap::FPoint Point;
	Point.Id = TEXT("Front_00");
	Point.Group = TEXT("Front");
	Point.Position = FVector::ZeroVector;
	Point.Normal = FVector::ForwardVector;
	Points.Add(Point);

	WorldSnap::FQuery Query;
	Query.HitPoint = FVector::ZeroVector;
	Query.HitNormal = FVector::ForwardVector;
	Query.RequiredGroup = TEXT("Top");

	WorldSnap::FResolveResult Result;
	TestFalse(TEXT("a different snap group is rejected"), FWorldSnapResolver::Resolve(Points, Query, Result));
	TestTrue(TEXT("rejected candidates return a specific status"), Result.Status == WorldSnap::EResolveStatus::NoCompatiblePoint);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

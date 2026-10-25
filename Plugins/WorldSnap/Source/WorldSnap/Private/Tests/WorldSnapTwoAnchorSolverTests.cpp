// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "WorldSnapTwoAnchorSolver.h"

namespace
{
	WorldSnap::FTwoAnchorQuery MakeQuery(const FVector& Start, const FVector& End)
	{
		WorldSnap::FTwoAnchorQuery Query;
		Query.StartAnchor.Id = TEXT("StartSocket");
		Query.StartAnchor.Kind = WorldSnap::EAnchorKind::Ground;
		Query.StartAnchor.Position = Start;
		Query.StartAnchor.Normal = FVector::UpVector;
		Query.StartAnchor.bLoadBearing = true;

		Query.EndAnchor.Id = TEXT("EndSocket");
		Query.EndAnchor.Kind = WorldSnap::EAnchorKind::PostTop;
		Query.EndAnchor.Position = End;
		Query.EndAnchor.Normal = FVector::UpVector;
		Query.EndAnchor.bLoadBearing = true;

		Query.PieceLengthCm = 600.f;
		Query.LocalMinX = -300.f;
		Query.LocalMaxX = 300.f;
		Query.LengthToleranceCm = 2.f;
		Query.MinPitchDegrees = -80.f;
		Query.MaxPitchDegrees = 80.f;
		return Query;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldSnapTwoAnchorLevelPlacementTest,
	"WorldSnap.TwoAnchor.LevelPlacement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldSnapTwoAnchorLevelPlacementTest::RunTest(const FString& Parameters)
{
	const WorldSnap::FTwoAnchorQuery Query = MakeQuery(
		FVector(-300.f, 0.f, 0.f), FVector(300.f, 0.f, 0.f));

	WorldSnap::FTwoAnchorResult Result;
	TestTrue(TEXT("level anchors solve"), FWorldSnapTwoAnchorSolver::Solve(Query, Result));
	TestTrue(TEXT("status is resolved"), Result.Status == WorldSnap::ETwoAnchorStatus::Resolved);
	TestTrue(TEXT("local start meets anchor"), Result.SolvedStart.Equals(Query.StartAnchor.Position, 0.1f));
	TestTrue(TEXT("local end meets anchor"), Result.SolvedEnd.Equals(Query.EndAnchor.Position, 0.1f));
	TestTrue(TEXT("zero pitch"), FMath::IsNearlyZero(Result.PitchDegrees, 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldSnapTwoAnchorSlopePitchTest,
	"WorldSnap.TwoAnchor.SlopePitch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldSnapTwoAnchorSlopePitchTest::RunTest(const FString& Parameters)
{
	const WorldSnap::FTwoAnchorQuery Query = MakeQuery(
		FVector::ZeroVector, FVector(519.61524f, 0.f, 300.f));

	WorldSnap::FTwoAnchorResult Result;
	TestTrue(TEXT("sloped endpoints solve"), FWorldSnapTwoAnchorSolver::Solve(Query, Result));
	TestTrue(TEXT("pitch is 30 degrees"), FMath::IsNearlyEqual(Result.PitchDegrees, 30.f, 0.02f));
	TestTrue(TEXT("start is exact"), Result.SolvedStart.Equals(Query.StartAnchor.Position, 0.1f));
	TestTrue(TEXT("end is exact"), Result.SolvedEnd.Equals(Query.EndAnchor.Position, 0.1f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldSnapTwoAnchorLengthMismatchTest,
	"WorldSnap.TwoAnchor.LengthMismatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldSnapTwoAnchorLengthMismatchTest::RunTest(const FString& Parameters)
{
	const WorldSnap::FTwoAnchorQuery Query = MakeQuery(
		FVector::ZeroVector, FVector(450.f, 0.f, 300.f));

	WorldSnap::FTwoAnchorResult Result;
	TestFalse(TEXT("anchor distance outside piece-length tolerance is rejected"),
		FWorldSnapTwoAnchorSolver::Solve(Query, Result));
	TestTrue(TEXT("mismatch has a dedicated status"),
		Result.Status == WorldSnap::ETwoAnchorStatus::LengthMismatch);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldSnapTwoAnchorNonLoadBearingTest,
	"WorldSnap.TwoAnchor.NonLoadBearing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldSnapTwoAnchorNonLoadBearingTest::RunTest(const FString& Parameters)
{
	WorldSnap::FTwoAnchorQuery Query = MakeQuery(
		FVector::ZeroVector, FVector(519.61524f, 0.f, 300.f));
	Query.StartAnchor.bLoadBearing = false;

	WorldSnap::FTwoAnchorResult Result;
	TestFalse(TEXT("non-load-bearing anchor is rejected"), FWorldSnapTwoAnchorSolver::Solve(Query, Result));
	TestTrue(TEXT("load failure is explicit"),
		Result.Status == WorldSnap::ETwoAnchorStatus::NonLoadBearing);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldSnapTwoAnchorSameAnchorTest,
	"WorldSnap.TwoAnchor.SameAnchor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldSnapTwoAnchorSameAnchorTest::RunTest(const FString& Parameters)
{
	WorldSnap::FTwoAnchorQuery Query = MakeQuery(
		FVector::ZeroVector, FVector(600.f, 0.f, 0.f));
	Query.EndAnchor.Id = Query.StartAnchor.Id;

	WorldSnap::FTwoAnchorResult Result;
	TestFalse(TEXT("one socket cannot be used for both endpoints"), FWorldSnapTwoAnchorSolver::Solve(Query, Result));
	TestTrue(TEXT("duplicate anchor has a dedicated status"),
		Result.Status == WorldSnap::ETwoAnchorStatus::SameAnchor);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldSnapTwoAnchorPitchRangeTest,
	"WorldSnap.TwoAnchor.PitchRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldSnapTwoAnchorPitchRangeTest::RunTest(const FString& Parameters)
{
	const FVector End(600.f * FMath::Cos(FMath::DegreesToRadians(85.f)), 0.f,
		600.f * FMath::Sin(FMath::DegreesToRadians(85.f)));
	const WorldSnap::FTwoAnchorQuery Query = MakeQuery(FVector::ZeroVector, End);

	WorldSnap::FTwoAnchorResult Result;
	TestFalse(TEXT("pitch outside configured range is rejected"), FWorldSnapTwoAnchorSolver::Solve(Query, Result));
	TestTrue(TEXT("pitch rejection has a dedicated status"),
		Result.Status == WorldSnap::ETwoAnchorStatus::PitchOutOfRange);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

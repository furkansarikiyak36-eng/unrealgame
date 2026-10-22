// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "LogSnapSystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLogSnapFoldAxisYawTest,
	"MyProject2.LogPlacement.Snap.FoldAxisYaw",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLogSnapFoldAxisYawTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("0 remains 0"), LogSnapSystem::FoldAxisYawDelta(0.f), 0.f);
	TestEqual(TEXT("135 folds to -45"), LogSnapSystem::FoldAxisYawDelta(135.f), -45.f);
	TestEqual(TEXT("-135 folds to +45"), LogSnapSystem::FoldAxisYawDelta(-135.f), 45.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLogSnapNearestSectorTest,
	"MyProject2.LogPlacement.Snap.NearestSector",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLogSnapNearestSectorTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("parallel"), LogSnapSystem::NearestSector(3.f), 0.f);
	TestEqual(TEXT("positive diagonal"), LogSnapSystem::NearestSector(42.f), 45.f);
	TestEqual(TEXT("negative diagonal"), LogSnapSystem::NearestSector(-43.f), -45.f);
	TestEqual(TEXT("square cross"), LogSnapSystem::NearestSector(87.f), 90.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLogSnapResolveNearestCompatiblePointTest,
	"MyProject2.LogPlacement.Snap.ResolveNearestCompatiblePoint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLogSnapResolveNearestCompatiblePointTest::RunTest(const FString& Parameters)
{
	TArray<LogSnapSystem::FPoint> Points;
	LogSnapSystem::FPoint First;
	First.Id = TEXT("Top_00");
	First.Face = LogSnapSystem::EFace::Top;
	First.Position = FVector(0.f, 0.f, 18.f);
	First.Normal = FVector::UpVector;
	First.Axis = FVector::ForwardVector;
	First.AlongAxis = 0.f;
	Points.Add(First);

	LogSnapSystem::FPoint Second = First;
	Second.Id = TEXT("Top_01");
	Second.Position = FVector(75.f, 0.f, 18.f);
	Second.AlongAxis = 75.f;
	Points.Add(Second);

	LogSnapSystem::FPoint WrongFace = Second;
	WrongFace.Id = TEXT("Front_01");
	WrongFace.Face = LogSnapSystem::EFace::Front;
	WrongFace.Normal = FVector::RightVector;
	WrongFace.Position.Y = 18.f;
	Points.Add(WrongFace);

	LogSnapSystem::FQuery Query;
	Query.HitPoint = FVector(67.f, 0.f, 18.f);
	Query.HitNormal = FVector::UpVector;
	Query.MaxAlongAxisError = 45.f;

	LogSnapSystem::FPoint Resolved;
	TestTrue(TEXT("a compatible point resolves"), LogSnapSystem::Resolve(Points, Query, Resolved));
	TestTrue(TEXT("the closest module point wins"), Resolved.Id == FName(TEXT("Top_01")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLogSnapResolveTieIsDeterministicTest,
	"MyProject2.LogPlacement.Snap.ResolveTieIsDeterministic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLogSnapResolveTieIsDeterministicTest::RunTest(const FString& Parameters)
{
	TArray<LogSnapSystem::FPoint> Points;
	for (int32 Index = 0; Index < 2; ++Index)
	{
		LogSnapSystem::FPoint Point;
		Point.Id = FName(*FString::Printf(TEXT("Top_%02d"), Index));
		Point.Face = LogSnapSystem::EFace::Top;
		Point.Position = FVector(Index * 75.f, 0.f, 18.f);
		Point.Normal = FVector::UpVector;
		Point.Axis = FVector::ForwardVector;
		Point.AlongAxis = Index * 75.f;
		Points.Add(Point);
	}

	LogSnapSystem::FQuery Query;
	Query.HitPoint = FVector(37.5f, 0.f, 18.f);
	Query.HitNormal = FVector::UpVector;
	Query.MaxAlongAxisError = 45.f;
	LogSnapSystem::FPoint Resolved;
	TestTrue(TEXT("tie still resolves"), LogSnapSystem::Resolve(Points, Query, Resolved));
	TestTrue(TEXT("generation order breaks ties deterministically"), Resolved.Id == FName(TEXT("Top_00")));
	return true;
}
#endif

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
#endif

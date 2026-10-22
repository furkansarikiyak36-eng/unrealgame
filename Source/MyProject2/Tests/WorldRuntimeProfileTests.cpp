// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "WorldCore/WorldRuntimeProfile.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldRuntimeProfileDefaultsTest,
    "MyProject2.World.RuntimeProfile.DefaultProfilesValidate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldRuntimeProfileDefaultsTest::RunTest(const FString& Parameters)
{
    const UWorldRuntimeProfileDataAsset* Defaults = NewObject<UWorldRuntimeProfileDataAsset>();
    if (!TestNotNull(TEXT("Default profile asset should be created"), Defaults))
    {
        return false;
    }

    FString Error;
    TestTrue(TEXT("LOW_MID defaults should validate"), Defaults->LowMid.Validate(&Error));
    TestTrue(TEXT("LOW_MID grid spacing should match chunk size"), Defaults->LowMid.HasConsistentChunkGrid());
    TestTrue(TEXT("HIGH_END defaults should validate"), Defaults->HighEnd.Validate(&Error));
    TestTrue(TEXT("HIGH_END grid spacing should match chunk size"), Defaults->HighEnd.HasConsistentChunkGrid());

    TestTrue(
        TEXT("HIGH_END should use finer near-field spacing"),
        Defaults->HighEnd.VoxelSampleSpacingCm < Defaults->LowMid.VoxelSampleSpacingCm);

    TestTrue(
        TEXT("HIGH_END should budget at least as many active editable chunks"),
        Defaults->HighEnd.MaxActiveEditableChunks >= Defaults->LowMid.MaxActiveEditableChunks);

    FWorldRuntimeProfileValues InvalidSettings = Defaults->LowMid;
    InvalidSettings.CellsPerChunkAxis = 16;
    TestFalse(
        TEXT("Inconsistent spacing/chunk grid should fail validation"),
        InvalidSettings.Validate(&Error));
    TestFalse(TEXT("Invalid settings should provide a reason"), Error.IsEmpty());

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

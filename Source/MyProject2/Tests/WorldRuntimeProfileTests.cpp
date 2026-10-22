// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "WorldCore/WorldRuntimeProfile.h"
#include "WorldCore/WorldRuntimeProfileSubsystem.h"
#include "Misc/AutomationTest.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldRuntimeProfileDefaultsTest,
    "MyProject2.World.RuntimeProfile.DefaultProfilesValidate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldRuntimeProfileDefaultsTest::RunTest(const FString& Parameters)
{
    UWorldRuntimeProfileDataAsset* Defaults = NewObject<UWorldRuntimeProfileDataAsset>(GetTransientPackage());
    if (!TestNotNull(TEXT("Default profile asset should be created"), Defaults))
    {
        return false;
    }

    FString Error = TEXT("stale error should be cleared");
    TestTrue(TEXT("LOW_MID defaults should validate"), Defaults->LowMid.Validate(&Error));
    TestTrue(TEXT("Successful profile validation should clear the previous error"), Error.IsEmpty());
    TestTrue(TEXT("LOW_MID grid spacing should match chunk size"), Defaults->LowMid.HasConsistentChunkGrid());
    TestTrue(TEXT("HIGH_END defaults should validate"), Defaults->HighEnd.Validate(&Error));
    TestTrue(TEXT("HIGH_END grid spacing should match chunk size"), Defaults->HighEnd.HasConsistentChunkGrid());

    TestTrue(
        TEXT("HIGH_END should use finer near-field spacing"),
        Defaults->HighEnd.VoxelSampleSpacingCm < Defaults->LowMid.VoxelSampleSpacingCm);

    TestTrue(
        TEXT("HIGH_END should budget at least as many active editable chunks"),
        Defaults->HighEnd.MaxActiveEditableChunks >= Defaults->LowMid.MaxActiveEditableChunks);

    TestTrue(
        TEXT("HIGH_END should allow more mesh work than LOW_MID"),
        Defaults->HighEnd.MaxMeshJobsInFlight > Defaults->LowMid.MaxMeshJobsInFlight);

    FWorldRuntimeProfileValues InvalidSettings = Defaults->LowMid;
    InvalidSettings.CellsPerChunkAxis = 16;
    TestFalse(
        TEXT("Inconsistent spacing/chunk grid should fail validation"),
        InvalidSettings.Validate(&Error));
    TestFalse(TEXT("Invalid settings should provide a reason"), Error.IsEmpty());

    InvalidSettings = Defaults->LowMid;
    InvalidSettings.MaxQueuedMeshJobs = 0;
    TestFalse(
        TEXT("A zero mesh queue capacity should fail validation"),
        InvalidSettings.Validate(&Error));
    TestFalse(TEXT("Queue validation should provide a reason"), Error.IsEmpty());

    Defaults->ActiveProfile = static_cast<EWorldRuntimeProfile>(255);
    TestFalse(
        TEXT("An unsupported profile enum value should be rejected"),
        Defaults->ValidateActiveSettings(Error));
    TestFalse(TEXT("Invalid profile enum should provide a reason"), Error.IsEmpty());

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldRuntimeProfileSubsystemLockTest,
    "MyProject2.World.RuntimeProfile.ConfigureAndLock",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldRuntimeProfileSubsystemLockTest::RunTest(const FString& Parameters)
{
    UWorldRuntimeProfileDataAsset* Asset = NewObject<UWorldRuntimeProfileDataAsset>(GetTransientPackage());
    UWorldRuntimeProfileSubsystem* Runtime = NewObject<UWorldRuntimeProfileSubsystem>(GetTransientPackage());

    if (!TestNotNull(TEXT("Runtime profile asset should be created"), Asset)
        || !TestNotNull(TEXT("Runtime profile subsystem object should be created"), Runtime))
    {
        return false;
    }

    FString Error;
    TestFalse(TEXT("Profile should begin unlocked"), Runtime->IsProfileLocked());
    TestTrue(
        TEXT("A valid LOW_MID profile should configure before the lock"),
        Runtime->ConfigureProfile(Asset, Error));
    TestEqual(
        TEXT("The selected profile should be LOW_MID"),
        static_cast<uint8>(Runtime->GetActiveProfile()),
        static_cast<uint8>(EWorldRuntimeProfile::LowMid));
    TestTrue(TEXT("Successful configuration should clear the error"), Error.IsEmpty());

    Asset->ActiveProfile = EWorldRuntimeProfile::HighEnd;
    TestTrue(
        TEXT("Changing to HIGH_END should be allowed before locking"),
        Runtime->ConfigureProfile(Asset, Error));
    TestEqual(
        TEXT("The selected profile should now be HIGH_END"),
        static_cast<uint8>(Runtime->GetActiveProfile()),
        static_cast<uint8>(EWorldRuntimeProfile::HighEnd));

    Runtime->LockProfileForSession();
    TestTrue(TEXT("LockProfileForSession should lock the profile"), Runtime->IsProfileLocked());

    Asset->ActiveProfile = EWorldRuntimeProfile::LowMid;
    TestFalse(
        TEXT("Profile reconfiguration should fail after the session lock"),
        Runtime->ConfigureProfile(Asset, Error));
    TestFalse(TEXT("A rejected post-lock configuration should explain why"), Error.IsEmpty());
    TestEqual(
        TEXT("Rejected reconfiguration must not change the active profile"),
        static_cast<uint8>(Runtime->GetActiveProfile()),
        static_cast<uint8>(EWorldRuntimeProfile::HighEnd));

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

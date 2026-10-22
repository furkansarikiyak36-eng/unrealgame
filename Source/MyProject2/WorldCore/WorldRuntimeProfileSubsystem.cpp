// Copyright Epic Games, Inc. All Rights Reserved.

#include "WorldRuntimeProfileSubsystem.h"

void UWorldRuntimeProfileSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Safe fallback: LOW_MID is always the default if no DataAsset is configured.
    ActiveProfile = EWorldRuntimeProfile::LowMid;
    RuntimeSettings = FWorldRuntimeProfileValues{};
    bProfileLocked = false;
    ProfileAsset = nullptr;
}

void UWorldRuntimeProfileSubsystem::Deinitialize()
{
    ProfileAsset = nullptr;
    bProfileLocked = false;
    RuntimeSettings = FWorldRuntimeProfileValues{};
    ActiveProfile = EWorldRuntimeProfile::LowMid;

    Super::Deinitialize();
}

bool UWorldRuntimeProfileSubsystem::ConfigureProfile(
    UWorldRuntimeProfileDataAsset* InProfileAsset,
    FString& OutError)
{
    if (bProfileLocked)
    {
        OutError = TEXT("The runtime profile is locked for this world session.");
        return false;
    }

    if (!IsValid(InProfileAsset))
    {
        OutError = TEXT("A valid WorldRuntimeProfileDataAsset is required.");
        return false;
    }

    if (!InProfileAsset->ValidateActiveSettings(OutError))
    {
        return false;
    }

    const FWorldRuntimeProfileValues& CandidateSettings = InProfileAsset->GetActiveSettings();

    RuntimeSettings = CandidateSettings;
    ActiveProfile = InProfileAsset->ActiveProfile;
    ProfileAsset = InProfileAsset;
    OutError.Reset();
    return true;
}

void UWorldRuntimeProfileSubsystem::LockProfileForSession()
{
    bProfileLocked = true;
}

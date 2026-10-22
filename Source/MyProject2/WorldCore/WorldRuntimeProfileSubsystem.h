// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldRuntimeProfile.h"
#include "WorldRuntimeProfileSubsystem.generated.h"

/**
 * Session-level owner of the selected performance profile.
 * A profile must be locked before the voxel runtime begins editing world data.
 * A later phase will call LockProfileForSession from the voxel runtime bootstrap.
 */
UCLASS()
class MYPROJECT2_API UWorldRuntimeProfileSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Configure from a profile asset before the voxel runtime locks its grid. */
    UFUNCTION(BlueprintCallable, Category = "World|Performance Profile")
    bool ConfigureProfile(UWorldRuntimeProfileDataAsset* InProfileAsset, FString& OutError);

    /** Prevent grid-affecting profile changes after runtime initialization/editing. */
    UFUNCTION(BlueprintCallable, Category = "World|Performance Profile")
    void LockProfileForSession();

    UFUNCTION(BlueprintPure, Category = "World|Performance Profile")
    EWorldRuntimeProfile GetActiveProfile() const { return ActiveProfile; }

    UFUNCTION(BlueprintPure, Category = "World|Performance Profile")
    FWorldRuntimeProfileValues GetActiveSettings() const { return RuntimeSettings; }

    UFUNCTION(BlueprintPure, Category = "World|Performance Profile")
    bool IsProfileLocked() const { return bProfileLocked; }

    UFUNCTION(BlueprintPure, Category = "World|Performance Profile")
    bool IsUsingConfiguredAsset() const { return IsValid(ProfileAsset); }

private:
    UPROPERTY(Transient)
    TObjectPtr<UWorldRuntimeProfileDataAsset> ProfileAsset = nullptr;

    UPROPERTY(Transient)
    EWorldRuntimeProfile ActiveProfile = EWorldRuntimeProfile::LowMid;

    FWorldRuntimeProfileValues RuntimeSettings;
    bool bProfileLocked = false;
};

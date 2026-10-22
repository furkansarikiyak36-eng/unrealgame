// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WorldRuntimeProfile.generated.h"

/**
 * Shared runtime quality policy. Gameplay rules and serialized world state must
 * not depend on this enum; profiles may change detail and scheduling only.
 */
UENUM(BlueprintType)
enum class EWorldRuntimeProfile : uint8
{
    LowMid UMETA(DisplayName = "Low–Mid"),
    HighEnd UMETA(DisplayName = "High-End")
};

/**
 * Tunable performance settings for the smooth-voxel runtime.
 * These initial values are design starting points, not benchmarked guarantees.
 */
USTRUCT(BlueprintType)
struct MYPROJECT2_API FWorldRuntimeProfileValues
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel", meta = (ClampMin = "1.0"))
    float VoxelSampleSpacingCm = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel", meta = (ClampMin = "100.0"))
    float ChunkSizeCm = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel", meta = (ClampMin = "4", ClampMax = "128"))
    int32 CellsPerChunkAxis = 32;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scheduling", meta = (ClampMin = "1", ClampMax = "64"))
    int32 MaxMeshJobsInFlight = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scheduling", meta = (ClampMin = "1", ClampMax = "4096"))
    int32 MaxQueuedMeshJobs = 8;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scheduling", meta = (ClampMin = "1", ClampMax = "64"))
    int32 MaxChunksPublishedPerFrame = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scheduling", meta = (ClampMin = "1", ClampMax = "64"))
    int32 MaxCollisionUpdatesPerFrame = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scan", meta = (ClampMin = "1", ClampMax = "32"))
    int32 MaxSurfaceSamplesPerAxis = 5;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel", meta = (ClampMin = "1", ClampMax = "100000"))
    int32 MaxActiveEditableChunks = 32;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel", meta = (ClampMin = "1", ClampMax = "50000000"))
    int32 MaxDensitySamplesPerEdit = 100000;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rendering")
    bool bUseAdaptiveVoxelLOD = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scheduling")
    bool bAllowBackgroundMeshing = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Physics")
    bool bAllowAsyncCollision = false;

    /** Checks profile values before they are used to allocate or schedule work. */
    bool Validate(FString* OutError = nullptr) const;

    /** Derives the cells-per-axis value from spacing and chunk size if they align. */
    bool HasConsistentChunkGrid(float ToleranceCm = 1.0f) const;
};

/**
 * Selectable settings asset for the one shared codebase.
 * LOW_MID is the first implementation target; HIGH_END is enabled only after
 * equivalent functional tests pass on the LOW_MID profile.
 */
UCLASS(BlueprintType)
class MYPROJECT2_API UWorldRuntimeProfileDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UWorldRuntimeProfileDataAsset();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Profile")
    EWorldRuntimeProfile ActiveProfile = EWorldRuntimeProfile::LowMid;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Profile|Low-Mid")
    FWorldRuntimeProfileValues LowMid;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Profile|High-End")
    FWorldRuntimeProfileValues HighEnd;

    const FWorldRuntimeProfileValues& GetActiveSettings() const
    {
        return ActiveProfile == EWorldRuntimeProfile::HighEnd ? HighEnd : LowMid;
    }

    UFUNCTION(BlueprintPure, Category = "Profile")
    FWorldRuntimeProfileValues GetActiveSettingsCopy() const
    {
        return GetActiveSettings();
    }

    UFUNCTION(BlueprintCallable, Category = "Profile")
    bool ValidateActiveSettings(FString& OutError) const
    {
        switch (ActiveProfile)
        {
        case EWorldRuntimeProfile::LowMid:
        case EWorldRuntimeProfile::HighEnd:
            break;
        default:
            OutError = TEXT("ActiveProfile contains an unsupported enum value.");
            return false;
        }

        return GetActiveSettings().Validate(&OutError);
    }

    UFUNCTION(BlueprintCallable, Category = "Profile")
    void SetActiveProfile(EWorldRuntimeProfile NewProfile)
    {
        ActiveProfile = NewProfile;
    }
};

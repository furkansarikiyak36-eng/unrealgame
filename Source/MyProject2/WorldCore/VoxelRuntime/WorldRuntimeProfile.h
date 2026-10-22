#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "WorldRuntimeProfile.generated.h"

UENUM(BlueprintType)
enum class EWorldRuntimeProfile : uint8
{
    LowMid UMETA(DisplayName = "Low–Mid"),
    HighEnd UMETA(DisplayName = "High-End")
};

/**
 * Shared runtime tuning for terrain, Scan and placement. Profile changes quality,
 * never gameplay rules or the logical save format.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "World Runtime Profile"))
class MYPROJECT2_API UWorldRuntimeProfileSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    static const UWorldRuntimeProfileSettings& Get() { return *GetDefault<UWorldRuntimeProfileSettings>(); }

    UPROPERTY(Config, EditAnywhere, Category = "Profile")
    EWorldRuntimeProfile ActiveProfile = EWorldRuntimeProfile::LowMid;

    UPROPERTY(Config, EditAnywhere, Category = "Low–Mid", meta = (ClampMin = "1.0"))
    float LowMidCellSizeCm = 25.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Low–Mid", meta = (ClampMin = "4"))
    int32 LowMidCellsPerChunkAxis = 32;

    UPROPERTY(Config, EditAnywhere, Category = "High-End", meta = (ClampMin = "1.0"))
    float HighEndCellSizeCm = 12.5f;

    UPROPERTY(Config, EditAnywhere, Category = "High-End", meta = (ClampMin = "4"))
    int32 HighEndCellsPerChunkAxis = 64;

    UPROPERTY(Config, EditAnywhere, Category = "Scheduling", meta = (ClampMin = "1", ClampMax = "8"))
    int32 LowMidMaxMeshJobsInFlight = 1;

    UPROPERTY(Config, EditAnywhere, Category = "Scheduling", meta = (ClampMin = "1", ClampMax = "16"))
    int32 HighEndMaxMeshJobsInFlight = 3;

    UPROPERTY(Config, EditAnywhere, Category = "Scheduling", meta = (ClampMin = "1", ClampMax = "4"))
    int32 MaxChunksSampledPerTick = 1;

    UPROPERTY(Config, EditAnywhere, Category = "Scheduling", meta = (ClampMin = "1", ClampMax = "4"))
    int32 MaxChunksPublishedPerTick = 1;

    UPROPERTY(Config, EditAnywhere, Category = "Terrain", meta = (ClampMin = "1"))
    int32 MaxCellsPerEdit = 250000;

    UPROPERTY(Config, EditAnywhere, Category = "Terrain", meta = (ClampMin = "1"))
    int32 MaxSnapshotOverrideCells = 2000000;

    UPROPERTY(Config, EditAnywhere, Category = "Terrain")
    bool bGenerateCollision = true;

    UPROPERTY(Config, EditAnywhere, Category = "Terrain")
    bool bUseAdaptiveVoxelLOD = false;

    UPROPERTY(Config, EditAnywhere, Category = "Terrain")
    bool bUseAsyncCollision = false;

    UFUNCTION(BlueprintPure, Category = "World|Profile")
    float GetCellSizeCm() const;

    UFUNCTION(BlueprintPure, Category = "World|Profile")
    int32 GetCellsPerChunkAxis() const;

    UFUNCTION(BlueprintPure, Category = "World|Profile")
    int32 GetMaxMeshJobsInFlight() const;

    UFUNCTION(BlueprintPure, Category = "World|Profile")
    float GetChunkSizeCm() const;

    /** Applies grid settings only before edits exist; never silently reinterprets saved cells. */
    bool ApplyTo(class UExcavationVolumeSubsystem* Volume) const;
};

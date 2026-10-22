#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldCore/ExcavationTypes.h"
#include "WorldCore/WorldRuntimeProfile.h"
#include "ExcavationLandscapeDensitySource.h"
#include "ExcavationTerrainChunkSubsystem.generated.h"

class AExcavationTerrainChunkActor;
class UExcavationFlatBaseDensitySource;
class UExcavationVolumeSubsystem;
class UWorldRuntimeProfileSubsystem;

/**
 * Routes excavation edits to derived mesh adapters and optionally streams those
 * adapters around the player. It owns neither density values nor simulation cells.
 */
UCLASS(Config = Game)
class MYPROJECT2_API UExcavationTerrainChunkSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

    void RegisterChunkActor(AExcavationTerrainChunkActor* Actor);
    void UnregisterChunkActor(AExcavationTerrainChunkActor* Actor);

    /** Coalesces rebuild requests; mesh publication and collision updates are drained under profile budgets. */
    void QueueChunkRebuild(const FIntVector& Coordinate);

    bool IsRuntimeReady() const { return bRuntimeReady; }
    FString GetRuntimeSetupError() const { return RuntimeSetupError; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation|Materials")
    TArray<FExcavationLandscapeMaterialMapping> LandscapeMaterialMappings;

    /** Optional Blueprint subclass can provide the project's default terrain and material profile. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation|Renderer")
    TSubclassOf<AExcavationTerrainChunkActor> TerrainChunkActorClass;

    UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Excavation|Streaming")
    bool bEnableAutomaticStreaming = true;

    UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Excavation|Streaming", meta = (ClampMin = "0", ClampMax = "8"))
    int32 HorizontalChunkRadius = 1;

    UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Excavation|Streaming", meta = (ClampMin = "1", ClampMax = "32"))
    int32 UndergroundChunkLayers = 4;

    UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Excavation|Streaming", meta = (ClampMin = "0", ClampMax = "8"))
    int32 AbovePlayerChunkLayers = 0;

    UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Excavation|Streaming", meta = (ClampMin = "1", ClampMax = "16"))
    int32 MaxChunksSpawnedPerTick = 1;

    UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Excavation|Streaming", meta = (ClampMin = "0.01", ClampMax = "5.0"))
    float StreamingIntervalSeconds = 0.05f;

    UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Excavation|Streaming", meta = (ClampMin = "16", ClampMax = "4096"))
    int32 MaxQueuedDirtyChunks = 256;

protected:
    UFUNCTION()
    void HandleVolumeChanged(const FExcavationEditRecord& Record, bool bIsRollback);

    UFUNCTION()
    void HandleVolumeReset();

    void UpdateStreamedChunks();
    void ProcessDirtyChunkQueue();
    void RefreshLandscapeFallbackProvider();
    bool HasLiveRegisteredChunk(const FIntVector& Coordinate);
    void RemoveInvalidRegisteredActors();

    UPROPERTY(Transient)
    TObjectPtr<UExcavationFlatBaseDensitySource> FlatFallbackDensitySource;

    UPROPERTY(Transient)
    TObjectPtr<UExcavationLandscapeDensitySource> LandscapeFallbackDensitySource;

    UPROPERTY(Transient)
    TObjectPtr<UObject> RegisteredFallbackDensitySource;

    TMap<FIntVector, TArray<TWeakObjectPtr<AExcavationTerrainChunkActor>>> RegisteredChunks;
    TMap<FIntVector, TWeakObjectPtr<AExcavationTerrainChunkActor>> ManagedChunkActors;
    TSet<FIntVector> DirtyChunkQueue;
    FWorldRuntimeProfileValues RuntimeSettings;
    FString RuntimeSetupError;
    bool bRuntimeReady = false;
    bool bFullRefreshPending = false;
    float StreamingAccumulatorSeconds = 0.0f;
    float LandscapeDiscoveryAccumulatorSeconds = 0.0f;
    float LandscapeDiscoveryIntervalSeconds = 1.0f;
    bool bDidInitialLandscapeDiscovery = false;
};

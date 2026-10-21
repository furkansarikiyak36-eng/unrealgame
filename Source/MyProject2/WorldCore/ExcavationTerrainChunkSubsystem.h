#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldCore/ExcavationTypes.h"
#include "ExcavationLandscapeDensitySource.h"
#include "ExcavationTerrainChunkSubsystem.generated.h"

class AExcavationTerrainChunkActor;
class UExcavationFlatBaseDensitySource;
class UExcavationVolumeSubsystem;

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
    /** Builds the ground round the player start before the game mode places anything on it. */
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

    void RegisterChunkActor(AExcavationTerrainChunkActor* Actor);
    void UnregisterChunkActor(AExcavationTerrainChunkActor* Actor);

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

    /**
     * When false (default) ground chunks stay once built: whatever lies on them (logs, stones, buildings) must not
     * fall through when the player walks away. True frees far chunks (large maps; needs things there frozen).
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Excavation|Streaming")
    bool bUnloadDistantChunks = false;

    /** Builds the ground under Location now if it is not there yet (things placed on the ground at world start). */
    UFUNCTION(BlueprintCallable, Category = "Excavation|Streaming")
    void EnsureGroundAt(const FVector& Location);

    /**
     * Edits queue their chunks; each frame rebuilds queued chunks until this many ms are spent (at least one).
     * Rapid digging then rebuilds a chunk once instead of once per hit, and a hit never stalls a frame for long.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Excavation|Renderer", meta = (ClampMin = "0.5", ClampMax = "50.0"))
    float RebuildBudgetMs = 4.0f;

    UFUNCTION(BlueprintPure, Category = "Excavation|Renderer")
    int32 GetPendingRebuildCount() const { return PendingRebuilds.Num(); }

    /** Rebuilds every queued chunk now (tests, save/load). */
    UFUNCTION(BlueprintCallable, Category = "Excavation|Renderer")
    int32 FlushPendingRebuilds();

protected:
    UFUNCTION()
    void HandleVolumeChanged(const FExcavationEditRecord& Record, bool bIsRollback);

    UFUNCTION()
    void HandleVolumeReset();

    void UpdateStreamedChunks();
    /** Streams chunks round Center; bBuildNow builds each new chunk's mesh and collision at once (world start). */
    void UpdateStreamedChunksAround(const FVector& Center, bool bBuildNow);
    AExcavationTerrainChunkActor* SpawnManagedChunk(const FIntVector& Coordinate, bool bBuildNow);
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
    void QueueRebuild(AExcavationTerrainChunkActor* Actor);
    void RemoveGroundCoverAround(const FExcavationEditRecord& Record);
    void ProcessPendingRebuilds(double BudgetMs);
    TArray<TWeakObjectPtr<AExcavationTerrainChunkActor>> PendingRebuilds;
    float StreamingAccumulatorSeconds = 0.0f;
    float LandscapeDiscoveryAccumulatorSeconds = 0.0f;
    float LandscapeDiscoveryIntervalSeconds = 1.0f;
    bool bDidInitialLandscapeDiscovery = false;
};

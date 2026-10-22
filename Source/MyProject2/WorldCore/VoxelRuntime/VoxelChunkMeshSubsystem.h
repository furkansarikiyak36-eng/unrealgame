#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ExcavationTypes.h"
#include "VoxelChunkMeshSubsystem.generated.h"

class AProceduralVoxelChunkActor;
class UExcavationVolumeSubsystem;

UCLASS()
class MYPROJECT2_API UVoxelChunkMeshSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

    UFUNCTION(BlueprintCallable, Category = "Voxel|Mesh")
    void RequestRebuildRegion(FVector CenterCm, FVector ExtentCm);

    UFUNCTION(BlueprintCallable, Category = "Voxel|Mesh")
    void RequestRebuildAllEditedChunks();

    UFUNCTION(BlueprintPure, Category = "Voxel|Mesh")
    int32 GetQueuedChunkCount() const { return DirtyChunks.Num(); }

    UFUNCTION(BlueprintPure, Category = "Voxel|Mesh")
    int32 GetBuildsInFlight() const { return InFlightChunks.Num(); }

protected:
    UFUNCTION()
    void HandleVolumeChanged(const FExcavationEditRecord& Record, bool bIsRollback);

    UFUNCTION()
    void HandleVolumeReset();

private:
    struct FCompletedMesh
    {
        FIntVector Coordinate = FIntVector::ZeroValue;
        FVector WorldOrigin = FVector::ZeroVector;
        TArray<FVector> Vertices;
        TArray<int32> Triangles;
        TArray<FVector> Normals;
    };

    struct FActiveSampleJob
    {
        FIntVector Coordinate = FIntVector::ZeroValue;
        FVector WorldOrigin = FVector::ZeroVector;
        float SampleSpacingCm = 25.0f;
        int32 CellsPerAxis = 32;
        TArray<float> Samples;
        int32 NextSampleIndex = 0;
    };

    TSet<FIntVector> DirtyChunks;
    TSet<FIntVector> InFlightChunks;
    TArray<FActiveSampleJob> ActiveSampleJobs;
    UPROPERTY(Transient)
    TMap<FIntVector, TObjectPtr<AProceduralVoxelChunkActor>> ChunkActors;
    TArray<FCompletedMesh> PendingPublishes;

    void QueueChunk(FIntVector Coordinate);
    void StartOneBuild(FIntVector Coordinate);
    void ProcessDensitySamples(int32 MaxSamplesThisTick);
    void StartMeshingFromSamples(FIntVector Coordinate, FVector WorldOrigin, float SampleSpacingCm,
        int32 CellsPerAxis, TArray<float>&& Samples);
    void PublishCompleted(FCompletedMesh&& Mesh);
    AProceduralVoxelChunkActor* FindOrCreateChunk(FIntVector Coordinate, FVector WorldOrigin);
};

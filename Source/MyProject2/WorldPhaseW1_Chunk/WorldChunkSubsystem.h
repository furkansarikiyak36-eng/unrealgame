#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ChunkCoordinate.h"
#include "WorldChunkSubsystem.generated.h"
UCLASS()
class MYPROJECT2_API UWorldChunkSubsystem : public UTickableWorldSubsystem
{
 GENERATED_BODY()
public:
 virtual void Initialize(FSubsystemCollectionBase& Collection) override;
 virtual void Tick(float DeltaTime) override;
 virtual TStatId GetStatId() const override;
 UFUNCTION(BlueprintCallable,Category="Chunk") bool LoadChunk(const FIntPoint& Coord);
 UFUNCTION(BlueprintCallable,Category="Chunk") bool UnloadChunk(const FIntPoint& Coord);
 UFUNCTION(BlueprintCallable,Category="Chunk") void ActivateChunksInRadius(const FVector& Center,float RadiusMeters);
 UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) TSet<FIntPoint> LoadedChunks;
};

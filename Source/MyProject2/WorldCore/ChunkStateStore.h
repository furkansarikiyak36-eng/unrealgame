#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ChunkState.h"
#include "ChunkStateStore.generated.h"
UCLASS()
class MYPROJECT2_API UChunkStateStore : public UObject
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Store") UChunkState* GetOrCreateChunk(const FIntPoint& Coord, int32 CellsPerAxis);
    UFUNCTION(BlueprintPure, Category="Store") UChunkState* FindChunk(const FIntPoint& Coord) const;
    UFUNCTION(BlueprintCallable, Category="Store") bool RemoveChunk(const FIntPoint& Coord);
    UFUNCTION(BlueprintCallable, Category="Store") TArray<UChunkState*> GetAllChunks() const;
    UFUNCTION(BlueprintPure, Category="Store") int32 GetChunkCount() const { return Chunks.Num(); }
    UFUNCTION(BlueprintCallable, Category="Store") void ClearAll() { Chunks.Empty(); }
    UFUNCTION(BlueprintCallable, Category="Store") void AddOrReplaceChunk(UChunkState* Chunk);
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Config") int32 CellsPerChunkAxis = 16;
protected:
    UPROPERTY() TMap<FIntPoint, TObjectPtr<UChunkState>> Chunks;
};

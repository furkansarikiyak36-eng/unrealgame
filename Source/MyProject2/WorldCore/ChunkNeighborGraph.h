#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ChunkNeighborGraph.generated.h"
UCLASS()
class MYPROJECT2_API UChunkNeighborGraph : public UObject
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Neighbor") static TArray<FIntPoint> GetNeighbors4(const FIntPoint& Chunk);
    UFUNCTION(BlueprintCallable, Category="Neighbor") static TArray<FIntPoint> GetNeighbors8(const FIntPoint& Chunk);
    UFUNCTION(BlueprintCallable, Category="Neighbor") static TArray<FIntPoint> GetChunksInRadius(const FIntPoint& Center, int32 Radius);
    UFUNCTION(BlueprintPure, Category="Neighbor") static bool AreAdjacent(const FIntPoint& A, const FIntPoint& B);
};

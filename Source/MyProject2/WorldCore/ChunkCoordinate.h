#pragma once
#include "CoreMinimal.h"
#include "ChunkCoordinate.generated.h"

USTRUCT(BlueprintType)
struct FChunkCoordinate
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 X = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Y = 0;
    FChunkCoordinate() = default;
    FChunkCoordinate(int32 InX, int32 InY) : X(InX), Y(InY) {}
    bool operator==(const FChunkCoordinate& O) const { return X == O.X && Y == O.Y; }
    friend uint32 GetTypeHash(const FChunkCoordinate& C) { return HashCombine(::GetTypeHash(C.X), ::GetTypeHash(C.Y)); }
};
USTRUCT(BlueprintType)
struct FWorldChunkSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World") float ChunkSizeMeters = 64.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World") int32 CellsPerChunkAxis = 16;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World") int32 WorldChunksX = 99;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World") int32 WorldChunksY = 99;
    float GetCellSizeMeters() const;
    int32 GetTotalChunks() const;
    FIntPoint WorldToChunk(const FVector& WorldPos) const;
    FVector ChunkToWorldCenter(const FIntPoint& Chunk) const;
    FIntPoint WorldToCell(const FVector& WorldPos) const;
    FIntPoint WorldToLocalCell(const FVector& WorldPos) const;
};

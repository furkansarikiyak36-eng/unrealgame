#include "ChunkStateStore.h"
UChunkState* UChunkStateStore::GetOrCreateChunk(const FIntPoint& Coord, int32 N)
{
    if (UChunkState* Existing = FindChunk(Coord)) return Existing;
    N = FMath::Clamp(N, 1, 256);
    UChunkState* Chunk = NewObject<UChunkState>(this); Chunk->Coordinate = Coord; Chunk->Initialize(N);
    CellsPerChunkAxis = N; Chunks.Add(Coord, Chunk); return Chunk;
}
UChunkState* UChunkStateStore::FindChunk(const FIntPoint& Coord) const { const TObjectPtr<UChunkState>* P = Chunks.Find(Coord); return P ? P->Get() : nullptr; }
bool UChunkStateStore::RemoveChunk(const FIntPoint& Coord) { return Chunks.Remove(Coord) > 0; }
TArray<UChunkState*> UChunkStateStore::GetAllChunks() const { TArray<UChunkState*> R; R.Reserve(Chunks.Num()); for (const auto& P : Chunks) if (P.Value) R.Add(P.Value.Get()); return R; }
void UChunkStateStore::AddOrReplaceChunk(UChunkState* Chunk) { if (Chunk) { Chunks.Add(Chunk->Coordinate, Chunk); CellsPerChunkAxis = FMath::Max(1, FMath::RoundToInt(FMath::Sqrt((float)Chunk->Cells.Num()))); } }

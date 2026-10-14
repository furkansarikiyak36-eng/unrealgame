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


bool UChunkStateStore::ReplaceAllChunks(const TArray<FChunkStateSnapshot>& SnapshotChunks, int32 CellsPerAxis)
{
    if (CellsPerAxis < 1 || CellsPerAxis > 256 || SnapshotChunks.Num() > 16384)
    {
        return false;
    }

    const int64 ExpectedCells = static_cast<int64>(CellsPerAxis) * CellsPerAxis;
    int64 TotalCells = 0;
    TMap<FIntPoint, TObjectPtr<UChunkState>> NewChunks;
    NewChunks.Reserve(SnapshotChunks.Num());

    for (const FChunkStateSnapshot& Snapshot : SnapshotChunks)
    {
        if (Snapshot.Cells.Num() != ExpectedCells || NewChunks.Contains(Snapshot.Coordinate))
        {
            return false;
        }
        TotalCells += Snapshot.Cells.Num();
        if (TotalCells > 4194304)
        {
            return false;
        }

        UChunkState* NewChunk = NewObject<UChunkState>(this);
        if (!NewChunk)
        {
            return false;
        }
        NewChunk->Coordinate = Snapshot.Coordinate;
        NewChunk->Cells = Snapshot.Cells;
        NewChunk->Metadata = Snapshot.Metadata;
        NewChunks.Add(Snapshot.Coordinate, NewChunk);
    }

    // Commit only after every replacement chunk has been constructed.
    Chunks = MoveTemp(NewChunks);
    CellsPerChunkAxis = CellsPerAxis;
    return true;
}

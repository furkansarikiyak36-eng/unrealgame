#include "ChunkState.h"
int32 UChunkState::GetCellIndex(int32 X, int32 Y, int32 N) const
{
    if (N <= 0 || X < 0 || Y < 0 || X >= N || Y >= N) return INDEX_NONE;
    return Y * N + X;
}
FChunkCell* UChunkState::GetCell(int32 X, int32 Y, int32 N) { const int32 I = GetCellIndex(X,Y,N); return Cells.IsValidIndex(I) ? &Cells[I] : nullptr; }
const FChunkCell* UChunkState::GetCellConst(int32 X, int32 Y, int32 N) const { const int32 I = GetCellIndex(X,Y,N); return Cells.IsValidIndex(I) ? &Cells[I] : nullptr; }
bool UChunkState::ReadCell(int32 X, int32 Y, int32 N, FChunkCell& Out) const { const FChunkCell* C = GetCellConst(X,Y,N); if (!C) return false; Out = *C; return true; }
bool UChunkState::WriteCell(int32 X, int32 Y, int32 N, const FChunkCell& In) { FChunkCell* C = GetCell(X,Y,N); if (!C) return false; *C = In; C->bIsDirty = true; MarkDirty(); return true; }
void UChunkState::Initialize(int32 N) { if (N <= 0 || N > 256) { Cells.Reset(); Metadata.bIsInitialized = false; return; } Cells.SetNum(N*N); Metadata.bIsInitialized = true; }

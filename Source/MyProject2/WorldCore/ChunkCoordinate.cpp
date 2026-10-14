#include "ChunkCoordinate.h"
float FWorldChunkSettings::GetCellSizeMeters() const { return ChunkSizeMeters / (float)FMath::Max(1, CellsPerChunkAxis); }
int32 FWorldChunkSettings::GetTotalChunks() const { return FMath::Max(0, WorldChunksX) * FMath::Max(0, WorldChunksY); }
FIntPoint FWorldChunkSettings::WorldToChunk(const FVector& P) const
{
    const float S = FMath::Max(1.0f, ChunkSizeMeters * 100.0f);
    return FIntPoint(FMath::FloorToInt(P.X / S), FMath::FloorToInt(P.Y / S));
}
FVector FWorldChunkSettings::ChunkToWorldCenter(const FIntPoint& C) const
{
    const float S = FMath::Max(1.0f, ChunkSizeMeters * 100.0f);
    return FVector((C.X + 0.5f) * S, (C.Y + 0.5f) * S, 0.0f);
}
FIntPoint FWorldChunkSettings::WorldToCell(const FVector& P) const
{
    const float S = FMath::Max(1.0f, GetCellSizeMeters() * 100.0f);
    return FIntPoint(FMath::FloorToInt(P.X / S), FMath::FloorToInt(P.Y / S));
}
FIntPoint FWorldChunkSettings::WorldToLocalCell(const FVector& P) const
{
    const float ChunkCm = FMath::Max(1.0f, ChunkSizeMeters * 100.0f);
    const float CellCm = FMath::Max(1.0f, GetCellSizeMeters() * 100.0f);
    const float LX = P.X - FMath::FloorToFloat(P.X / ChunkCm) * ChunkCm;
    const float LY = P.Y - FMath::FloorToFloat(P.Y / ChunkCm) * ChunkCm;
    const int32 MaxLocal = FMath::Max(0, CellsPerChunkAxis - 1);
    return FIntPoint(FMath::Clamp(FMath::FloorToInt(LX / CellCm), 0, MaxLocal), FMath::Clamp(FMath::FloorToInt(LY / CellCm), 0, MaxLocal));
}

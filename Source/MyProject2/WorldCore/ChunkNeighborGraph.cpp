#include "ChunkNeighborGraph.h"
TArray<FIntPoint> UChunkNeighborGraph::GetNeighbors4(const FIntPoint& C) { return { {C.X+1,C.Y}, {C.X-1,C.Y}, {C.X,C.Y+1}, {C.X,C.Y-1} }; }
TArray<FIntPoint> UChunkNeighborGraph::GetNeighbors8(const FIntPoint& C) { TArray<FIntPoint> R; for(int32 X=-1;X<=1;++X) for(int32 Y=-1;Y<=1;++Y) if(X||Y) R.Add(FIntPoint(C.X+X,C.Y+Y)); return R; }
TArray<FIntPoint> UChunkNeighborGraph::GetChunksInRadius(const FIntPoint& C, int32 Radius) { TArray<FIntPoint> R; const int32 Safe=FMath::Clamp(Radius,0,1024); for(int32 X=-Safe;X<=Safe;++X) for(int32 Y=-Safe;Y<=Safe;++Y) R.Add(FIntPoint(C.X+X,C.Y+Y)); return R; }
bool UChunkNeighborGraph::AreAdjacent(const FIntPoint& A,const FIntPoint& B) { return FMath::Abs(A.X-B.X)+FMath::Abs(A.Y-B.Y)==1; }

#include "WorldChunkSubsystem.h"
#include "WorldCore/WorldStateSubsystem.h"
#include "WorldCore/ChunkStateStore.h"
#include "Engine/World.h"
void UWorldChunkSubsystem::Initialize(FSubsystemCollectionBase& C){Super::Initialize(C);}
TStatId UWorldChunkSubsystem::GetStatId() const{RETURN_QUICK_DECLARE_CYCLE_STAT(UWorldChunkSubsystem,STATGROUP_Tickables);}
void UWorldChunkSubsystem::Tick(float D){Super::Tick(D);}
bool UWorldChunkSubsystem::LoadChunk(const FIntPoint& C){UWorld* W=GetWorld();UWorldStateSubsystem* S=W?W->GetSubsystem<UWorldStateSubsystem>():nullptr;if(!S||!S->StateStore)return false;S->StateStore->GetOrCreateChunk(C,S->ChunkSettings.CellsPerChunkAxis);LoadedChunks.Add(C);return true;}
bool UWorldChunkSubsystem::UnloadChunk(const FIntPoint& C){if(UWorld* W=GetWorld())if(UWorldStateSubsystem* S=W->GetSubsystem<UWorldStateSubsystem>())if(UChunkState* Chunk=S->GetChunk(C))Chunk->Metadata.ActivationMode=EChunkActivationMode::Passive;return LoadedChunks.Remove(C)>0;}
void UWorldChunkSubsystem::ActivateChunksInRadius(const FVector& Center, float RadiusMeters)
{
    UWorld* World = GetWorld();
    UWorldStateSubsystem* State = World ? World->GetSubsystem<UWorldStateSubsystem>() : nullptr;
    if (!State || !State->StateStore) return;

    const FIntPoint CenterChunk = State->ChunkSettings.WorldToChunk(Center);
    const float ChunkSize = FMath::Max(1.0f, State->ChunkSettings.ChunkSizeMeters);
    const int32 Radius = FMath::Clamp(FMath::CeilToInt(FMath::Max(0.0f, RadiusMeters) / ChunkSize) + 1, 0, 1024);
    TSet<FIntPoint> Desired;
    for (int32 X = -Radius; X <= Radius; ++X)
    {
        for (int32 Y = -Radius; Y <= Radius; ++Y)
        {
            const FIntPoint Coord(CenterChunk.X + X, CenterChunk.Y + Y);
            const FVector ChunkCenter = State->ChunkSettings.ChunkToWorldCenter(Coord);
            const float Reach = FMath::Max(0.0f, RadiusMeters) + ChunkSize * 0.72f;
            if (FVector::Dist2D(Center, ChunkCenter) <= Reach * 100.0f) Desired.Add(Coord);
        }
    }

    for (const FIntPoint& Existing : LoadedChunks)
    {
        if (!Desired.Contains(Existing))
            if (UChunkState* Chunk = State->StateStore->FindChunk(Existing))
                Chunk->Metadata.ActivationMode = EChunkActivationMode::Passive;
    }
    LoadedChunks.Reset();
    for (const FIntPoint& Coord : Desired) LoadChunk(Coord);
}

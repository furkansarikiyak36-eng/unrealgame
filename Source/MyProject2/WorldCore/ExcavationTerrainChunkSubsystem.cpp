#include "ExcavationTerrainChunkSubsystem.h"

#include "ExcavationTerrainChunkActor.h"
#include "ExcavationFlatBaseDensitySource.h"
#include "WorldCore/ExcavationVolumeSubsystem.h"
#include "Engine/World.h"

void UExcavationTerrainChunkSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UExcavationVolumeSubsystem>();

    FlatFallbackDensitySource = NewObject<UExcavationFlatBaseDensitySource>(this);
    FlatFallbackDensitySource->SurfaceHeightCm = 0.0f;

    if (UExcavationVolumeSubsystem* Excavation = GetWorld()->GetSubsystem<UExcavationVolumeSubsystem>())
    {
        Excavation->RegisterFallbackBaseDensitySource(FlatFallbackDensitySource);
        Excavation->OnVolumeChanged.AddDynamic(
            this, &UExcavationTerrainChunkSubsystem::HandleVolumeChanged);
        Excavation->OnVolumeReset.AddDynamic(
            this, &UExcavationTerrainChunkSubsystem::HandleVolumeReset);
    }
}

void UExcavationTerrainChunkSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        if (UExcavationVolumeSubsystem* Excavation = World->GetSubsystem<UExcavationVolumeSubsystem>())
        {
            Excavation->OnVolumeChanged.RemoveDynamic(
                this, &UExcavationTerrainChunkSubsystem::HandleVolumeChanged);
            Excavation->OnVolumeReset.RemoveDynamic(
                this, &UExcavationTerrainChunkSubsystem::HandleVolumeReset);
            Excavation->UnregisterBaseDensitySource(FlatFallbackDensitySource);
        }
    }

    RegisteredChunks.Reset();
    FlatFallbackDensitySource = nullptr;
    Super::Deinitialize();
}

void UExcavationTerrainChunkSubsystem::RegisterChunkActor(AExcavationTerrainChunkActor* Actor)
{
    if (!IsValid(Actor))
    {
        return;
    }

    TArray<TWeakObjectPtr<AExcavationTerrainChunkActor>>& Bucket =
        RegisteredChunks.FindOrAdd(Actor->GetChunkCoordinate());

    for (const TWeakObjectPtr<AExcavationTerrainChunkActor>& Existing : Bucket)
    {
        if (Existing.Get() == Actor)
        {
            return;
        }
    }

    Bucket.Add(Actor);
}

void UExcavationTerrainChunkSubsystem::UnregisterChunkActor(AExcavationTerrainChunkActor* Actor)
{
    if (!Actor)
    {
        return;
    }

    for (auto It = RegisteredChunks.CreateIterator(); It; ++It)
    {
        TArray<TWeakObjectPtr<AExcavationTerrainChunkActor>>& Bucket = It.Value();
        Bucket.RemoveAll([Actor](const TWeakObjectPtr<AExcavationTerrainChunkActor>& Entry)
        {
            return !Entry.IsValid() || Entry.Get() == Actor;
        });
        if (Bucket.IsEmpty())
        {
            It.RemoveCurrent();
        }
    }
}

void UExcavationTerrainChunkSubsystem::HandleVolumeChanged(
    const FExcavationEditRecord& Record,
    bool bIsRollback)
{
    UWorld* World = GetWorld();
    UExcavationVolumeSubsystem* Excavation =
        World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!Excavation)
    {
        return;
    }

    // Deduplicate the affected chunk coordinates; each registered mesh rebuilds at most once per edit.
    const TArray<FIntVector> AffectedChunks = Excavation->GetAffectedChunksForEdit(Record);
    TSet<FIntVector> UniqueAffectedChunks;
    for (const FIntVector& Coordinate : AffectedChunks)
    {
        UniqueAffectedChunks.Add(Coordinate);
    }

    for (const FIntVector& Coordinate : UniqueAffectedChunks)
    {
        TArray<TWeakObjectPtr<AExcavationTerrainChunkActor>>* Bucket = RegisteredChunks.Find(Coordinate);
        if (!Bucket)
        {
            continue;
        }

        Bucket->RemoveAll([](const TWeakObjectPtr<AExcavationTerrainChunkActor>& Entry)
        {
            return !Entry.IsValid();
        });

        for (const TWeakObjectPtr<AExcavationTerrainChunkActor>& Entry : *Bucket)
        {
            if (AExcavationTerrainChunkActor* Actor = Entry.Get())
            {
                Actor->RebuildTerrainChunk();
            }
        }
    }
}

void UExcavationTerrainChunkSubsystem::HandleVolumeReset()
{
    for (auto It = RegisteredChunks.CreateIterator(); It; ++It)
    {
        TArray<TWeakObjectPtr<AExcavationTerrainChunkActor>>& Bucket = It.Value();
        Bucket.RemoveAll([](const TWeakObjectPtr<AExcavationTerrainChunkActor>& Entry)
        {
            return !Entry.IsValid();
        });

        for (const TWeakObjectPtr<AExcavationTerrainChunkActor>& Entry : Bucket)
        {
            if (AExcavationTerrainChunkActor* Actor = Entry.Get())
            {
                Actor->RebuildTerrainChunk();
            }
        }

        if (Bucket.IsEmpty())
        {
            It.RemoveCurrent();
        }
    }
}

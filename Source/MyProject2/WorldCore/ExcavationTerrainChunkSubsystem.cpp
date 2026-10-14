#include "ExcavationTerrainChunkSubsystem.h"

#include "ExcavationTerrainChunkActor.h"
#include "ExcavationFlatBaseDensitySource.h"
#include "ExcavationLandscapeDensitySource.h"
#include "WorldCore/ExcavationVolumeSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "LandscapeProxy.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void UExcavationTerrainChunkSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UExcavationVolumeSubsystem>();

    FlatFallbackDensitySource = NewObject<UExcavationFlatBaseDensitySource>(this);
    FlatFallbackDensitySource->SurfaceHeightCm = 0.0f;
    RegisteredFallbackDensitySource = FlatFallbackDensitySource;

    if (UWorld* World = GetWorld())
    {
        if (UExcavationVolumeSubsystem* Excavation = World->GetSubsystem<UExcavationVolumeSubsystem>())
        {
            Excavation->RegisterFallbackBaseDensitySource(FlatFallbackDensitySource);
            Excavation->OnVolumeChanged.AddDynamic(
                this, &UExcavationTerrainChunkSubsystem::HandleVolumeChanged);
            Excavation->OnVolumeReset.AddDynamic(
                this, &UExcavationTerrainChunkSubsystem::HandleVolumeReset);
        }
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
            Excavation->UnregisterFallbackBaseDensitySource(RegisteredFallbackDensitySource);
        }
    }

    TArray<AExcavationTerrainChunkActor*> ActorsToDestroy;
    for (const TPair<FIntVector, TWeakObjectPtr<AExcavationTerrainChunkActor>>& Pair : ManagedChunkActors)
    {
        if (AExcavationTerrainChunkActor* Actor = Pair.Value.Get())
        {
            ActorsToDestroy.Add(Actor);
        }
    }
    ManagedChunkActors.Reset();

    for (AExcavationTerrainChunkActor* Actor : ActorsToDestroy)
    {
        if (IsValid(Actor))
        {
            Actor->Destroy();
        }
    }

    RegisteredChunks.Reset();
    FlatFallbackDensitySource = nullptr;
    LandscapeFallbackDensitySource = nullptr;
    RegisteredFallbackDensitySource = nullptr;
    Super::Deinitialize();
}

TStatId UExcavationTerrainChunkSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UExcavationTerrainChunkSubsystem, STATGROUP_Tickables);
}

bool UExcavationTerrainChunkSubsystem::IsTickable() const
{
    return !IsTemplate();
}

void UExcavationTerrainChunkSubsystem::Tick(float DeltaTime)
{
    LandscapeDiscoveryAccumulatorSeconds += FMath::Max(0.0f, DeltaTime);
    if (!bDidInitialLandscapeDiscovery
        || LandscapeDiscoveryAccumulatorSeconds >= FMath::Max(0.1f, LandscapeDiscoveryIntervalSeconds))
    {
        LandscapeDiscoveryAccumulatorSeconds = 0.0f;
        bDidInitialLandscapeDiscovery = true;
        RefreshLandscapeFallbackProvider();
    }

    if (!bEnableAutomaticStreaming)
    {
        return;
    }

    StreamingAccumulatorSeconds += FMath::Max(0.0f, DeltaTime);
    if (StreamingAccumulatorSeconds < FMath::Max(0.01f, StreamingIntervalSeconds))
    {
        return;
    }

    StreamingAccumulatorSeconds = 0.0f;
    UpdateStreamedChunks();
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

    // ManagedChunkActors is reconciled by the streaming loop. Do not mutate it
    // from EndPlay, because EndPlay can be triggered while that loop destroys actors.
}

bool UExcavationTerrainChunkSubsystem::HasLiveRegisteredChunk(const FIntVector& Coordinate)
{
    TArray<TWeakObjectPtr<AExcavationTerrainChunkActor>>* Bucket = RegisteredChunks.Find(Coordinate);
    if (!Bucket)
    {
        return false;
    }

    Bucket->RemoveAll([](const TWeakObjectPtr<AExcavationTerrainChunkActor>& Entry)
    {
        return !Entry.IsValid();
    });

    return !Bucket->IsEmpty();
}

void UExcavationTerrainChunkSubsystem::RemoveInvalidRegisteredActors()
{
    for (auto It = RegisteredChunks.CreateIterator(); It; ++It)
    {
        It.Value().RemoveAll([](const TWeakObjectPtr<AExcavationTerrainChunkActor>& Entry)
        {
            return !Entry.IsValid();
        });
        if (It.Value().IsEmpty())
        {
            It.RemoveCurrent();
        }
    }

    for (auto It = ManagedChunkActors.CreateIterator(); It; ++It)
    {
        AExcavationTerrainChunkActor* Actor = It.Value().Get();
        if (!Actor || Actor->GetChunkCoordinate() != It.Key())
        {
            It.RemoveCurrent();
        }
    }
}

void UExcavationTerrainChunkSubsystem::RefreshLandscapeFallbackProvider()
{
    UWorld* World = GetWorld();
    UExcavationVolumeSubsystem* Excavation =
        World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!World || !Excavation)
    {
        return;
    }

    TArray<ALandscapeProxy*> Proxies;
    for (TActorIterator<ALandscapeProxy> It(World); It; ++It)
    {
        if (IsValid(*It))
        {
            Proxies.Add(*It);
        }
    }

    bool bProviderChanged = false;
    if (!Proxies.IsEmpty())
    {
        if (!IsValid(LandscapeFallbackDensitySource))
        {
            LandscapeFallbackDensitySource = NewObject<UExcavationLandscapeDensitySource>(this);
        }

        const bool bProxySetChanged = LandscapeFallbackDensitySource->SetLandscapeProxies(Proxies);
        if (RegisteredFallbackDensitySource != LandscapeFallbackDensitySource)
        {
            if (!Excavation->RegisterFallbackBaseDensitySource(LandscapeFallbackDensitySource))
            {
                return;
            }
            RegisteredFallbackDensitySource = LandscapeFallbackDensitySource;
            bProviderChanged = true;
        }

        bProviderChanged = bProviderChanged || bProxySetChanged;
    }
    else
    {
        if (IsValid(LandscapeFallbackDensitySource))
        {
            bProviderChanged = LandscapeFallbackDensitySource->SetLandscapeProxies(TArray<ALandscapeProxy*>());
        }

        if (RegisteredFallbackDensitySource != FlatFallbackDensitySource)
        {
            if (!Excavation->RegisterFallbackBaseDensitySource(FlatFallbackDensitySource))
            {
                return;
            }
            RegisteredFallbackDensitySource = FlatFallbackDensitySource;
            bProviderChanged = true;
        }
    }

    // A provider/proxy set change alters density samples even without a cell edit.
    if (bProviderChanged)
    {
        HandleVolumeReset();
    }
}

void UExcavationTerrainChunkSubsystem::UpdateStreamedChunks()
{
    UWorld* World = GetWorld();
    UExcavationVolumeSubsystem* Excavation =
        World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
    APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
    if (!World || !Excavation || !Pawn)
    {
        return;
    }

    FIntVector PlayerCell;
    FIntVector PlayerChunk;
    if (!Excavation->TryWorldToCell(Pawn->GetActorLocation(), PlayerCell)
        || !Excavation->TryGetChunkCoordinateForCell(PlayerCell, PlayerChunk))
    {
        return;
    }

    const int32 Radius = FMath::Clamp(HorizontalChunkRadius, 0, 8);
    const int32 Below = FMath::Clamp(UndergroundChunkLayers, 1, 32);
    const int32 Above = FMath::Clamp(AbovePlayerChunkLayers, 0, 8);
    TSet<FIntVector> DesiredChunks;

    for (int32 ZOffset = -Below; ZOffset <= Above; ++ZOffset)
    {
        const int64 Z = static_cast<int64>(PlayerChunk.Z) + ZOffset;
        if (Z < TNumericLimits<int32>::Lowest() || Z > TNumericLimits<int32>::Max())
        {
            continue;
        }

        for (int32 YOffset = -Radius; YOffset <= Radius; ++YOffset)
        {
            const int64 Y = static_cast<int64>(PlayerChunk.Y) + YOffset;
            if (Y < TNumericLimits<int32>::Lowest() || Y > TNumericLimits<int32>::Max())
            {
                continue;
            }

            for (int32 XOffset = -Radius; XOffset <= Radius; ++XOffset)
            {
                const int64 X = static_cast<int64>(PlayerChunk.X) + XOffset;
                if (X < TNumericLimits<int32>::Lowest() || X > TNumericLimits<int32>::Max())
                {
                    continue;
                }

                DesiredChunks.Add(FIntVector(
                    static_cast<int32>(X),
                    static_cast<int32>(Y),
                    static_cast<int32>(Z)));
            }
        }
    }

    // Destroy only actors spawned by this renderer; manually placed chunk actors remain user-owned.
    for (auto It = ManagedChunkActors.CreateIterator(); It; ++It)
    {
        AExcavationTerrainChunkActor* Actor = It.Value().Get();
        if (!Actor || !DesiredChunks.Contains(It.Key()))
        {
            It.RemoveCurrent();
            if (Actor)
            {
                Actor->Destroy();
            }
        }
    }

    RemoveInvalidRegisteredActors();

    TArray<FIntVector> OrderedDesiredChunks = DesiredChunks.Array();
    OrderedDesiredChunks.Sort([](const FIntVector& A, const FIntVector& B)
    {
        if (A.X != B.X) return A.X < B.X;
        if (A.Y != B.Y) return A.Y < B.Y;
        return A.Z < B.Z;
    });

    int32 SpawnedThisTick = 0;
    const int32 SpawnBudget = FMath::Clamp(MaxChunksSpawnedPerTick, 1, 16);
    for (const FIntVector& Coordinate : OrderedDesiredChunks)
    {
        if (SpawnedThisTick >= SpawnBudget)
        {
            break;
        }

        if (ManagedChunkActors.Contains(Coordinate) || HasLiveRegisteredChunk(Coordinate))
        {
            continue;
        }

        AExcavationTerrainChunkActor* Actor = World->SpawnActorDeferred<AExcavationTerrainChunkActor>(
            AExcavationTerrainChunkActor::StaticClass(),
            FTransform::Identity,
            nullptr,
            nullptr,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (!Actor)
        {
            continue;
        }

        // Assign the coordinate before BeginPlay so the actor never builds its default (0,0,0) chunk.
        Actor->ChunkCoordinate = Coordinate;
        Actor->FinishSpawning(FTransform::Identity);
        ManagedChunkActors.Add(Coordinate, Actor);
        ++SpawnedThisTick;
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

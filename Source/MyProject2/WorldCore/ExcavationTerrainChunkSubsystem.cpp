#include "ExcavationTerrainChunkSubsystem.h"

#include "ExcavationTerrainChunkActor.h"
#include "ExcavationFlatBaseDensitySource.h"
#include "ExcavationLandscapeDensitySource.h"
#include "WorldCore/ExcavationVolumeSubsystem.h"
#include "WorldCore/WorldRuntimeProfileSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "LandscapeProxy.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Async/Async.h"

void UExcavationTerrainChunkSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UWorldRuntimeProfileSubsystem>();
    Collection.InitializeDependency<UExcavationVolumeSubsystem>();

    bRuntimeReady = false;
    bFullRefreshPending = false;
    DirtyChunkQueue.Reset();
    InFlightMeshJobs.Reset();
    CompletedMeshResults.Reset();
    FullRefreshCoordinates.Reset();
    FullRefreshCursor = 0;
    bFullRefreshListReady = false;
    RuntimeSetupError.Reset();

    if (!TerrainChunkActorClass)
    {
        TerrainChunkActorClass = AExcavationTerrainChunkActor::StaticClass();
    }

    UWorld* World = GetWorld();
    UWorldRuntimeProfileSubsystem* Profile =
        World ? World->GetSubsystem<UWorldRuntimeProfileSubsystem>() : nullptr;
    UExcavationVolumeSubsystem* Excavation =
        World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;

    if (!Profile || !Excavation)
    {
        RuntimeSetupError = TEXT("Voxel chunk runtime could not resolve the world profile or authoritative excavation subsystem.");
        UE_LOG(LogTemp, Error, TEXT("%s"), *RuntimeSetupError);
        return;
    }

    RuntimeSettings = Profile->GetActiveSettings();
    if (!RuntimeSettings.Validate(&RuntimeSetupError)
        || !RuntimeSettings.HasConsistentChunkGrid())
    {
        if (RuntimeSetupError.IsEmpty())
        {
            RuntimeSetupError = TEXT("Selected world profile has inconsistent voxel spacing/chunk dimensions.");
        }
        UE_LOG(LogTemp, Error, TEXT("Voxel runtime disabled: %s"), *RuntimeSetupError);
        return;
    }

    if (!Excavation->ConfigureGridForRuntimeProfile(
        RuntimeSettings.VoxelSampleSpacingCm,
        RuntimeSettings.CellsPerChunkAxis,
        RuntimeSetupError))
    {
        UE_LOG(LogTemp, Error, TEXT("Voxel runtime disabled: %s"), *RuntimeSetupError);
        return;
    }

    Excavation->MaxCellsPerEdit = FMath::Clamp(
        RuntimeSettings.MaxDensitySamplesPerEdit, 1, 50000000);

    FlatFallbackDensitySource = NewObject<UExcavationFlatBaseDensitySource>(this);
    FlatFallbackDensitySource->SurfaceHeightCm = 0.0f;
    FlatFallbackDensitySource->DensityTransitionWidthCm = RuntimeSettings.VoxelSampleSpacingCm;
    RegisteredFallbackDensitySource = FlatFallbackDensitySource;

    Excavation->RegisterFallbackBaseDensitySource(FlatFallbackDensitySource);
    Excavation->OnVolumeChanged.AddDynamic(
        this, &UExcavationTerrainChunkSubsystem::HandleVolumeChanged);
    Excavation->OnVolumeReset.AddDynamic(
        this, &UExcavationTerrainChunkSubsystem::HandleVolumeReset);

    // The grid dimensions are immutable after this point; serialized cell coordinates
    // remain meaningful for the full world session.
    Profile->LockProfileForSession();
    bRuntimeReady = true;
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
    DirtyChunkQueue.Reset();
    InFlightMeshJobs.Reset();
    CompletedMeshResults.Reset();
    FullRefreshCoordinates.Reset();
    FullRefreshCursor = 0;
    bFullRefreshPending = false;
    bFullRefreshListReady = false;
    bRuntimeReady = false;
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
    if (!bRuntimeReady)
    {
        return;
    }

    LandscapeDiscoveryAccumulatorSeconds += FMath::Max(0.0f, DeltaTime);
    if (!bDidInitialLandscapeDiscovery
        || LandscapeDiscoveryAccumulatorSeconds >= FMath::Max(0.1f, LandscapeDiscoveryIntervalSeconds))
    {
        LandscapeDiscoveryAccumulatorSeconds = 0.0f;
        bDidInitialLandscapeDiscovery = true;
        RefreshLandscapeFallbackProvider();
    }

    ProcessDirtyChunkQueue();

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
    bool bProviderPointerChanged = false;
    if (!Proxies.IsEmpty())
    {
        if (!IsValid(LandscapeFallbackDensitySource))
        {
            LandscapeFallbackDensitySource = NewObject<UExcavationLandscapeDensitySource>(this);
            LandscapeFallbackDensitySource->SetDensityTransitionWidthCm(RuntimeSettings.VoxelSampleSpacingCm);
        }

        const bool bProxySetChanged = LandscapeFallbackDensitySource->SetLandscapeProxies(Proxies);
        const bool bMaterialMappingsChanged = LandscapeFallbackDensitySource->SetMaterialMappings(LandscapeMaterialMappings);
        if (RegisteredFallbackDensitySource != LandscapeFallbackDensitySource)
        {
            if (!Excavation->RegisterFallbackBaseDensitySource(LandscapeFallbackDensitySource))
            {
                return;
            }
            RegisteredFallbackDensitySource = LandscapeFallbackDensitySource;
            bProviderChanged = true;
            bProviderPointerChanged = true;
        }

        bProviderChanged = bProviderChanged || bProxySetChanged || bMaterialMappingsChanged;
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
            bProviderPointerChanged = true;
        }
    }

    // A provider/proxy set change alters density samples even without a cell edit.
    if (bProviderChanged && !bProviderPointerChanged)
    {
        // RegisterFallbackBaseDensitySource already advanced the revision if the pointer changed.
        Excavation->NotifyBaseDensityChanged();
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

    RemoveInvalidRegisteredActors();

    TArray<FIntVector> OrderedDesiredChunks = DesiredChunks.Array();
    OrderedDesiredChunks.Sort([&PlayerChunk](const FIntVector& A, const FIntVector& B)
    {
        const double ADX = static_cast<double>(A.X) - PlayerChunk.X;
        const double ADY = static_cast<double>(A.Y) - PlayerChunk.Y;
        const double ADZ = static_cast<double>(A.Z) - PlayerChunk.Z;
        const double BDX = static_cast<double>(B.X) - PlayerChunk.X;
        const double BDY = static_cast<double>(B.Y) - PlayerChunk.Y;
        const double BDZ = static_cast<double>(B.Z) - PlayerChunk.Z;
        const double DistanceA = (ADX * ADX) + (ADY * ADY) + (ADZ * ADZ);
        const double DistanceB = (BDX * BDX) + (BDY * BDY) + (BDZ * BDZ);
        if (!FMath::IsNearlyEqual(DistanceA, DistanceB))
        {
            return DistanceA < DistanceB;
        }
        if (A.X != B.X) return A.X < B.X;
        if (A.Y != B.Y) return A.Y < B.Y;
        return A.Z < B.Z;
    });

    const int32 MaxActive = FMath::Clamp(RuntimeSettings.MaxActiveEditableChunks, 1, 100000);
    if (OrderedDesiredChunks.Num() > MaxActive)
    {
        OrderedDesiredChunks.SetNum(MaxActive, EAllowShrinking::No);
    }
    DesiredChunks.Reset();
    for (const FIntVector& Coordinate : OrderedDesiredChunks)
    {
        DesiredChunks.Add(Coordinate);
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

    // Manually placed actors are user-owned and cannot be evicted by streaming. Reserve
    // their live slots before spawning managed chunks so the profile cap is respected.
    int32 RegisteredActorCount = 0;
    for (const TPair<FIntVector, TArray<TWeakObjectPtr<AExcavationTerrainChunkActor>>>& Pair : RegisteredChunks)
    {
        for (const TWeakObjectPtr<AExcavationTerrainChunkActor>& Entry : Pair.Value)
        {
            RegisteredActorCount += Entry.IsValid() ? 1 : 0;
        }
    }
    const int32 ManualActorCount = FMath::Max(0, RegisteredActorCount - ManagedChunkActors.Num());
    const int32 ManagedQuota = FMath::Max(0, MaxActive - ManualActorCount);

    int32 SpawnedThisTick = 0;
    const int32 SpawnBudget = FMath::Clamp(MaxChunksSpawnedPerTick, 1, 16);
    for (const FIntVector& Coordinate : OrderedDesiredChunks)
    {
        if (SpawnedThisTick >= SpawnBudget || ManagedChunkActors.Num() >= ManagedQuota)
        {
            break;
        }

        if (ManagedChunkActors.Contains(Coordinate) || HasLiveRegisteredChunk(Coordinate))
        {
            continue;
        }

        UClass* ChunkClass = TerrainChunkActorClass.Get();
        if (!ChunkClass)
        {
            ChunkClass = AExcavationTerrainChunkActor::StaticClass();
        }

        AExcavationTerrainChunkActor* Actor = World->SpawnActorDeferred<AExcavationTerrainChunkActor>(
            ChunkClass,
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
    for (const FIntVector& Coordinate : AffectedChunks)
    {
        QueueChunkRebuild(Coordinate);
    }
}

void UExcavationTerrainChunkSubsystem::HandleVolumeReset()
{
    DirtyChunkQueue.Reset();
    FullRefreshCoordinates.Reset();
    FullRefreshCursor = 0;
    bFullRefreshListReady = false;
    bFullRefreshPending = true;
}

void UExcavationTerrainChunkSubsystem::QueueChunkRebuild(const FIntVector& Coordinate)
{
    if (!bRuntimeReady || DirtyChunkQueue.Contains(Coordinate))
    {
        return;
    }

    const int32 ProfileQueueLimit = FMath::Clamp(RuntimeSettings.MaxQueuedMeshJobs, 1, 4096);
    const int32 ConfiguredQueueLimit = FMath::Clamp(MaxQueuedDirtyChunks, 16, 4096);
    const int32 QueueLimit = FMath::Min(ProfileQueueLimit, ConfiguredQueueLimit);
    if (DirtyChunkQueue.Num() >= QueueLimit)
    {
        // One bounded full reconciliation is safer than silently losing final state.
        DirtyChunkQueue.Reset();
        FullRefreshCoordinates.Reset();
        FullRefreshCursor = 0;
        bFullRefreshListReady = false;
        bFullRefreshPending = true;
        return;
    }

    DirtyChunkQueue.Add(Coordinate);
}

bool UExcavationTerrainChunkSubsystem::IsChunkMeshCurrent(
    const AExcavationTerrainChunkActor* Actor) const
{
    if (!bRuntimeReady || !IsValid(Actor) || Actor->GetPublishedDensityRevision() == 0
        || Actor->GetWorld() != GetWorld() || bFullRefreshPending)
    {
        return false;
    }

    const FIntVector Coordinate = Actor->GetChunkCoordinate();
    if (DirtyChunkQueue.Contains(Coordinate)
        || InFlightMeshJobs.Contains(Coordinate)
        || CompletedMeshResults.Contains(Coordinate))
    {
        return false;
    }

    const TArray<TWeakObjectPtr<AExcavationTerrainChunkActor>>* Bucket = RegisteredChunks.Find(Coordinate);
    if (!Bucket)
    {
        return false;
    }

    for (const TWeakObjectPtr<AExcavationTerrainChunkActor>& Entry : *Bucket)
    {
        if (Entry.Get() == Actor)
        {
            return true;
        }
    }
    return false;
}

void UExcavationTerrainChunkSubsystem::CompleteMeshJob(
    const FIntVector& Coordinate,
    FQueuedVoxelMeshResult&& Result)
{
    // This function is only called on the game thread by AsyncTask's continuation.
    InFlightMeshJobs.Remove(Coordinate);
    if (!bRuntimeReady)
    {
        return;
    }

    CompletedMeshResults.Add(Coordinate, MoveTemp(Result));
}

void UExcavationTerrainChunkSubsystem::ProcessDirtyChunkQueue()
{
    if (!bRuntimeReady)
    {
        return;
    }

    const int32 QueueLimit = FMath::Min(
        FMath::Clamp(RuntimeSettings.MaxQueuedMeshJobs, 1, 4096),
        FMath::Clamp(MaxQueuedDirtyChunks, 16, 4096));

    if (bFullRefreshPending && !bFullRefreshListReady)
    {
        FullRefreshCoordinates.Reset();
        for (const TPair<FIntVector, TArray<TWeakObjectPtr<AExcavationTerrainChunkActor>>>& Pair : RegisteredChunks)
        {
            for (const TWeakObjectPtr<AExcavationTerrainChunkActor>& Entry : Pair.Value)
            {
                if (Entry.IsValid())
                {
                    FullRefreshCoordinates.Add(Pair.Key);
                    break;
                }
            }
        }
        FullRefreshCoordinates.Sort([](const FIntVector& A, const FIntVector& B)
        {
            if (A.X != B.X) return A.X < B.X;
            if (A.Y != B.Y) return A.Y < B.Y;
            return A.Z < B.Z;
        });
        FullRefreshCursor = 0;
        bFullRefreshListReady = true;
    }

    // Reconciliation feeds the bounded dirty queue incrementally, so a reset over
    // many active chunks cannot defeat MaxQueuedMeshJobs with one unbounded burst.
    while (bFullRefreshPending
        && DirtyChunkQueue.Num() < QueueLimit
        && FullRefreshCursor < FullRefreshCoordinates.Num())
    {
        const FIntVector Coordinate = FullRefreshCoordinates[FullRefreshCursor++];
        if (!InFlightMeshJobs.Contains(Coordinate) && !CompletedMeshResults.Contains(Coordinate))
        {
            DirtyChunkQueue.Add(Coordinate);
        }
    }

    if (bFullRefreshPending && FullRefreshCursor >= FullRefreshCoordinates.Num())
    {
        bFullRefreshPending = false;
        bFullRefreshListReady = false;
        FullRefreshCoordinates.Reset();
        FullRefreshCursor = 0;
    }

    const int32 PublishBudget = FMath::Max(0, FMath::Min(
        RuntimeSettings.MaxChunksPublishedPerFrame,
        RuntimeSettings.MaxCollisionUpdatesPerFrame));
    if (PublishBudget == 0)
    {
        return;
    }

    UWorld* World = GetWorld();
    UExcavationVolumeSubsystem* Excavation =
        World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!Excavation)
    {
        RuntimeSetupError = TEXT("Density owner became unavailable while processing voxel mesh jobs.");
        return;
    }

    // 1) Publish completed CPU output on the game thread under a shared mesh/collision budget.
    TArray<FIntVector> CompletedCoordinates;
    CompletedMeshResults.GenerateKeyArray(CompletedCoordinates);
    CompletedCoordinates.Sort([](const FIntVector& A, const FIntVector& B)
    {
        if (A.X != B.X) return A.X < B.X;
        if (A.Y != B.Y) return A.Y < B.Y;
        return A.Z < B.Z;
    });

    int32 PublishedThisTick = 0;
    for (const FIntVector& Coordinate : CompletedCoordinates)
    {
        if (PublishedThisTick >= PublishBudget)
        {
            break;
        }

        FQueuedVoxelMeshResult* FoundResult = CompletedMeshResults.Find(Coordinate);
        if (!FoundResult)
        {
            continue;
        }

        FQueuedVoxelMeshResult Result = MoveTemp(*FoundResult);
        CompletedMeshResults.Remove(Coordinate);

        if (!Result.bSucceeded)
        {
            // Ignore an error produced from an obsolete snapshot; a new job can recover it.
            if (Result.SourceRevision != Excavation->GetDensityRevision())
            {
                QueueChunkRebuild(Coordinate);
                continue;
            }

            TArray<TWeakObjectPtr<AExcavationTerrainChunkActor>>* FailedBucket = RegisteredChunks.Find(Coordinate);
            if (FailedBucket)
            {
                FailedBucket->RemoveAll([](const TWeakObjectPtr<AExcavationTerrainChunkActor>& Entry)
                {
                    return !Entry.IsValid();
                });
                AExcavationTerrainChunkActor* FailedActor = nullptr;
                for (const TWeakObjectPtr<AExcavationTerrainChunkActor>& Entry : *FailedBucket)
                {
                    if (AExcavationTerrainChunkActor* Actor = Entry.Get())
                    {
                        FailedActor = Actor;
                        break;
                    }
                }
                if (FailedActor)
                {
                    FailedActor->InvalidateVoxelMesh();
                    ++PublishedThisTick;
                }
            }

            UE_LOG(LogTemp, Warning, TEXT("Voxel meshing failed for current chunk revision (%d,%d,%d): %s"),
                Coordinate.X, Coordinate.Y, Coordinate.Z, *Result.Error);
            continue;
        }

        // A terrain change may have occurred while the worker was running.
        if (Excavation->GetDensityRevision() != Result.Mesh.SourceRevision)
        {
            QueueChunkRebuild(Coordinate);
            continue;
        }

        TArray<TWeakObjectPtr<AExcavationTerrainChunkActor>>* Bucket = RegisteredChunks.Find(Coordinate);
        if (!Bucket)
        {
            continue;
        }

        Bucket->RemoveAll([](const TWeakObjectPtr<AExcavationTerrainChunkActor>& Entry)
        {
            return !Entry.IsValid();
        });

        AExcavationTerrainChunkActor* TargetActor = nullptr;
        for (const TWeakObjectPtr<AExcavationTerrainChunkActor>& Entry : *Bucket)
        {
            if (AExcavationTerrainChunkActor* Actor = Entry.Get())
            {
                TargetActor = Actor;
                break;
            }
        }
        if (!TargetActor)
        {
            continue;
        }

        FString PublishError;
        if (!TargetActor->PublishVoxelMeshData(Result.Mesh, PublishError))
        {
            // Never leave an old collider authoritative after a current-revision publish
            // failure. A stale revision is re-queued by PublishVoxelMeshData; any other
            // failure remains safely non-interactive until a later rebuild request.
            TargetActor->InvalidateVoxelMesh();
            if (!PublishError.Contains(TEXT("stale voxel mesh")))
            {
                UE_LOG(LogTemp, Warning, TEXT("Voxel mesh publication failed for chunk (%d,%d,%d): %s"),
                    Coordinate.X, Coordinate.Y, Coordinate.Z, *PublishError);
            }
        }
        ++PublishedThisTick;
    }

    if (!RuntimeSettings.bAllowBackgroundMeshing)
    {
        // Synchronous fallback still respects per-frame publication/collision limits.
        TArray<FIntVector> OrderedDirtyChunks = DirtyChunkQueue.Array();
        OrderedDirtyChunks.Sort([](const FIntVector& A, const FIntVector& B)
        {
            if (A.X != B.X) return A.X < B.X;
            if (A.Y != B.Y) return A.Y < B.Y;
            return A.Z < B.Z;
        });

        for (const FIntVector& Coordinate : OrderedDirtyChunks)
        {
            if (PublishedThisTick >= PublishBudget)
            {
                break;
            }
            if (InFlightMeshJobs.Contains(Coordinate) || CompletedMeshResults.Contains(Coordinate))
            {
                continue;
            }

            TArray<TWeakObjectPtr<AExcavationTerrainChunkActor>>* Bucket = RegisteredChunks.Find(Coordinate);
            if (!Bucket)
            {
                DirtyChunkQueue.Remove(Coordinate);
                continue;
            }
            Bucket->RemoveAll([](const TWeakObjectPtr<AExcavationTerrainChunkActor>& Entry)
            {
                return !Entry.IsValid();
            });
            if (Bucket->IsEmpty())
            {
                DirtyChunkQueue.Remove(Coordinate);
                continue;
            }

            DirtyChunkQueue.Remove(Coordinate);
            if (AExcavationTerrainChunkActor* Actor = Bucket->Last().Get())
            {
                Actor->RebuildTerrainChunk();
            }
            ++PublishedThisTick;
        }
        return;
    }

    // 2) Snapshot sampling stays on the game thread because a base density provider
    // can query Landscape/UObjects. The pure mesher receives arrays only on workers.
    const int32 JobLimit = FMath::Clamp(RuntimeSettings.MaxMeshJobsInFlight, 1, 64);
    TArray<FIntVector> OrderedDirtyChunks = DirtyChunkQueue.Array();
    OrderedDirtyChunks.Sort([](const FIntVector& A, const FIntVector& B)
    {
        if (A.X != B.X) return A.X < B.X;
        if (A.Y != B.Y) return A.Y < B.Y;
        return A.Z < B.Z;
    });

    for (const FIntVector& Coordinate : OrderedDirtyChunks)
    {
        // Count finished-but-not-yet-published jobs as outstanding too. This bounds
        // worker output memory when publication/collision budgets are the bottleneck.
        if (InFlightMeshJobs.Num() + CompletedMeshResults.Num() >= JobLimit)
        {
            break;
        }
        if (InFlightMeshJobs.Contains(Coordinate) || CompletedMeshResults.Contains(Coordinate))
        {
            continue;
        }

        TArray<TWeakObjectPtr<AExcavationTerrainChunkActor>>* Bucket = RegisteredChunks.Find(Coordinate);
        if (!Bucket)
        {
            DirtyChunkQueue.Remove(Coordinate);
            continue;
        }
        Bucket->RemoveAll([](const TWeakObjectPtr<AExcavationTerrainChunkActor>& Entry)
        {
            return !Entry.IsValid();
        });
        if (Bucket->IsEmpty())
        {
            DirtyChunkQueue.Remove(Coordinate);
            continue;
        }

        FWorldDensityChunkSnapshot Snapshot;
        if (!Excavation->BuildChunkSnapshot(Coordinate, Snapshot))
        {
            DirtyChunkQueue.Remove(Coordinate);
            FQueuedVoxelMeshResult Failure;
            Failure.SourceRevision = Excavation->GetDensityRevision();
            Failure.Error = TEXT("Could not capture a valid density snapshot.");
            CompletedMeshResults.Add(Coordinate, MoveTemp(Failure));
            continue;
        }

        DirtyChunkQueue.Remove(Coordinate);
        InFlightMeshJobs.Add(Coordinate);
        TWeakObjectPtr<UExcavationTerrainChunkSubsystem> WeakThis(this);

        Async(EAsyncExecution::ThreadPool,
            [WeakThis, Coordinate, Snapshot = MoveTemp(Snapshot)]() mutable
            {
                FQueuedVoxelMeshResult Result;
                Result.SourceRevision = Snapshot.SourceRevision;
                const FWorldMarchingTetrahedraMesher Mesher;
                Result.bSucceeded = Mesher.BuildMesh(Snapshot, Result.Mesh, Result.Error);
                if (!Result.bSucceeded && Result.Error.IsEmpty())
                {
                    Result.Error = TEXT("The CPU mesher rejected the density snapshot.");
                }

                AsyncTask(ENamedThreads::GameThread,
                    [WeakThis, Coordinate, Result = MoveTemp(Result)]() mutable
                    {
                        if (UExcavationTerrainChunkSubsystem* Subsystem = WeakThis.Get())
                        {
                            Subsystem->CompleteMeshJob(Coordinate, MoveTemp(Result));
                        }
                    });
            });
    }
}

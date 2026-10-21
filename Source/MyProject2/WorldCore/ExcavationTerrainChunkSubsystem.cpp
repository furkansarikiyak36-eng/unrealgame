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
#include "GameFramework/PlayerStart.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "UObject/UObjectIterator.h"

void UExcavationTerrainChunkSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UExcavationVolumeSubsystem>();

    if (!TerrainChunkActorClass)
    {
        TerrainChunkActorClass = AExcavationTerrainChunkActor::StaticClass();
    }

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

void UExcavationTerrainChunkSubsystem::QueueRebuild(AExcavationTerrainChunkActor* Actor)
{
    if (IsValid(Actor))
    {
        PendingRebuilds.AddUnique(Actor);
    }
}

void UExcavationTerrainChunkSubsystem::ProcessPendingRebuilds(double BudgetMs)
{
    const double Start = FPlatformTime::Seconds();
    int32 Done = 0;
    while (PendingRebuilds.Num() > 0)
    {
        if (Done > 0 && (FPlatformTime::Seconds() - Start) * 1000.0 >= BudgetMs)
        {
            break;
        }
        const TWeakObjectPtr<AExcavationTerrainChunkActor> Next = PendingRebuilds[0];
        PendingRebuilds.RemoveAt(0, EAllowShrinking::No);
        if (AExcavationTerrainChunkActor* Actor = Next.Get())
        {
            Actor->RebuildTerrainChunk();
            ++Done;
        }
    }
}

int32 UExcavationTerrainChunkSubsystem::FlushPendingRebuilds()
{
    const int32 Count = PendingRebuilds.Num();
    ProcessPendingRebuilds(TNumericLimits<float>::Max());
    return Count;
}

void UExcavationTerrainChunkSubsystem::Tick(float DeltaTime)
{
    ProcessPendingRebuilds(FMath::Clamp(RebuildBudgetMs, 0.5f, 50.0f));

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
    // The very first update runs at once: loose logs and stones in the level would otherwise fall through.
    if (!ManagedChunkActors.IsEmpty() && StreamingAccumulatorSeconds < FMath::Max(0.01f, StreamingIntervalSeconds))
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
        const bool bMaterialMappingsChanged = LandscapeFallbackDensitySource->SetMaterialMappings(LandscapeMaterialMappings);
        if (RegisteredFallbackDensitySource != LandscapeFallbackDensitySource)
        {
            if (!Excavation->RegisterFallbackBaseDensitySource(LandscapeFallbackDensitySource))
            {
                return;
            }
            RegisteredFallbackDensitySource = LandscapeFallbackDensitySource;
            bProviderChanged = true;
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
        }
    }

    // A provider/proxy set change alters density samples even without a cell edit.
    if (bProviderChanged)
    {
        HandleVolumeReset();
    }
}

void UExcavationTerrainChunkSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    if (!bEnableAutomaticStreaming || !InWorld.IsGameWorld())
    {
        return;
    }
    // The base terrain source first, then the ground round the player start, built now: the game mode snaps
    // its starting items to the ground right after this, before any actor's BeginPlay.
    LandscapeDiscoveryAccumulatorSeconds = 0.0f;
    bDidInitialLandscapeDiscovery = true;
    RefreshLandscapeFallbackProvider();
    FVector Center = FVector::ZeroVector;
    for (TActorIterator<APlayerStart> It(&InWorld); It; ++It)
    {
        Center = It->GetActorLocation();
        break;
    }
    UpdateStreamedChunksAround(Center, true);
}

void UExcavationTerrainChunkSubsystem::UpdateStreamedChunks()
{
    UWorld* World = GetWorld();
    APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
    APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
    if (Pawn)
    {
        UpdateStreamedChunksAround(Pawn->GetActorLocation(), false);
    }
}

void UExcavationTerrainChunkSubsystem::UpdateStreamedChunksAround(const FVector& Center, bool bBuildNow)
{
    UWorld* World = GetWorld();
    UExcavationVolumeSubsystem* Excavation =
        World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!World || !Excavation)
    {
        return;
    }

    FIntVector PlayerCell;
    FIntVector PlayerChunk;
    if (!Excavation->TryWorldToCell(Center, PlayerCell)
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
        if (!Actor || (bUnloadDistantChunks && !DesiredChunks.Contains(It.Key())))
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
    // Nearest first, so the ground under the player is there before anything else.
    OrderedDesiredChunks.Sort([&PlayerChunk](const FIntVector& A, const FIntVector& B)
    {
        const FIntVector DA = A - PlayerChunk;
        const FIntVector DB = B - PlayerChunk;
        const int64 HA = static_cast<int64>(DA.X) * DA.X + static_cast<int64>(DA.Y) * DA.Y;
        const int64 HB = static_cast<int64>(DB.X) * DB.X + static_cast<int64>(DB.Y) * DB.Y;
        if (HA != HB) return HA < HB;
        if (FMath::Abs(DA.Z) != FMath::Abs(DB.Z)) return FMath::Abs(DA.Z) < FMath::Abs(DB.Z);
        if (A.X != B.X) return A.X < B.X;
        if (A.Y != B.Y) return A.Y < B.Y;
        return A.Z < B.Z;
    });

    const bool bInitialLoad = ManagedChunkActors.IsEmpty();
    int32 SpawnedThisTick = 0;
    const int32 SpawnBudget = FMath::Clamp(MaxChunksSpawnedPerTick, 1, 16);
    for (const FIntVector& Coordinate : OrderedDesiredChunks)
    {
        // The player's own column is never held back by the budget: without it the player falls through.
        const bool bUnderPlayer = Coordinate.X == PlayerChunk.X && Coordinate.Y == PlayerChunk.Y;
        // On the first update (nothing streamed yet) the whole ground goes in at once, before physics drops anything.
        if (SpawnedThisTick >= SpawnBudget && !bUnderPlayer && !bInitialLoad && !bBuildNow)
        {
            break;
        }

        if (ManagedChunkActors.Contains(Coordinate) || HasLiveRegisteredChunk(Coordinate))
        {
            continue;
        }

        if (!SpawnManagedChunk(Coordinate, bBuildNow))
        {
            continue;
        }
        ++SpawnedThisTick;
    }
}

AExcavationTerrainChunkActor* UExcavationTerrainChunkSubsystem::SpawnManagedChunk(const FIntVector& Coordinate, bool bBuildNow)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
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
        return nullptr;
    }
    // Assign the coordinate before BeginPlay so the actor never builds its default (0,0,0) chunk.
    Actor->ChunkCoordinate = Coordinate;
    Actor->FinishSpawning(FTransform::Identity);
    if (bBuildNow && !Actor->HasActorBegunPlay())
    {
        Actor->RebuildTerrainChunk();
    }
    ManagedChunkActors.Add(Coordinate, Actor);
    return Actor;
}

void UExcavationTerrainChunkSubsystem::EnsureGroundAt(const FVector& Location)
{
    UWorld* World = GetWorld();
    UExcavationVolumeSubsystem* Excavation = World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!Excavation || !bEnableAutomaticStreaming || !World->IsGameWorld())
    {
        return;
    }
    if (!bDidInitialLandscapeDiscovery)
    {
        bDidInitialLandscapeDiscovery = true;
        RefreshLandscapeFallbackProvider();
    }
    FIntVector Cell;
    FIntVector Chunk;
    if (!Excavation->TryWorldToCell(Location, Cell) || !Excavation->TryGetChunkCoordinateForCell(Cell, Chunk))
    {
        return;
    }
    // The chunk holding the point and the one below it: a surface on a chunk boundary is built by the lower one.
    for (int32 ZOffset = -1; ZOffset <= 0; ++ZOffset)
    {
        const FIntVector Coordinate(Chunk.X, Chunk.Y, Chunk.Z + ZOffset);
        if (!ManagedChunkActors.Contains(Coordinate) && !HasLiveRegisteredChunk(Coordinate))
        {
            SpawnManagedChunk(Coordinate, true);
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

    if (!bIsRollback && Record.EditType == EExcavationEditType::Carve)
    {
        RemoveGroundCoverAround(Record);
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
                QueueRebuild(Actor);
            }
        }
    }
}

void UExcavationTerrainChunkSubsystem::RemoveGroundCoverAround(const FExcavationEditRecord& Record)
{
    // Grass, flowers and small plants are instances without collision; where the ground is dug away they would
    // hang in the air over the hole. Trees and rocks (with collision, or taller than 1.5 m) are left alone.
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    float Reach = Record.RadiusCm;
    FVector Center = Record.CenterCm;
    if (Record.ShapeType == EExcavationShapeType::Box)
    {
        Reach = static_cast<float>(Record.HalfExtentCm.GetMax());
    }
    else if (Record.ShapeType == EExcavationShapeType::Capsule)
    {
        Center = (Record.CenterCm + Record.EndCm) * 0.5;
        Reach += static_cast<float>(FVector::Dist(Record.CenterCm, Record.EndCm)) * 0.5f;
    }
    Reach += 30.0f;
    for (TObjectIterator<UInstancedStaticMeshComponent> It; It; ++It)
    {
        UInstancedStaticMeshComponent* Instances = *It;
        if (!IsValid(Instances) || Instances->GetWorld() != World || Instances->GetInstanceCount() == 0
            || Instances->GetCollisionEnabled() != ECollisionEnabled::NoCollision || !Instances->GetStaticMesh())
        {
            continue;
        }
        if (Instances->GetStaticMesh()->GetBounds().BoxExtent.Z > 75.0)
        {
            continue;
        }
        if (!Instances->Bounds.GetBox().ExpandBy(Reach).IsInside(Center))
        {
            continue;
        }
        TArray<int32> Hits = Instances->GetInstancesOverlappingSphere(Center, Reach, true);
        if (Hits.Num() > 0)
        {
            Instances->RemoveInstances(Hits);
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
                QueueRebuild(Actor);
            }
        }

        if (Bucket.IsEmpty())
        {
            It.RemoveCurrent();
        }
    }
}

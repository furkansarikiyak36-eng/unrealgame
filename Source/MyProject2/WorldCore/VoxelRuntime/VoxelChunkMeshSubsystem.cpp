#include "VoxelChunkMeshSubsystem.h"

#include "Async/Async.h"
#include "Engine/World.h"
#include "LandscapeBuilderSubsystem.h"
#include "WorldCore/Classification/LandscapeBuilderSubsystem.h"
#include "WorldCore/ExcavationVolumeSubsystem.h"
#include "WorldCore/VoxelRuntime/WorldRuntimeProfile.h"
#include "ProceduralVoxelChunkActor.h"
#include "Engine/Engine.h"

namespace
{
    struct FVertexKeyHasher
    {
        uint32 operator()(const FIntVector& K) const
        {
            return HashCombine(HashCombine(GetTypeHash(K.X), GetTypeHash(K.Y)), GetTypeHash(K.Z));
        }
    };

    struct FMeshBuffers
    {
        TArray<FVector> Vertices;
        TArray<int32> Triangles;
        TArray<FVector> Normals;
    };

    static int32 AddSharedVertex(FMeshBuffers& Mesh, TMap<FIntVector, int32, FDefaultSetAllocator, TDefaultMapHashableKeyFuncs<FIntVector, int32, false>>& Cache, const FVector& Position)
    {
        const FIntVector Key(FMath::RoundToInt(Position.X * 10.0f),
            FMath::RoundToInt(Position.Y * 10.0f), FMath::RoundToInt(Position.Z * 10.0f));
        if (const int32* Existing = Cache.Find(Key))
        {
            return *Existing;
        }
        const int32 Index = Mesh.Vertices.Add(Position);
        Cache.Add(Key, Index);
        return Index;
    }

    static void AddOrientedTriangle(FMeshBuffers& Mesh,
        TMap<FIntVector, int32, FDefaultSetAllocator, TDefaultMapHashableKeyFuncs<FIntVector, int32, false>>& Cache,
        FVector A, FVector B, FVector C, const FVector& Outward)
    {
        FVector FaceNormal = FVector::CrossProduct(B - A, C - A);
        if (FVector::DotProduct(FaceNormal, Outward) < 0.0f)
        {
            Swap(B, C);
        }
        Mesh.Triangles.Add(AddSharedVertex(Mesh, Cache, A));
        Mesh.Triangles.Add(AddSharedVertex(Mesh, Cache, B));
        Mesh.Triangles.Add(AddSharedVertex(Mesh, Cache, C));
    }

    static FVector IntersectHalfDensity(const FVector& A, const FVector& B, float DA, float DB)
    {
        const float Denominator = DB - DA;
        const float T = FMath::Abs(Denominator) <= SMALL_NUMBER ? 0.5f : FMath::Clamp((0.5f - DA) / Denominator, 0.0f, 1.0f);
        return FMath::Lerp(A, B, T);
    }

    static FMeshBuffers BuildMarchingTetrahedra(const TArray<float>& Samples, int32 N, float CellSizeCm)
    {
        FMeshBuffers Mesh;
        const int32 Dim = N + 1;
        auto SampleIndex = [Dim](int32 X, int32 Y, int32 Z) { return X + Dim * (Y + Dim * Z); };
        const FIntVector CornerOffset[8] = {
            FIntVector(0,0,0), FIntVector(1,0,0), FIntVector(1,1,0), FIntVector(0,1,0),
            FIntVector(0,0,1), FIntVector(1,0,1), FIntVector(1,1,1), FIntVector(0,1,1)
        };
        const int32 Tetrahedra[6][4] = {
            {0,5,1,6}, {0,1,2,6}, {0,2,3,6},
            {0,3,7,6}, {0,7,4,6}, {0,4,5,6}
        };

        TMap<FIntVector, int32, FDefaultSetAllocator, TDefaultMapHashableKeyFuncs<FIntVector, int32, false>> VertexCache;
        VertexCache.Reserve(N * N * 6);

        for (int32 Z = 0; Z < N; ++Z)
        for (int32 Y = 0; Y < N; ++Y)
        for (int32 X = 0; X < N; ++X)
        {
            FVector P[8];
            float D[8];
            for (int32 C = 0; C < 8; ++C)
            {
                const FIntVector O = CornerOffset[C];
                const int32 SX = X + O.X, SY = Y + O.Y, SZ = Z + O.Z;
                P[C] = FVector((SX + 0.5f) * CellSizeCm, (SY + 0.5f) * CellSizeCm, (SZ + 0.5f) * CellSizeCm);
                D[C] = Samples[SampleIndex(SX, SY, SZ)];
            }

            for (const auto& Tet : Tetrahedra)
            {
                int32 Inside[4], Outside[4], InsideCount = 0, OutsideCount = 0;
                FVector InsideCenter = FVector::ZeroVector, OutsideCenter = FVector::ZeroVector;
                for (int32 I = 0; I < 4; ++I)
                {
                    const int32 C = Tet[I];
                    if (D[C] >= 0.5f)
                    {
                        Inside[InsideCount++] = C;
                        InsideCenter += P[C];
                    }
                    else
                    {
                        Outside[OutsideCount++] = C;
                        OutsideCenter += P[C];
                    }
                }
                if (InsideCount == 0 || InsideCount == 4) continue;
                InsideCenter /= static_cast<float>(InsideCount);
                OutsideCenter /= static_cast<float>(OutsideCount);
                const FVector Outward = (OutsideCenter - InsideCenter).GetSafeNormal();

                if (InsideCount == 1)
                {
                    const int32 A = Inside[0];
                    const FVector V0 = IntersectHalfDensity(P[A], P[Outside[0]], D[A], D[Outside[0]]);
                    const FVector V1 = IntersectHalfDensity(P[A], P[Outside[1]], D[A], D[Outside[1]]);
                    const FVector V2 = IntersectHalfDensity(P[A], P[Outside[2]], D[A], D[Outside[2]]);
                    AddOrientedTriangle(Mesh, VertexCache, V0, V1, V2, Outward);
                }
                else if (InsideCount == 3)
                {
                    const int32 A = Outside[0];
                    const FVector V0 = IntersectHalfDensity(P[A], P[Inside[0]], D[A], D[Inside[0]]);
                    const FVector V1 = IntersectHalfDensity(P[A], P[Inside[1]], D[A], D[Inside[1]]);
                    const FVector V2 = IntersectHalfDensity(P[A], P[Inside[2]], D[A], D[Inside[2]]);
                    AddOrientedTriangle(Mesh, VertexCache, V0, V1, V2, Outward);
                }
                else
                {
                    const int32 A = Inside[0], B = Inside[1], C = Outside[0], DOut = Outside[1];
                    const FVector AC = IntersectHalfDensity(P[A], P[C], D[A], D[C]);
                    const FVector AD = IntersectHalfDensity(P[A], P[DOut], D[A], D[DOut]);
                    const FVector BC = IntersectHalfDensity(P[B], P[C], D[B], D[C]);
                    const FVector BD = IntersectHalfDensity(P[B], P[DOut], D[B], D[DOut]);
                    AddOrientedTriangle(Mesh, VertexCache, AC, AD, BD, Outward);
                    AddOrientedTriangle(Mesh, VertexCache, AC, BD, BC, Outward);
                }
            }
        }

        Mesh.Normals.SetNumZeroed(Mesh.Vertices.Num());
        for (int32 I = 0; I + 2 < Mesh.Triangles.Num(); I += 3)
        {
            const int32 IA = Mesh.Triangles[I], IB = Mesh.Triangles[I + 1], IC = Mesh.Triangles[I + 2];
            const FVector Nrm = FVector::CrossProduct(Mesh.Vertices[IB] - Mesh.Vertices[IA],
                Mesh.Vertices[IC] - Mesh.Vertices[IA]);
            Mesh.Normals[IA] += Nrm;
            Mesh.Normals[IB] += Nrm;
            Mesh.Normals[IC] += Nrm;
        }
        for (FVector& Normal : Mesh.Normals)
        {
            Normal = Normal.GetSafeNormal();
        }
        return Mesh;
    }
}

void UVoxelChunkMeshSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    if (UWorld* World = GetWorld())
    {
        if (UExcavationVolumeSubsystem* Volume = World->GetSubsystem<UExcavationVolumeSubsystem>())
        {
            UWorldRuntimeProfileSettings::Get().ApplyTo(Volume);
            Volume->OnVolumeChanged.AddDynamic(this, &UVoxelChunkMeshSubsystem::HandleVolumeChanged);
            Volume->OnVolumeReset.AddDynamic(this, &UVoxelChunkMeshSubsystem::HandleVolumeReset);
        }
    }
}

void UVoxelChunkMeshSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        if (UExcavationVolumeSubsystem* Volume = World->GetSubsystem<UExcavationVolumeSubsystem>())
        {
            Volume->OnVolumeChanged.RemoveDynamic(this, &UVoxelChunkMeshSubsystem::HandleVolumeChanged);
            Volume->OnVolumeReset.RemoveDynamic(this, &UVoxelChunkMeshSubsystem::HandleVolumeReset);
        }
    }
    DirtyChunks.Reset();
    InFlightChunks.Reset();
    PendingPublishes.Reset();
    ChunkActors.Reset();
    Super::Deinitialize();
}

TStatId UVoxelChunkMeshSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UVoxelChunkMeshSubsystem, STATGROUP_Tickables);
}

bool UVoxelChunkMeshSubsystem::IsTickable() const
{
    return GetWorld() != nullptr && !GetWorld()->IsPreviewWorld();
}

void UVoxelChunkMeshSubsystem::Tick(float DeltaTime)
{
    const UWorldRuntimeProfileSettings& Profile = UWorldRuntimeProfileSettings::Get();

    int32 Published = 0;
    while (!PendingPublishes.IsEmpty() && Published < FMath::Max(1, Profile.MaxChunksPublishedPerTick))
    {
        FCompletedMesh Completed = MoveTemp(PendingPublishes[0]);
        PendingPublishes.RemoveAt(0, 1, EAllowShrinking::No);
        PublishCompleted(MoveTemp(Completed));
        ++Published;
    }

    const int32 MaxInFlight = Profile.GetMaxMeshJobsInFlight();
    int32 Started = 0;
    while (Started < FMath::Max(1, Profile.MaxChunksSampledPerTick)
        && InFlightChunks.Num() < MaxInFlight && !DirtyChunks.IsEmpty())
    {
        TArray<FIntVector> Ordered = DirtyChunks.Array();
        Ordered.Sort([](const FIntVector& A, const FIntVector& B)
        {
            if (A.X != B.X) return A.X < B.X;
            if (A.Y != B.Y) return A.Y < B.Y;
            return A.Z < B.Z;
        });

        FIntVector Next = Ordered[0];
        DirtyChunks.Remove(Next);
        if (InFlightChunks.Contains(Next))
        {
            DirtyChunks.Add(Next);
            break;
        }
        StartOneBuild(Next);
        ++Started;
    }
}

void UVoxelChunkMeshSubsystem::QueueChunk(FIntVector Coordinate)
{
    DirtyChunks.Add(Coordinate);
}

void UVoxelChunkMeshSubsystem::HandleVolumeChanged(const FExcavationEditRecord& Record, bool bIsRollback)
{
    if (UWorld* World = GetWorld())
    {
        if (UExcavationVolumeSubsystem* Volume = World->GetSubsystem<UExcavationVolumeSubsystem>())
        {
            for (const FIntVector& Chunk : Volume->GetAffectedChunksForEdit(Record))
            {
                QueueChunk(Chunk);
            }
        }
    }
}

void UVoxelChunkMeshSubsystem::HandleVolumeReset()
{
    if (UWorld* World = GetWorld())
    {
        if (UExcavationVolumeSubsystem* Volume = World->GetSubsystem<UExcavationVolumeSubsystem>())
        {
            FExcavationVolumeSnapshot Snapshot;
            if (Volume->ExportSnapshot(Snapshot))
            {
                for (const FExcavationCellSnapshot& Row : Snapshot.Overrides)
                {
                    FIntVector Chunk;
                    if (Volume->TryGetChunkCoordinateForCell(Row.Cell, Chunk))
                    {
                        QueueChunk(Chunk);
                    }
                }
            }
        }
    }
    for (const TPair<FIntVector, TObjectPtr<AProceduralVoxelChunkActor>>& Pair : ChunkActors)
    {
        QueueChunk(Pair.Key);
    }
}

void UVoxelChunkMeshSubsystem::RequestRebuildRegion(FVector CenterCm, FVector ExtentCm)
{
    if (ExtentCm.X <= 0.0f || ExtentCm.Y <= 0.0f || ExtentCm.Z <= 0.0f) return;
    UWorld* World = GetWorld();
    UExcavationVolumeSubsystem* Volume = World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!Volume) return;

    FIntVector MinCell, MaxCell;
    if (!Volume->TryWorldToCell(CenterCm - ExtentCm, MinCell)
        || !Volume->TryWorldToCell(CenterCm + ExtentCm, MaxCell)) return;

    FIntVector MinChunk, MaxChunk;
    if (!Volume->TryGetChunkCoordinateForCell(MinCell, MinChunk)
        || !Volume->TryGetChunkCoordinateForCell(MaxCell, MaxChunk)) return;

    const int64 Count = static_cast<int64>(MaxChunk.X - MinChunk.X + 1)
        * static_cast<int64>(MaxChunk.Y - MinChunk.Y + 1)
        * static_cast<int64>(MaxChunk.Z - MinChunk.Z + 1);
    if (Count <= 0 || Count > 100000) return;

    for (int32 X = MinChunk.X; X <= MaxChunk.X; ++X)
    for (int32 Y = MinChunk.Y; Y <= MaxChunk.Y; ++Y)
    for (int32 Z = MinChunk.Z; Z <= MaxChunk.Z; ++Z)
    {
        QueueChunk(FIntVector(X, Y, Z));
    }
}

void UVoxelChunkMeshSubsystem::RequestRebuildAllEditedChunks()
{
    HandleVolumeReset();
}

void UVoxelChunkMeshSubsystem::StartOneBuild(FIntVector Coordinate)
{
    UWorld* World = GetWorld();
    UExcavationVolumeSubsystem* Volume = World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!World || !Volume)
    {
        return;
    }

    const float CellSize = Volume->CellSizeCm;
    const int32 N = Volume->CellsPerChunkAxis;
    const int32 Dim = N + 1;
    if (!FMath::IsFinite(CellSize) || CellSize < 1.0f || N < 4 || N > 96)
    {
        return;
    }

    const FVector WorldOrigin(static_cast<double>(Coordinate.X) * N * CellSize,
        static_cast<double>(Coordinate.Y) * N * CellSize,
        static_cast<double>(Coordinate.Z) * N * CellSize);
    TArray<float> Samples;
    Samples.SetNumUninitialized(Dim * Dim * Dim);

    ULandscapeBuilderSubsystem* Builder = World->GetSubsystem<ULandscapeBuilderSubsystem>();
    for (int32 Z = 0; Z < Dim; ++Z)
    for (int32 Y = 0; Y < Dim; ++Y)
    for (int32 X = 0; X < Dim; ++X)
    {
        const FVector WorldPoint = WorldOrigin + FVector((X + 0.5) * CellSize, (Y + 0.5) * CellSize, (Z + 0.5) * CellSize);
        float Density = 0.0f;
        const bool bHasDensity = Volume->TrySampleDensityAtWorld(WorldPoint, Density);
        if (!bHasDensity) Density = 0.0f;

        if (Builder && Builder->QueryModeAtWorld(WorldPoint).Mode == EWorldTerrainMode::StaticGeometry)
        {
            Density = 0.0f;
        }
        Samples[X + Dim * (Y + Dim * Z)] = FMath::Clamp(Density, 0.0f, 1.0f);
    }

    InFlightChunks.Add(Coordinate);
    TWeakObjectPtr<UVoxelChunkMeshSubsystem> WeakThis(this);
    Async(EAsyncExecution::ThreadPool,
        [WeakThis, Coordinate, WorldOrigin, CellSize, N, Samples = MoveTemp(Samples)]() mutable
        {
            FMeshBuffers Built = BuildMarchingTetrahedra(Samples, N, CellSize);
            AsyncTask(ENamedThreads::GameThread,
                [WeakThis, Coordinate, WorldOrigin, Built = MoveTemp(Built)]() mutable
                {
                    if (UVoxelChunkMeshSubsystem* Self = WeakThis.Get())
                    {
                        Self->InFlightChunks.Remove(Coordinate);
                        FCompletedMesh Completed;
                        Completed.Coordinate = Coordinate;
                        Completed.WorldOrigin = WorldOrigin;
                        Completed.Vertices = MoveTemp(Built.Vertices);
                        Completed.Triangles = MoveTemp(Built.Triangles);
                        Completed.Normals = MoveTemp(Built.Normals);
                        Self->PendingPublishes.Add(MoveTemp(Completed));
                    }
                });
        });
}

AProceduralVoxelChunkActor* UVoxelChunkMeshSubsystem::FindOrCreateChunk(FIntVector Coordinate, FVector WorldOrigin)
{
    if (TObjectPtr<AProceduralVoxelChunkActor>* Existing = ChunkActors.Find(Coordinate))
    {
        if (IsValid(Existing->Get()))
        {
            Existing->Get()->SetActorLocation(WorldOrigin);
            return Existing->Get();
        }
    }

    UWorld* World = GetWorld();
    if (!World) return nullptr;
    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = *FString::Printf(TEXT("VoxelChunk_%d_%d_%d"), Coordinate.X, Coordinate.Y, Coordinate.Z);
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AProceduralVoxelChunkActor* Actor = World->SpawnActor<AProceduralVoxelChunkActor>(
        AProceduralVoxelChunkActor::StaticClass(), WorldOrigin, FRotator::ZeroRotator, SpawnParams);
    if (Actor)
    {
        Actor->SetChunkCoordinate(Coordinate);
        ChunkActors.Add(Coordinate, Actor);
    }
    return Actor;
}

void UVoxelChunkMeshSubsystem::PublishCompleted(FCompletedMesh&& Mesh)
{
    if (Mesh.Triangles.IsEmpty() || Mesh.Vertices.IsEmpty())
    {
        if (TObjectPtr<AProceduralVoxelChunkActor>* Existing = ChunkActors.Find(Mesh.Coordinate))
        {
            if (IsValid(Existing->Get()))
            {
                Existing->Get()->Destroy();
            }
            ChunkActors.Remove(Mesh.Coordinate);
        }
        return;
    }

    AProceduralVoxelChunkActor* Actor = FindOrCreateChunk(Mesh.Coordinate, Mesh.WorldOrigin);
    if (!Actor) return;
    Actor->SetActorLocation(Mesh.WorldOrigin);
    Actor->ApplySurface(Mesh.Vertices, Mesh.Triangles, Mesh.Normals, UWorldRuntimeProfileSettings::Get().bGenerateCollision);
}

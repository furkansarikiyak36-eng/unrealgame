// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"

/**
 * Shared design contracts for the Landscape Builder, smooth-voxel runtime,
 * scanning, placement, shoring, survival and persistence systems.
 *
 * These are data/API contracts, not implementations. Keep gameplay ownership
 * outside this header so individual backends can be replaced by quality profile.
 */

enum class EWorldRegionKind : uint8
{
    EditableVoxel,
    StaticGeometry,
    Transition,
    Water,
    Unknown
};

enum class EWorldClassificationSource : uint8
{
    ManualOverride,
    ActorTag,
    LandscapeLayer,
    PhysicalMaterial,
    GeometrySampling,
    DefaultUnknown
};

struct MYPROJECT2_API FWorldRegionClassification
{
    FBox Bounds = FBox(ForceInit);
    EWorldRegionKind Kind = EWorldRegionKind::Unknown;
    EWorldClassificationSource Source = EWorldClassificationSource::DefaultUnknown;
    float Confidence = 0.0f;
    bool bExplicitDecision = false;
    FName SourceTag = NAME_None;
    FName LandscapeLayer = NAME_None;
    FString Reason;
};

struct MYPROJECT2_API FWorldClassificationBuildRequest
{
    FBox Bounds = FBox(ForceInit);
    FVector RegionSizeCm = FVector(800.0f);
    int32 MaxRegions = 100000;
    bool bIncludeGeometrySampling = true;
    bool bRespectManualOverrides = true;
};

struct MYPROJECT2_API FWorldClassificationBuildResult
{
    bool bSucceeded = false;
    TArray<FWorldRegionClassification> Regions;
    int32 UnknownRegionCount = 0;
    FString Error;
};

enum class EWorldVoxelEditShape : uint8
{
    Sphere,
    Capsule,
    Box
};

enum class EWorldVoxelEditOperation : uint8
{
    Carve,
    Fill
};

struct MYPROJECT2_API FWorldVoxelEditRequest
{
    FGuid OperationId = FGuid::NewGuid();
    EWorldVoxelEditShape Shape = EWorldVoxelEditShape::Sphere;
    EWorldVoxelEditOperation Operation = EWorldVoxelEditOperation::Carve;
    FVector CenterCm = FVector::ZeroVector;
    FVector EndCm = FVector::ZeroVector;
    FVector HalfExtentCm = FVector::ZeroVector;
    float RadiusCm = 0.0f;
    float TargetDensity = 0.0f;
    int32 MaxSamples = 100000;
};

struct MYPROJECT2_API FWorldDirtyChunkSet
{
    FGuid OperationId;
    TSet<FIntVector> Chunks;
    bool bIncludesNeighbors = false;

    void AddChunk(const FIntVector& ChunkCoordinate)
    {
        Chunks.Add(ChunkCoordinate);
    }
};

struct MYPROJECT2_API FWorldVoxelEditResult
{
    bool bSucceeded = false;
    FGuid OperationId;
    FWorldDirtyChunkSet DirtyChunks;
    int32 ModifiedSampleCount = 0;
    FString Error;
};

struct MYPROJECT2_API FWorldDensityChunkSnapshot
{
    FIntVector ChunkCoordinate = FIntVector::ZeroValue;
    int32 CellsPerChunkAxis = 0;
    int32 BorderCells = 1;
    float SampleSpacingCm = 0.0f;
    TArray<float> DensitySamples;
    TArray<int32> MaterialIds;

    bool IsShapeMetadataValid() const
    {
        return CellsPerChunkAxis > 0
            && BorderCells >= 0
            && FMath::IsFinite(SampleSpacingCm)
            && SampleSpacingCm > 0.0f;
    }
};

/** CPU-side mesh data prepared from a density snapshot, before UObject publication. */
struct MYPROJECT2_API FWorldVoxelMeshData
{
    FIntVector ChunkCoordinate = FIntVector::ZeroValue;
    TArray<FVector> Vertices;
    TArray<FVector> Normals;
    TArray<FVector2D> UV0;
    TArray<int32> TriangleIndices;
    /** Optional material ID per triangle; empty means a single section/material. */
    TArray<int32> TriangleMaterialIds;
    uint64 SourceRevision = 0;

    bool IsTopologyValid() const
    {
        return Vertices.Num() == Normals.Num()
            && (UV0.IsEmpty() || UV0.Num() == Vertices.Num())
            && (TriangleIndices.Num() % 3) == 0;
    }
};

struct MYPROJECT2_API FWorldScanQuery
{
    UWorld* World = nullptr;
    FVector StartCm = FVector::ZeroVector;
    FVector EndCm = FVector::ZeroVector;
    const AActor* ActorToIgnore = nullptr;
    ECollisionChannel Channel = ECC_Visibility;
    bool bTraceComplex = false;
    bool bReturnPhysicalMaterial = false;
};

struct MYPROJECT2_API FWorldScanHit
{
    bool bHit = false;
    FHitResult Hit;
    EWorldRegionKind RegionKind = EWorldRegionKind::Unknown;
    FName ObjectClass = NAME_None;
    bool bCanSupport = false;
    bool bCanSnap = false;
    bool bCanDig = false;
    bool bCanHarvest = false;
    bool bCanCarry = false;
    bool bCanInteract = false;
};

struct MYPROJECT2_API FWorldSurfaceSampleRequest
{
    FVector CenterCm = FVector::ZeroVector;
    FVector2D HalfExtentCm = FVector2D(50.0f, 50.0f);
    int32 SamplesX = 3;
    int32 SamplesY = 3;
    float TraceUpCm = 200.0f;
    float TraceDownCm = 400.0f;
    int32 MaxSamples = 25;
};

struct MYPROJECT2_API FWorldSurfaceSampleResult
{
    bool bSucceeded = false;
    TArray<FVector> Points;
    TArray<FVector> Normals;
    float MinHeightCm = 0.0f;
    float MaxHeightCm = 0.0f;
    float MaxSlopeDegrees = 0.0f;
    FString Error;
};

struct MYPROJECT2_API FWorldSnapCandidate
{
    FName Id = NAME_None;
    FName Group = NAME_None;
    FVector PositionCm = FVector::ZeroVector;
    FVector Normal = FVector::UpVector;
    FVector Axis = FVector::ForwardVector;
    float AxisDistanceCm = 0.0f;
    int32 SourceIndex = INDEX_NONE;
};

enum class EWorldPlacementFailure : uint8
{
    None,
    NoTarget,
    StaticRegion,
    NoCompatibleSnap,
    Occupied,
    Unsupported,
    StaleSurface,
    InvalidTransform,
    MissingResource,
    InternalError
};

struct MYPROJECT2_API FWorldPlacementQuery
{
    FVector HitPointCm = FVector::ZeroVector;
    FVector HitNormal = FVector::UpVector;
    FVector DesiredAxis = FVector::ForwardVector;
    FVector PieceHalfExtentCm = FVector(10.0f);
    FName RequiredGroup = NAME_None;
    float MaxSnapDistanceCm = 100.0f;
    uint64 ExpectedWorldRevision = 0;
};

struct MYPROJECT2_API FWorldPlacementResult
{
    bool bValid = false;
    EWorldPlacementFailure Failure = EWorldPlacementFailure::NoTarget;
    FTransform Transform = FTransform::Identity;
    FWorldSnapCandidate Candidate;
    uint64 ValidatedWorldRevision = 0;
    FString Message;
};

struct MYPROJECT2_API FWorldEnvironmentSample
{
    bool bValid = false;
    float AmbientTemperatureC = 15.0f;
    float Wetness01 = 0.0f;
    float WaterDepthCm = 0.0f;
    float LightLevel01 = 1.0f;
    bool bUnderRoof = false;
    bool bSubmerged = false;
    FName RegionId = NAME_None;
};

struct MYPROJECT2_API FWorldSerializedSection
{
    FName SectionId = NAME_None;
    int32 SchemaVersion = 1;
    TArray<uint8> Payload;
};

struct MYPROJECT2_API FWorldPersistencePayload
{
    int32 ContainerVersion = 1;
    TArray<FWorldSerializedSection> Sections;
};

/** Builds deterministic classifications for the requested world region. */
class MYPROJECT2_API IWorldClassificationSource
{
public:
    virtual ~IWorldClassificationSource() = default;
    virtual bool BuildClassification(
        const FWorldClassificationBuildRequest& Request,
        FWorldClassificationBuildResult& OutResult) const = 0;
    virtual bool QueryClassification(
        const FVector& WorldPositionCm,
        FWorldRegionClassification& OutResult) const = 0;
};

/** Owns base density sampling plus the final density after sparse edit deltas. */
class MYPROJECT2_API IVoxelDensitySource
{
public:
    virtual ~IVoxelDensitySource() = default;
    virtual bool SampleBaseDensityAtWorld(const FVector& PositionCm, float& OutDensity) const = 0;
    virtual bool SampleDensityAtWorld(const FVector& PositionCm, float& OutDensity) const = 0;
    virtual bool BuildChunkSnapshot(
        const FIntVector& ChunkCoordinate,
        FWorldDensityChunkSnapshot& OutSnapshot) const = 0;
};

/** Applies atomic carve/fill operations and identifies the chunks needing refresh. */
class MYPROJECT2_API IVoxelEditService
{
public:
    virtual ~IVoxelEditService() = default;
    virtual bool ApplyEdit(
        const FWorldVoxelEditRequest& Request,
        FWorldVoxelEditResult& OutResult) = 0;
    virtual bool RollbackEdit(const FGuid& OperationId, FWorldVoxelEditResult& OutResult) = 0;
};

/** Pure CPU-side surface extraction; it must not create or mutate UObjects on a worker thread. */
class MYPROJECT2_API IVoxelSurfaceMesher
{
public:
    virtual ~IVoxelSurfaceMesher() = default;
    virtual bool BuildMesh(
        const FWorldDensityChunkSnapshot& Input,
        FWorldVoxelMeshData& OutMesh,
        FString& OutError) const = 0;
};

/** Enqueues collision work only after a matching mesh revision has been prepared. */
class MYPROJECT2_API IVoxelCollisionUpdater
{
public:
    virtual ~IVoxelCollisionUpdater() = default;
    virtual void QueueCollisionUpdate(const FWorldVoxelMeshData& Mesh) = 0;
    virtual void CancelPendingUpdates(const FIntVector& ChunkCoordinate) = 0;
};

/** Unified, demand-driven query contract for interaction and environmental tools. */
class MYPROJECT2_API IWorldScanProvider
{
public:
    virtual ~IWorldScanProvider() = default;
    virtual bool Trace(const FWorldScanQuery& Query, FWorldScanHit& OutHit) const = 0;
    virtual bool SampleSurface(
        UWorld* World,
        const FWorldSurfaceSampleRequest& Request,
        const AActor* ActorToIgnore,
        FWorldSurfaceSampleResult& OutResult) const = 0;
};

/** Supplies generic snap anchors for logs, planks, stone pieces, and voxel surfaces. */
class MYPROJECT2_API IPlacementSurfaceProvider
{
public:
    virtual ~IPlacementSurfaceProvider() = default;
    virtual bool GatherCandidates(
        const FWorldPlacementQuery& Query,
        TArray<FWorldSnapCandidate>& OutCandidates) const = 0;
};

/** Connects placement/terrain invalidation to the project's authoritative support graph. */
class MYPROJECT2_API IStructureSupportAdapter
{
public:
    virtual ~IStructureSupportAdapter() = default;
    virtual bool ValidatePlacement(
        const FTransform& Transform,
        const FBox& WorldBounds,
        FString& OutReason) const = 0;
    virtual void OnWorldChunksChanged(const FWorldDirtyChunkSet& ChangedChunks) = 0;
};

/** Versioned sections let subsystems persist independently without owning a second save-game. */
class MYPROJECT2_API IWorldPersistenceAdapter
{
public:
    virtual ~IWorldPersistenceAdapter() = default;
    virtual bool ExportState(FWorldPersistencePayload& OutPayload) const = 0;
    virtual bool ImportState(const FWorldPersistencePayload& Payload, FString& OutError) = 0;
};

/** Reports local environment to survival systems without coupling them to voxel internals. */
class MYPROJECT2_API ISurvivalRegionProvider
{
public:
    virtual ~ISurvivalRegionProvider() = default;
    virtual bool SampleEnvironment(
        const FVector& WorldPositionCm,
        FWorldEnvironmentSample& OutSample) const = 0;
};

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"
#include "Engine/OverlapResult.h"

class AActor;
class UPrimitiveComponent;
class UWorld;

namespace WorldSystemContractValidation
{
    inline bool IsFiniteVector(const FVector& Value)
    {
        return FMath::IsFinite(Value.X)
            && FMath::IsFinite(Value.Y)
            && FMath::IsFinite(Value.Z);
    }

    inline bool IsPositiveVector(const FVector& Value)
    {
        return IsFiniteVector(Value) && Value.X > 0.0f && Value.Y > 0.0f && Value.Z > 0.0f;
    }

    inline bool IsFiniteVector2D(const FVector2D& Value)
    {
        return FMath::IsFinite(Value.X) && FMath::IsFinite(Value.Y);
    }
}

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
    /** Hard ceiling on extra vertical surface probes during one classification build. */
    int32 MaxGeometrySamples = 1024;
    bool bIncludeGeometrySampling = true;
    bool bRespectManualOverrides = true;

    bool Validate(FString& OutError) const
    {
        OutError.Reset();
        if (!Bounds.IsValid
            || !WorldSystemContractValidation::IsFiniteVector(Bounds.Min)
            || !WorldSystemContractValidation::IsFiniteVector(Bounds.Max))
        {
            OutError = TEXT("Classification bounds must be valid and finite.");
            return false;
        }
        if (!WorldSystemContractValidation::IsPositiveVector(RegionSizeCm))
        {
            OutError = TEXT("Classification region size must be finite and positive on every axis.");
            return false;
        }
        if (Bounds.Min.X > Bounds.Max.X || Bounds.Min.Y > Bounds.Max.Y || Bounds.Min.Z > Bounds.Max.Z)
        {
            OutError = TEXT("Classification bounds minimum must not exceed maximum.");
            return false;
        }
        const FVector BoundsSize = Bounds.GetSize();
        if (!WorldSystemContractValidation::IsPositiveVector(BoundsSize))
        {
            OutError = TEXT("Classification bounds must have finite, positive size on every axis.");
            return false;
        }
        if (MaxRegions < 1 || MaxRegions > 1000000)
        {
            OutError = TEXT("MaxRegions must be in [1, 1000000].");
            return false;
        }
        const double RatioX = static_cast<double>(BoundsSize.X) / RegionSizeCm.X;
        const double RatioY = static_cast<double>(BoundsSize.Y) / RegionSizeCm.Y;
        const double RatioZ = static_cast<double>(BoundsSize.Z) / RegionSizeCm.Z;
        if (!FMath::IsFinite(RatioX) || !FMath::IsFinite(RatioY) || !FMath::IsFinite(RatioZ)
            || RatioX <= 0.0 || RatioY <= 0.0 || RatioZ <= 0.0
            || RatioX > MaxRegions || RatioY > MaxRegions || RatioZ > MaxRegions)
        {
            OutError = TEXT("Classification region grid exceeds MaxRegions or overflows numeric limits.");
            return false;
        }
        const auto SafeCeil = [](double Ratio) -> int64
        {
            const int64 Truncated = static_cast<int64>(Ratio);
            return Truncated + (static_cast<double>(Truncated) < Ratio ? 1 : 0);
        };
        const int64 CountX = SafeCeil(RatioX);
        const int64 CountY = SafeCeil(RatioY);
        const int64 CountZ = SafeCeil(RatioZ);
        if ((CountX * CountY * CountZ) > MaxRegions)
        {
            OutError = TEXT("Classification region grid exceeds MaxRegions.");
            return false;
        }
        if (MaxGeometrySamples < 0 || MaxGeometrySamples > 50000)
        {
            OutError = TEXT("MaxGeometrySamples must be in [0, 50000].");
            return false;
        }
        if (bIncludeGeometrySampling && MaxGeometrySamples < 1)
        {
            OutError = TEXT("Geometry sampling is enabled but MaxGeometrySamples is zero.");
            return false;
        }
        return true;
    }
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

    bool Validate(FString& OutError) const
    {
        OutError.Reset();
        if (!OperationId.IsValid())
        {
            OutError = TEXT("Voxel edit request requires a valid operation ID.");
            return false;
        }
        if (!WorldSystemContractValidation::IsFiniteVector(CenterCm)
            || !WorldSystemContractValidation::IsFiniteVector(EndCm)
            || !WorldSystemContractValidation::IsFiniteVector(HalfExtentCm)
            || !FMath::IsFinite(TargetDensity))
        {
            OutError = TEXT("Voxel edit geometry and target density must be finite.");
            return false;
        }
        if (MaxSamples < 1 || MaxSamples > 50000000)
        {
            OutError = TEXT("MaxSamples must be in [1, 50000000].");
            return false;
        }

        switch (Operation)
        {
        case EWorldVoxelEditOperation::Carve:
        case EWorldVoxelEditOperation::Fill:
            break;
        default:
            OutError = TEXT("Unsupported voxel edit operation.");
            return false;
        }

        switch (Shape)
        {
        case EWorldVoxelEditShape::Sphere:
        case EWorldVoxelEditShape::Capsule:
            if (!FMath::IsFinite(RadiusCm) || RadiusCm <= 0.0f)
            {
                OutError = TEXT("Sphere/capsule voxel edits require a finite positive radius.");
                return false;
            }
            break;
        case EWorldVoxelEditShape::Box:
            if (!WorldSystemContractValidation::IsPositiveVector(HalfExtentCm))
            {
                OutError = TEXT("Box voxel edits require finite positive half-extents.");
                return false;
            }
            break;
        default:
            OutError = TEXT("Unsupported voxel edit shape.");
            return false;
        }
        return true;
    }
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
        return CellsPerChunkAxis >= 1
            && CellsPerChunkAxis <= 128
            && BorderCells >= 0
            && BorderCells <= 8
            && FMath::IsFinite(SampleSpacingCm)
            && SampleSpacingCm > 0.0f;
    }

    /** Number of scalar density samples when each chunk includes both boundary planes and border cells. */
    int64 GetExpectedDensitySampleCount() const
    {
        if (!IsShapeMetadataValid())
        {
            return INDEX_NONE;
        }

        const int64 SamplesPerAxis = static_cast<int64>(CellsPerChunkAxis) + 1 + (2LL * BorderCells);
        return SamplesPerAxis * SamplesPerAxis * SamplesPerAxis;
    }

    bool HasValidSampleStorage() const
    {
        const int64 Expected = GetExpectedDensitySampleCount();
        return Expected > 0 && Expected <= MAX_int32 && DensitySamples.Num() == Expected;
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
        if (Vertices.Num() != Normals.Num()
            || (!UV0.IsEmpty() && UV0.Num() != Vertices.Num())
            || (TriangleIndices.Num() % 3) != 0
            || (!TriangleMaterialIds.IsEmpty()
                && TriangleMaterialIds.Num() != TriangleIndices.Num() / 3))
        {
            return false;
        }

        for (int32 Index : TriangleIndices)
        {
            if (Index < 0 || Index >= Vertices.Num())
            {
                return false;
            }
        }
        for (int32 MaterialId : TriangleMaterialIds)
        {
            if (MaterialId < 0)
            {
                return false;
            }
        }
        for (const FVector& Vertex : Vertices)
        {
            if (!WorldSystemContractValidation::IsFiniteVector(Vertex))
            {
                return false;
            }
        }
        for (const FVector& Normal : Normals)
        {
            if (!WorldSystemContractValidation::IsFiniteVector(Normal))
            {
                return false;
            }
        }
        for (const FVector2D& UV : UV0)
        {
            if (!WorldSystemContractValidation::IsFiniteVector2D(UV))
            {
                return false;
            }
        }
        return true;
    }
};

struct MYPROJECT2_API FWorldScanQuery
{
    UWorld* World = nullptr;
    FVector StartCm = FVector::ZeroVector;
    FVector EndCm = FVector::ZeroVector;
    const AActor* ActorToIgnore = nullptr;
    TArray<const UPrimitiveComponent*> ComponentsToIgnore;
    ECollisionChannel Channel = ECC_Visibility;
    bool bTraceComplex = false;
    bool bReturnPhysicalMaterial = false;

    bool Validate(FString& OutError) const
    {
        OutError.Reset();
        if (World == nullptr)
        {
            OutError = TEXT("World scan query requires a non-null UWorld.");
            return false;
        }
        if (!WorldSystemContractValidation::IsFiniteVector(StartCm)
            || !WorldSystemContractValidation::IsFiniteVector(EndCm)
            || StartCm.Equals(EndCm, KINDA_SMALL_NUMBER))
        {
            OutError = TEXT("World scan start/end must be finite and define a non-zero segment.");
            return false;
        }
        return true;
    }
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

enum class EWorldShapeTraceKind : uint8
{
    Sphere,
    Box,
    Capsule
};

struct MYPROJECT2_API FWorldShapeTraceRequest
{
    FVector StartCm = FVector::ZeroVector;
    FVector EndCm = FVector::ZeroVector;
    EWorldShapeTraceKind Shape = EWorldShapeTraceKind::Sphere;
    float RadiusCm = 25.0f;
    FVector HalfExtentCm = FVector(25.0f);
    float HalfHeightCm = 50.0f;
    bool bMultiHit = false;

    bool Validate(FString& OutError) const
    {
        OutError.Reset();
        if (!WorldSystemContractValidation::IsFiniteVector(StartCm)
            || !WorldSystemContractValidation::IsFiniteVector(EndCm))
        {
            OutError = TEXT("Shape trace start/end must be finite.");
            return false;
        }

        switch (Shape)
        {
        case EWorldShapeTraceKind::Sphere:
            if (!FMath::IsFinite(RadiusCm) || RadiusCm <= 0.0f)
            {
                OutError = TEXT("Sphere trace requires a finite positive radius.");
                return false;
            }
            break;
        case EWorldShapeTraceKind::Box:
            if (!WorldSystemContractValidation::IsPositiveVector(HalfExtentCm))
            {
                OutError = TEXT("Box trace requires finite positive half-extents.");
                return false;
            }
            break;
        case EWorldShapeTraceKind::Capsule:
            if (!FMath::IsFinite(RadiusCm) || !FMath::IsFinite(HalfHeightCm)
                || RadiusCm <= 0.0f || HalfHeightCm < RadiusCm)
            {
                OutError = TEXT("Capsule trace requires positive radius and half-height at least the radius.");
                return false;
            }
            break;
        default:
            OutError = TEXT("Unsupported shape trace kind.");
            return false;
        }
        return true;
    }
};

struct MYPROJECT2_API FWorldShapeTraceResult
{
    bool bSucceeded = false;
    TArray<FHitResult> Hits;
};

enum class EWorldOverlapShape : uint8
{
    Sphere,
    OrientedBox
};

struct MYPROJECT2_API FWorldOverlapRequest
{
    EWorldOverlapShape Shape = EWorldOverlapShape::Sphere;
    FVector CenterCm = FVector::ZeroVector;
    float RadiusCm = 100.0f;
    FVector HalfExtentCm = FVector(100.0f);
    FQuat Rotation = FQuat::Identity;

    bool Validate(FString& OutError) const
    {
        OutError.Reset();
        if (!WorldSystemContractValidation::IsFiniteVector(CenterCm))
        {
            OutError = TEXT("Overlap center must be finite.");
            return false;
        }

        switch (Shape)
        {
        case EWorldOverlapShape::Sphere:
            if (!FMath::IsFinite(RadiusCm) || RadiusCm <= 0.0f)
            {
                OutError = TEXT("Sphere overlap requires a finite positive radius.");
                return false;
            }
            break;
        case EWorldOverlapShape::OrientedBox:
            if (!WorldSystemContractValidation::IsPositiveVector(HalfExtentCm))
            {
                OutError = TEXT("Box overlap requires finite positive half-extents.");
                return false;
            }
            const float RotationSizeSquared = Rotation.SizeSquared();
            if (!FMath::IsFinite(Rotation.X) || !FMath::IsFinite(Rotation.Y)
                || !FMath::IsFinite(Rotation.Z) || !FMath::IsFinite(Rotation.W)
                || !FMath::IsFinite(RotationSizeSquared)
                || !FMath::IsNearlyEqual(RotationSizeSquared, 1.0f, 0.01f))
            {
                OutError = TEXT("Box overlap rotation must be finite and normalized.");
                return false;
            }
            break;
        default:
            OutError = TEXT("Unsupported overlap shape.");
            return false;
        }
        return true;
    }
};

struct MYPROJECT2_API FWorldOverlapResult
{
    bool bSucceeded = false;
    TArray<FOverlapResult> Overlaps;
};

struct MYPROJECT2_API FWorldActorSnapshot
{
    bool bValid = false;
    FBox Bounds = FBox(ForceInit);
    FVector OriginCm = FVector::ZeroVector;
    FVector DimensionsCm = FVector::ZeroVector;
    FVector LinearVelocityCmPerSec = FVector::ZeroVector;
    FVector AngularVelocityRadiansPerSec = FVector::ZeroVector;
    bool bSimulatingPhysics = false;
    bool bRigidBodyAwake = false;
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

    bool Validate(FString& OutError) const
    {
        OutError.Reset();
        if (!WorldSystemContractValidation::IsFiniteVector(CenterCm)
            || !WorldSystemContractValidation::IsFiniteVector2D(HalfExtentCm)
            || HalfExtentCm.X <= 0.0f || HalfExtentCm.Y <= 0.0f)
        {
            OutError = TEXT("Surface sample center/extents must be finite and extents must be positive.");
            return false;
        }
        if (SamplesX < 1 || SamplesX > 32 || SamplesY < 1 || SamplesY > 32
            || MaxSamples < 1 || MaxSamples > 1024
            || static_cast<int64>(SamplesX) * static_cast<int64>(SamplesY) > MaxSamples)
        {
            OutError = TEXT("Surface sample dimensions must be in [1, 32], MaxSamples in [1, 1024], and the grid must fit within MaxSamples.");
            return false;
        }
        if (!FMath::IsFinite(TraceUpCm) || !FMath::IsFinite(TraceDownCm)
            || TraceUpCm <= 0.0f || TraceDownCm <= 0.0f)
        {
            OutError = TEXT("Surface trace distances must be finite and positive.");
            return false;
        }
        return true;
    }
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

    bool Validate(FString& OutError) const
    {
        OutError.Reset();
        if (!WorldSystemContractValidation::IsFiniteVector(HitPointCm)
            || !WorldSystemContractValidation::IsFiniteVector(HitNormal)
            || !WorldSystemContractValidation::IsFiniteVector(DesiredAxis)
            || !WorldSystemContractValidation::IsPositiveVector(PieceHalfExtentCm))
        {
            OutError = TEXT("Placement query vectors must be finite and piece half-extents positive.");
            return false;
        }
        if (HitNormal.IsNearlyZero() || DesiredAxis.IsNearlyZero())
        {
            OutError = TEXT("Placement query requires non-zero hit normal and desired axis.");
            return false;
        }
        if (!FMath::IsFinite(MaxSnapDistanceCm) || MaxSnapDistanceCm <= 0.0f)
        {
            OutError = TEXT("MaxSnapDistanceCm must be finite and positive.");
            return false;
        }
        return true;
    }
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
        FWorldClassificationBuildResult& OutResult) = 0;
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
    virtual bool ShapeTrace(
        UWorld* World,
        const FWorldShapeTraceRequest& Request,
        const AActor* ActorToIgnore,
        FWorldShapeTraceResult& OutResult,
        ECollisionChannel Channel = ECC_Visibility) const = 0;
    virtual bool QueryOverlaps(
        UWorld* World,
        const FWorldOverlapRequest& Request,
        const AActor* ActorToIgnore,
        FWorldOverlapResult& OutResult,
        ECollisionChannel Channel = ECC_Visibility) const = 0;
    virtual bool SampleSurface(
        UWorld* World,
        const FWorldSurfaceSampleRequest& Request,
        const AActor* ActorToIgnore,
        FWorldSurfaceSampleResult& OutResult) const = 0;
    virtual bool AnalyzeActor(const AActor* Actor, FWorldActorSnapshot& OutSnapshot) const = 0;
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

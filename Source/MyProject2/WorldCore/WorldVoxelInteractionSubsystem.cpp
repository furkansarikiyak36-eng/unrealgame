// Copyright Epic Games, Inc. All Rights Reserved.

#include "WorldVoxelInteractionSubsystem.h"

#include "WorldCore/ExcavationVolumeSubsystem.h"
#include "WorldCore/ExcavationTerrainChunkActor.h"
#include "WorldCore/ExcavationTerrainChunkSubsystem.h"
#include "WorldCore/WorldClassificationSubsystem.h"
#include "WorldCore/WorldRuntimeProfileSubsystem.h"
#include "WorldCore/WorldScanSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/PrimitiveComponent.h"
#include "Math/RotationMatrix.h"

#include <cmath>

namespace
{
    const FName VoxelSurfaceTag(TEXT("World.Object.VoxelSurface"));

    bool HasActorOrComponentTag(
        const AActor* Actor,
        const UPrimitiveComponent* Component,
        const FName Tag)
    {
        return (IsValid(Actor) && Actor->ActorHasTag(Tag))
            || (IsValid(Component) && Component->ComponentHasTag(Tag));
    }

    bool IsTerrainPlacementGroupAllowed(const FName Group)
    {
        return Group.IsNone()
            || Group == FName(TEXT("VoxelSurface"))
            || Group == FName(TEXT("Terrain"))
            || Group == FName(TEXT("Ground"))
            || Group == FName(TEXT("Support"));
    }

    FBox GetEditBounds(const FWorldVoxelEditRequest& Request)
    {
        switch (Request.Shape)
        {
        case EWorldVoxelEditShape::Sphere:
            return FBox(
                Request.CenterCm - FVector(Request.RadiusCm),
                Request.CenterCm + FVector(Request.RadiusCm));
        case EWorldVoxelEditShape::Capsule:
            return FBox(
                Request.CenterCm.ComponentMin(Request.EndCm) - FVector(Request.RadiusCm),
                Request.CenterCm.ComponentMax(Request.EndCm) + FVector(Request.RadiusCm));
        case EWorldVoxelEditShape::Box:
            return FBox(Request.CenterCm - Request.HalfExtentCm, Request.CenterCm + Request.HalfExtentCm);
        default:
            return FBox(ForceInit);
        }
    }

    bool EstimateEditCellBudget(
        const FBox& Bounds,
        float CellSizeCm,
        int32 MaxSamples,
        int64& OutEstimatedCells)
    {
        OutEstimatedCells = 0;
        if (!Bounds.IsValid || !FMath::IsFinite(CellSizeCm) || CellSizeCm < 1.0f || MaxSamples < 1)
        {
            return false;
        }

        const FVector Size = Bounds.GetSize();
        const double CountX = std::ceil(static_cast<double>(Size.X) / CellSizeCm) + 2.0;
        const double CountY = std::ceil(static_cast<double>(Size.Y) / CellSizeCm) + 2.0;
        const double CountZ = std::ceil(static_cast<double>(Size.Z) / CellSizeCm) + 2.0;
        if (!FMath::IsFinite(CountX) || !FMath::IsFinite(CountY) || !FMath::IsFinite(CountZ)
            || CountX < 1.0 || CountY < 1.0 || CountZ < 1.0
            || CountX > MaxSamples || CountY > MaxSamples || CountZ > MaxSamples)
        {
            return false;
        }

        const double Product = CountX * CountY * CountZ;
        if (!FMath::IsFinite(Product) || Product > static_cast<double>(MaxSamples))
        {
            return false;
        }

        OutEstimatedCells = static_cast<int64>(Product);
        return true;
    }
}

void UWorldVoxelInteractionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UWorldScanSubsystem>();
    Collection.InitializeDependency<UWorldClassificationSubsystem>();
    Collection.InitializeDependency<UWorldRuntimeProfileSubsystem>();
    Collection.InitializeDependency<UExcavationVolumeSubsystem>();
    EditReceipts.Reset();
    EditReceiptOrder.Reset();
}

void UWorldVoxelInteractionSubsystem::Deinitialize()
{
    EditReceipts.Reset();
    EditReceiptOrder.Reset();
    Super::Deinitialize();
}

bool UWorldVoxelInteractionSubsystem::IsVoxelSurfaceHit(const FHitResult& Hit)
{
    return HasActorOrComponentTag(Hit.GetActor(), Hit.GetComponent(), VoxelSurfaceTag);
}

bool UWorldVoxelInteractionSubsystem::IsCurrentVoxelSurfaceActor(
    const AActor* Actor,
    FString& OutError) const
{
    OutError.Reset();
    if (!IsValid(Actor) || !Actor->ActorHasTag(VoxelSurfaceTag))
    {
        OutError = TEXT("The target is not an explicitly tagged voxel surface.");
        return false;
    }

    UWorld* World = GetWorld();
    UExcavationTerrainChunkSubsystem* Renderer =
        World ? World->GetSubsystem<UExcavationTerrainChunkSubsystem>() : nullptr;
    const AExcavationTerrainChunkActor* Chunk = Cast<AExcavationTerrainChunkActor>(Actor);
    if (!Renderer || !Chunk || !Renderer->IsChunkMeshCurrent(Chunk))
    {
        OutError = TEXT("Voxel surface is being rebuilt or has no current published mesh/collision; retry after the chunk update.");
        return false;
    }
    return true;
}

bool UWorldVoxelInteractionSubsystem::TraceFromView(
    const FVector& OriginCm,
    const FVector& Direction,
    float ReachCm,
    const AActor* ActorToIgnore,
    FWorldScanHit& OutHit,
    FString& OutError) const
{
    OutHit = FWorldScanHit{};
    OutError.Reset();

    UWorld* World = GetWorld();
    const FVector SafeDirection = Direction.GetSafeNormal();
    if (!World
        || !WorldSystemContractValidation::IsFiniteVector(OriginCm)
        || !WorldSystemContractValidation::IsFiniteVector(Direction)
        || SafeDirection.IsNearlyZero()
        || !FMath::IsFinite(ReachCm)
        || ReachCm <= 0.0f
        || ReachCm > 100000.0f)
    {
        OutError = TEXT("View scan requires a valid world, finite non-zero direction and reach in (0, 100000] cm.");
        return false;
    }

    const UWorldScanSubsystem* Scanner = World->GetSubsystem<UWorldScanSubsystem>();
    if (!Scanner)
    {
        OutError = TEXT("The shared world scanner is not available.");
        return false;
    }

    FWorldScanQuery Query;
    Query.World = World;
    Query.StartCm = OriginCm;
    Query.EndCm = OriginCm + SafeDirection * ReachCm;
    Query.ActorToIgnore = ActorToIgnore;
    Query.Channel = ECC_Visibility;
    Query.bTraceComplex = false;
    Query.bReturnPhysicalMaterial = false;

    if (!Scanner->Trace(Query, OutHit) || !OutHit.bHit || !OutHit.Hit.GetActor())
    {
        OutError = TEXT("The view ray did not resolve a valid world target.");
        OutHit = FWorldScanHit{};
        return false;
    }
    return true;
}

bool UWorldVoxelInteractionSubsystem::ValidateVoxelSurfaceHit(
    const FHitResult& Hit,
    const AActor* ActorToIgnore,
    FString& OutError) const
{
    OutError.Reset();
    if (!IsVoxelSurfaceHit(Hit))
    {
        OutError = TEXT("The hit is not a voxel surface.");
        return false;
    }

    if (!IsCurrentVoxelSurfaceActor(Hit.GetActor(), OutError))
    {
        return false;
    }

    UWorld* World = GetWorld();
    const UWorldScanSubsystem* Scanner =
        World ? World->GetSubsystem<UWorldScanSubsystem>() : nullptr;
    const FVector Normal = Hit.ImpactNormal.GetSafeNormal();
    if (!Scanner || Normal.IsNearlyZero() || !WorldSystemContractValidation::IsFiniteVector(Hit.ImpactPoint))
    {
        OutError = TEXT("The voxel surface hit has no usable current scan or normal.");
        return false;
    }

    constexpr float RecheckDistanceCm = 40.0f;
    FWorldScanQuery Query;
    Query.World = World;
    Query.StartCm = Hit.ImpactPoint + Normal * RecheckDistanceCm;
    Query.EndCm = Hit.ImpactPoint - Normal * RecheckDistanceCm;
    Query.ActorToIgnore = ActorToIgnore;
    Query.Channel = ECC_Visibility;
    Query.bTraceComplex = false;

    FWorldScanHit CurrentHit;
    if (!Scanner->Trace(Query, CurrentHit)
        || !CurrentHit.bHit
        || CurrentHit.Hit.GetActor() != Hit.GetActor()
        || FVector::DistSquared(CurrentHit.Hit.ImpactPoint, Hit.ImpactPoint) > FMath::Square(10.0f))
    {
        OutError = TEXT("The previously hit voxel surface no longer matches the current scan.");
        return false;
    }

    if (CurrentHit.RegionKind != EWorldRegionKind::EditableVoxel
        || !CurrentHit.bCanSupport
        || !CurrentHit.bCanSnap)
    {
        OutError = TEXT("This voxel surface is not currently classified as a safe editable placement surface.");
        return false;
    }

    return IsCurrentVoxelSurfaceActor(CurrentHit.Hit.GetActor(), OutError);
}

bool UWorldVoxelInteractionSubsystem::DigFromView(
    const FVector& OriginCm,
    const FVector& Direction,
    float ReachCm,
    float RadiusCm,
    const AActor* ActorToIgnore,
    FWorldVoxelEditResult& OutResult,
    FString& OutError)
{
    OutResult = FWorldVoxelEditResult{};
    OutError.Reset();

    if (!FMath::IsFinite(RadiusCm) || RadiusCm < 1.0f || RadiusCm > 200.0f)
    {
        OutError = TEXT("Pickaxe carve radius must be in [1, 200] cm.");
        return false;
    }

    FWorldScanHit ScanHit;
    if (!TraceFromView(OriginCm, Direction, ReachCm, ActorToIgnore, ScanHit, OutError))
    {
        return false;
    }

    if (ScanHit.RegionKind != EWorldRegionKind::EditableVoxel || !ScanHit.bCanDig)
    {
        OutError = TEXT("Dig denied: the current scanner hit lacks EditableVoxel classification or World.Capability.Dig permission.");
        return false;
    }

    if (IsVoxelSurfaceHit(ScanHit.Hit)
        && !IsCurrentVoxelSurfaceActor(ScanHit.Hit.GetActor(), OutError))
    {
        return false;
    }

    const FVector Normal = ScanHit.Hit.ImpactNormal.GetSafeNormal();
    if (Normal.IsNearlyZero())
    {
        OutError = TEXT("Dig denied: the current hit has no stable surface normal.");
        return false;
    }

    FWorldVoxelEditRequest Request;
    Request.OperationId = FGuid::NewGuid();
    Request.Shape = EWorldVoxelEditShape::Sphere;
    Request.Operation = EWorldVoxelEditOperation::Carve;
    // Put most of the sphere into the solid half-space while retaining enough overlap
    // with the outer samples for the rendered surface to open at the scanned point.
    Request.CenterCm = ScanHit.Hit.ImpactPoint - Normal * (RadiusCm * 0.55f);
    Request.EndCm = Request.CenterCm;
    Request.RadiusCm = RadiusCm;
    Request.TargetDensity = 0.0f;
    Request.MaxSamples = 100000;

    if (UWorld* World = GetWorld())
    {
        if (const UWorldRuntimeProfileSubsystem* Profile =
            World->GetSubsystem<UWorldRuntimeProfileSubsystem>())
        {
            Request.MaxSamples = Profile->GetActiveSettings().MaxDensitySamplesPerEdit;
        }
    }

    if (!ApplyAuthorizedEdit(Request, OutResult, /*bHasCurrentScanAuthorization*/ true))
    {
        OutError = OutResult.Error;
        return false;
    }
    return true;
}

bool UWorldVoxelInteractionSubsystem::ApplyEdit(
    const FWorldVoxelEditRequest& Request,
    FWorldVoxelEditResult& OutResult)
{
    return ApplyAuthorizedEdit(Request, OutResult, /*bHasCurrentScanAuthorization*/ false);
}

bool UWorldVoxelInteractionSubsystem::ApplyAuthorizedEdit(
    const FWorldVoxelEditRequest& Request,
    FWorldVoxelEditResult& OutResult,
    bool bHasCurrentScanAuthorization)
{
    OutResult = FWorldVoxelEditResult{};
    FString ValidationError;
    if (!Request.Validate(ValidationError))
    {
        OutResult.Error = ValidationError;
        return false;
    }

    if (EditReceipts.Contains(Request.OperationId))
    {
        OutResult.Error = TEXT("Duplicate operation ID; the request will not be applied twice.");
        return false;
    }

    const float RequiredTargetDensity =
        Request.Operation == EWorldVoxelEditOperation::Carve ? 0.0f : 1.0f;
    if (!FMath::IsNearlyEqual(Request.TargetDensity, RequiredTargetDensity, KINDA_SMALL_NUMBER))
    {
        OutResult.Error = Request.Operation == EWorldVoxelEditOperation::Carve
            ? TEXT("Carve edits must target density 0.0.")
            : TEXT("Fill edits must target density 1.0.");
        return false;
    }

    UWorld* World = GetWorld();
    UExcavationVolumeSubsystem* Excavation =
        World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    UWorldClassificationSubsystem* Classification =
        World ? World->GetSubsystem<UWorldClassificationSubsystem>() : nullptr;
    if (!Excavation || !Classification)
    {
        OutResult.Error = TEXT("The authoritative density or classification subsystem is unavailable.");
        return false;
    }

    FWorldRegionClassification Region;
    const bool bHasRegion = Classification->QueryClassification(Request.CenterCm, Region);
    const bool bHasKnownRegion = bHasRegion && Region.Kind != EWorldRegionKind::Unknown;
    if (bHasKnownRegion && Region.Kind != EWorldRegionKind::EditableVoxel)
    {
        OutResult.Error = TEXT("Edit denied: the center lies in static, transition or water classified terrain.");
        return false;
    }
    if (!bHasKnownRegion && !bHasCurrentScanAuthorization)
    {
        OutResult.Error = TEXT("Edit denied: no current scanner authorization and no known EditableVoxel classification.");
        return false;
    }

    const FBox EditBounds = GetEditBounds(Request);
    const UWorldRuntimeProfileSubsystem* Profile = World->GetSubsystem<UWorldRuntimeProfileSubsystem>();
    const int32 ProfileMaxSamples = Profile
        ? FMath::Clamp(Profile->GetActiveSettings().MaxDensitySamplesPerEdit, 1, 50000000)
        : 100000;
    const int32 AllowedSamples = FMath::Min3(Request.MaxSamples, Excavation->MaxCellsPerEdit, ProfileMaxSamples);
    int64 EstimatedCells = 0;
    if (!EstimateEditCellBudget(EditBounds, Excavation->CellSizeCm, AllowedSamples, EstimatedCells))
    {
        OutResult.Error = FString::Printf(
            TEXT("Edit denied: estimated broad-phase cell count exceeds the request/profile budget (%d)."),
            AllowedSamples);
        return false;
    }

    const EExcavationEditType EditType = Request.Operation == EWorldVoxelEditOperation::Carve
        ? EExcavationEditType::Carve
        : EExcavationEditType::Fill;

    FExcavationEditRecord Receipt;
    bool bApplied = false;
    switch (Request.Shape)
    {
    case EWorldVoxelEditShape::Sphere:
        bApplied = Excavation->ApplySphereEdit(Request.CenterCm, Request.RadiusCm, EditType, Receipt);
        break;
    case EWorldVoxelEditShape::Capsule:
        bApplied = Excavation->ApplyCapsuleEdit(Request.CenterCm, Request.EndCm, Request.RadiusCm, EditType, Receipt);
        break;
    case EWorldVoxelEditShape::Box:
        bApplied = Excavation->ApplyBoxEdit(
            Request.CenterCm - Request.HalfExtentCm,
            Request.CenterCm + Request.HalfExtentCm,
            EditType,
            Receipt);
        break;
    default:
        OutResult.Error = TEXT("Unsupported voxel edit shape.");
        return false;
    }

    if (!bApplied || !Receipt.bCommitted || Receipt.Changes.IsEmpty())
    {
        OutResult.Error = TEXT("The authoritative density owner rejected the edit or no cells changed.");
        return false;
    }

    OutResult.bSucceeded = true;
    OutResult.OperationId = Request.OperationId;
    OutResult.ModifiedSampleCount = Receipt.Changes.Num();
    OutResult.DirtyChunks.OperationId = Request.OperationId;
    OutResult.DirtyChunks.bIncludesNeighbors = true;
    for (const FIntVector& Coordinate : Excavation->GetAffectedChunksForEdit(Receipt))
    {
        OutResult.DirtyChunks.AddChunk(Coordinate);
    }

    EditReceipts.Add(Request.OperationId, MoveTemp(Receipt));
    EditReceiptOrder.Add(Request.OperationId);
    constexpr int32 MaxRollbackReceipts = 512;
    while (EditReceiptOrder.Num() > MaxRollbackReceipts)
    {
        const FGuid Oldest = EditReceiptOrder[0];
        EditReceiptOrder.RemoveAt(0, 1, EAllowShrinking::No);
        EditReceipts.Remove(Oldest);
    }
    return true;
}

bool UWorldVoxelInteractionSubsystem::RollbackEdit(
    const FGuid& OperationId,
    FWorldVoxelEditResult& OutResult)
{
    OutResult = FWorldVoxelEditResult{};
    if (!OperationId.IsValid())
    {
        OutResult.Error = TEXT("Rollback requires a valid operation ID.");
        return false;
    }

    FExcavationEditRecord* FoundReceipt = EditReceipts.Find(OperationId);
    UWorld* World = GetWorld();
    UExcavationVolumeSubsystem* Excavation =
        World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!FoundReceipt || !Excavation)
    {
        OutResult.Error = TEXT("Rollback receipt is missing, expired or the density owner is unavailable.");
        return false;
    }

    const TArray<FIntVector> AffectedChunks = Excavation->GetAffectedChunksForEdit(*FoundReceipt);
    if (!Excavation->RollbackEdit(*FoundReceipt))
    {
        OutResult.Error = TEXT("Rollback rejected because a newer edit owns one or more cells.");
        return false;
    }

    OutResult.bSucceeded = true;
    OutResult.OperationId = OperationId;
    OutResult.ModifiedSampleCount = FoundReceipt->Changes.Num();
    OutResult.DirtyChunks.OperationId = OperationId;
    OutResult.DirtyChunks.bIncludesNeighbors = true;
    for (const FIntVector& Coordinate : AffectedChunks)
    {
        OutResult.DirtyChunks.AddChunk(Coordinate);
    }

    EditReceipts.Remove(OperationId);
    EditReceiptOrder.Remove(OperationId);
    return true;
}

bool UWorldVoxelInteractionSubsystem::GatherCandidates(
    const FWorldPlacementQuery& Query,
    TArray<FWorldSnapCandidate>& OutCandidates) const
{
    FString Error;
    return GatherCandidatesForActor(Query, nullptr, OutCandidates, Error);
}

bool UWorldVoxelInteractionSubsystem::GatherCandidatesForActor(
    const FWorldPlacementQuery& Query,
    const AActor* ActorToIgnore,
    TArray<FWorldSnapCandidate>& OutCandidates,
    FString& OutError) const
{
    OutCandidates.Reset();
    OutError.Reset();

    if (!Query.Validate(OutError))
    {
        return false;
    }
    if (!IsTerrainPlacementGroupAllowed(Query.RequiredGroup))
    {
        OutError = TEXT("The requested snap group is not compatible with a voxel terrain surface.");
        return false;
    }

    UWorld* World = GetWorld();
    const UExcavationVolumeSubsystem* Excavation =
        World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    const UWorldScanSubsystem* Scanner =
        World ? World->GetSubsystem<UWorldScanSubsystem>() : nullptr;
    if (!World || !Excavation || !Scanner)
    {
        OutError = TEXT("Placement requires world scan and authoritative voxel density services.");
        return false;
    }

    const uint64 CurrentRevision = Excavation->GetDensityRevision();
    if (Query.ExpectedWorldRevision != 0 && Query.ExpectedWorldRevision != CurrentRevision)
    {
        OutError = TEXT("Placement query is stale because the voxel density revision changed.");
        return false;
    }

    const FVector Normal = Query.HitNormal.GetSafeNormal();
    if (Normal.IsNearlyZero())
    {
        OutError = TEXT("Placement query normal is zero or invalid.");
        return false;
    }

    const float TraceDistance = FMath::Max(40.0f, Query.PieceHalfExtentCm.GetMax() + 20.0f);
    FWorldScanQuery ScanQuery;
    ScanQuery.World = World;
    ScanQuery.StartCm = Query.HitPointCm + Normal * TraceDistance;
    ScanQuery.EndCm = Query.HitPointCm - Normal * TraceDistance;
    ScanQuery.ActorToIgnore = ActorToIgnore;
    ScanQuery.Channel = ECC_Visibility;
    ScanQuery.bTraceComplex = false;

    FWorldScanHit Hit;
    if (!Scanner->Trace(ScanQuery, Hit) || !Hit.bHit || !Hit.Hit.GetActor())
    {
        OutError = TEXT("No current surface was found around the requested placement anchor.");
        return false;
    }
    if (!IsVoxelSurfaceHit(Hit.Hit))
    {
        OutError = TEXT("Placement target is not a generated voxel surface.");
        return false;
    }
    if (!IsCurrentVoxelSurfaceActor(Hit.Hit.GetActor(), OutError))
    {
        return false;
    }

    const float HitDistance = FVector::Distance(Hit.Hit.ImpactPoint, Query.HitPointCm);
    if (HitDistance > Query.MaxSnapDistanceCm)
    {
        OutError = TEXT("The current voxel surface lies outside the permitted snap distance.");
        return false;
    }
    if (Hit.RegionKind != EWorldRegionKind::EditableVoxel || !Hit.bCanSupport || !Hit.bCanSnap)
    {
        OutError = TEXT("Placement denied: the scanned voxel surface is not currently editable, stable and snappable.");
        return false;
    }
    if (Excavation->GetDensityRevision() != CurrentRevision)
    {
        OutError = TEXT("Placement query became stale during surface validation.");
        return false;
    }

    FWorldSnapCandidate Candidate;
    Candidate.Id = FName(TEXT("VoxelSurface.Direct"));
    Candidate.Group = FName(TEXT("VoxelSurface"));
    Candidate.PositionCm = Hit.Hit.ImpactPoint;
    Candidate.Normal = Hit.Hit.ImpactNormal.GetSafeNormal();
    Candidate.Axis = (Query.DesiredAxis - Candidate.Normal * FVector::DotProduct(Query.DesiredAxis, Candidate.Normal)).GetSafeNormal();
    if (Candidate.Axis.IsNearlyZero())
    {
        const FVector Fallback = FMath::Abs(Candidate.Normal.Z) < 0.9f
            ? FVector::UpVector
            : FVector::RightVector;
        Candidate.Axis = (Fallback - Candidate.Normal * FVector::DotProduct(Fallback, Candidate.Normal)).GetSafeNormal();
    }
    Candidate.AxisDistanceCm = HitDistance;
    Candidate.SourceIndex = 0;
    OutCandidates.Add(Candidate);
    return true;
}

bool UWorldVoxelInteractionSubsystem::HasPlacementOverlap(
    const FTransform& Transform,
    const FVector& PieceHalfExtentCm,
    const AActor* ActorToIgnore) const
{
    UWorld* World = GetWorld();
    if (!World || !WorldSystemContractValidation::IsPositiveVector(PieceHalfExtentCm))
    {
        return true;
    }

    FCollisionQueryParams Params(SCENE_QUERY_STAT(VoxelPlacementValidation), false, ActorToIgnore);
    if (ActorToIgnore)
    {
        Params.AddIgnoredActor(ActorToIgnore);
    }

    // Terrain is the support surface, not an obstructing object for a piece placed
    // against that surface. Other actors still participate in the overlap query.
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (IsValid(*It) && (*It)->ActorHasTag(VoxelSurfaceTag))
        {
            Params.AddIgnoredActor(*It);
        }
    }

    const FVector ShrunkenExtent = (PieceHalfExtentCm - FVector(2.0f)).ComponentMax(FVector(1.0f));
    return World->OverlapAnyTestByObjectType(
        Transform.GetLocation(),
        Transform.GetRotation(),
        FCollisionObjectQueryParams::AllObjects,
        FCollisionShape::MakeBox(ShrunkenExtent),
        Params);
}

bool UWorldVoxelInteractionSubsystem::BuildPlacementPreview(
    const FWorldPlacementQuery& Query,
    const AActor* ActorToIgnore,
    FWorldPlacementResult& OutResult) const
{
    OutResult = FWorldPlacementResult{};
    FString Error;
    if (!Query.Validate(Error))
    {
        OutResult.Failure = EWorldPlacementFailure::InvalidTransform;
        OutResult.Message = Error;
        return false;
    }

    TArray<FWorldSnapCandidate> Candidates;
    if (!GatherCandidatesForActor(Query, ActorToIgnore, Candidates, Error) || Candidates.IsEmpty())
    {
        OutResult.Failure = Error.Contains(TEXT("stale"), ESearchCase::IgnoreCase)
            || Error.Contains(TEXT("revision"), ESearchCase::IgnoreCase)
            ? EWorldPlacementFailure::StaleSurface
            : EWorldPlacementFailure::NoCompatibleSnap;
        OutResult.Message = Error.IsEmpty() ? TEXT("No compatible voxel surface candidate was found.") : Error;
        return false;
    }

    const FWorldSnapCandidate& Candidate = Candidates[0];
    const FVector Tangent = Candidate.Axis.GetSafeNormal();
    const FVector Normal = Candidate.Normal.GetSafeNormal();
    if (Tangent.IsNearlyZero() || Normal.IsNearlyZero())
    {
        OutResult.Failure = EWorldPlacementFailure::InvalidTransform;
        OutResult.Message = TEXT("Placement candidate has an invalid tangent or surface normal.");
        return false;
    }

    const FQuat Rotation = FRotationMatrix::MakeFromXZ(Tangent, Normal).ToQuat();
    const FVector Location = Candidate.PositionCm + Normal * Query.PieceHalfExtentCm.Z;
    const FTransform Transform(Rotation, Location, FVector::OneVector);
    if (!WorldSystemContractValidation::IsFiniteVector(Transform.GetLocation())
        || HasPlacementOverlap(Transform, Query.PieceHalfExtentCm, ActorToIgnore))
    {
        OutResult.Failure = EWorldPlacementFailure::Occupied;
        OutResult.Message = TEXT("Placement preview intersects another actor or has an invalid transform.");
        return false;
    }

    UWorld* World = GetWorld();
    const UExcavationVolumeSubsystem* Excavation =
        World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!Excavation)
    {
        OutResult.Failure = EWorldPlacementFailure::InternalError;
        OutResult.Message = TEXT("Voxel density owner is unavailable.");
        return false;
    }

    OutResult.bValid = true;
    OutResult.Failure = EWorldPlacementFailure::None;
    OutResult.Transform = Transform;
    OutResult.Candidate = Candidate;
    OutResult.ValidatedWorldRevision = Excavation->GetDensityRevision();
    return true;
}

bool UWorldVoxelInteractionSubsystem::ValidatePlacementCommit(
    const FWorldPlacementQuery& Query,
    const FWorldPlacementResult& Preview,
    const AActor* ActorToIgnore,
    FWorldPlacementResult& OutResult) const
{
    OutResult = FWorldPlacementResult{};
    if (!Preview.bValid || Preview.Failure != EWorldPlacementFailure::None)
    {
        OutResult.Failure = EWorldPlacementFailure::NoTarget;
        OutResult.Message = TEXT("Placement commit requires a previously valid preview.");
        return false;
    }

    UWorld* World = GetWorld();
    const UExcavationVolumeSubsystem* Excavation =
        World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!Excavation || Excavation->GetDensityRevision() != Preview.ValidatedWorldRevision)
    {
        OutResult.Failure = EWorldPlacementFailure::StaleSurface;
        OutResult.Message = TEXT("Placement preview is stale; the voxel density changed before commit.");
        return false;
    }

    FWorldPlacementQuery CommitQuery = Query;
    CommitQuery.ExpectedWorldRevision = Preview.ValidatedWorldRevision;
    if (!BuildPlacementPreview(CommitQuery, ActorToIgnore, OutResult))
    {
        return false;
    }

    const bool bSameLocation =
        FVector::DistSquared(OutResult.Transform.GetLocation(), Preview.Transform.GetLocation()) <= FMath::Square(2.0f);
    const bool bSameRotation =
        OutResult.Transform.GetRotation().AngularDistance(Preview.Transform.GetRotation()) <= FMath::DegreesToRadians(2.0f);
    const bool bSameSurface =
        OutResult.Candidate.Id == Preview.Candidate.Id
        && FVector::DistSquared(OutResult.Candidate.PositionCm, Preview.Candidate.PositionCm) <= FMath::Square(2.0f);
    if (!bSameLocation || !bSameRotation || !bSameSurface)
    {
        OutResult.bValid = false;
        OutResult.Failure = EWorldPlacementFailure::StaleSurface;
        OutResult.Message = TEXT("Placement target or transform changed since preview; no actor should be spawned and no resources consumed.");
        return false;
    }

    return true;
}

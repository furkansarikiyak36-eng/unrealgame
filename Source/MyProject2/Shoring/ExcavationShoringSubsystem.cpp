// Copyright Epic Games, Inc. All Rights Reserved.

#include "ExcavationShoringSubsystem.h"

#include "PlaceableShoringPiece.h"
#include "ShoringLayoutPlanner.h"
#include "WorldCore/ExcavationTerrainChunkActor.h"
#include "WorldCore/ExcavationVolumeSubsystem.h"
#include "WorldCore/WorldScanSubsystem.h"
#include "WorldCore/WorldVoxelInteractionSubsystem.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
    const FName VoxelSurfaceTag(TEXT("World.Object.VoxelSurface"));
    const FName ShoringPieceTag(TEXT("World.Object.ShoringPiece"));
    constexpr float AnchorProbeOffsetCm = 30.0f;
    constexpr float AnchorProbeReachCm = 60.0f;
    constexpr float CommitAnchorDeviationCm = 35.0f;
    constexpr float RevalidationAnchorDeviationCm = 12.0f;
    constexpr float BoundsConflictToleranceCm = 2.0f;

    bool IsFiniteVector(const FVector& Value)
    {
        return FMath::IsFinite(Value.X) && FMath::IsFinite(Value.Y) && FMath::IsFinite(Value.Z);
    }

    FBox MakeWorldBounds(const FTransform& Transform, const FVector& HalfExtentsCm)
    {
        const FQuat Rotation = Transform.GetRotation();
        const FVector Scale = Transform.GetScale3D();
        const FVector ScaledX = Rotation.RotateVector(FVector(HalfExtentsCm.X * Scale.X, 0.0f, 0.0f));
        const FVector ScaledY = Rotation.RotateVector(FVector(0.0f, HalfExtentsCm.Y * Scale.Y, 0.0f));
        const FVector ScaledZ = Rotation.RotateVector(FVector(0.0f, 0.0f, HalfExtentsCm.Z * Scale.Z));
        const FVector Extent(
            FMath::Abs(ScaledX.X) + FMath::Abs(ScaledY.X) + FMath::Abs(ScaledZ.X),
            FMath::Abs(ScaledX.Y) + FMath::Abs(ScaledY.Y) + FMath::Abs(ScaledZ.Y),
            FMath::Abs(ScaledX.Z) + FMath::Abs(ScaledY.Z) + FMath::Abs(ScaledZ.Z));
        return FBox(Transform.GetLocation() - Extent, Transform.GetLocation() + Extent);
    }

    bool HasPenetratingOverlap(const FBox& A, const FBox& B, float ToleranceCm)
    {
        if (!A.IsValid || !B.IsValid || !A.Intersect(B))
        {
            return false;
        }
        const FVector Overlap(
            FMath::Min(A.Max.X, B.Max.X) - FMath::Max(A.Min.X, B.Min.X),
            FMath::Min(A.Max.Y, B.Max.Y) - FMath::Max(A.Min.Y, B.Min.Y),
            FMath::Min(A.Max.Z, B.Max.Z) - FMath::Max(A.Min.Z, B.Min.Z));
        return Overlap.X > ToleranceCm && Overlap.Y > ToleranceCm && Overlap.Z > ToleranceCm;
    }

    bool IsVoxelSurfaceActor(const AActor* Actor)
    {
        return IsValid(Actor) && (Actor->ActorHasTag(VoxelSurfaceTag) || Actor->IsA<AExcavationTerrainChunkActor>());
    }
}

void UExcavationShoringSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    PiecesById.Reset();
    PlanToPieces.Reset();
    PreviewPiecesByPlan.Reset();
    CommittedPlanIds.Reset();

    if (UExcavationVolumeSubsystem* Volume = GetWorld() ? GetWorld()->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr)
    {
        Volume->OnVolumeChanged.AddDynamic(this, &UExcavationShoringSubsystem::HandleVolumeChanged);
        Volume->OnVolumeReset.AddDynamic(this, &UExcavationShoringSubsystem::HandleVolumeReset);
    }
}

void UExcavationShoringSubsystem::Deinitialize()
{
    if (UExcavationVolumeSubsystem* Volume = GetWorld() ? GetWorld()->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr)
    {
        Volume->OnVolumeChanged.RemoveDynamic(this, &UExcavationShoringSubsystem::HandleVolumeChanged);
        Volume->OnVolumeReset.RemoveDynamic(this, &UExcavationShoringSubsystem::HandleVolumeReset);
    }

    TArray<FGuid> PreviewIds;
    PreviewPiecesByPlan.GetKeys(PreviewIds);
    for (const FGuid& PreviewId : PreviewIds)
    {
        ClearPreview(PreviewId);
    }
    PiecesById.Reset();
    PlanToPieces.Reset();
    CommittedPlanIds.Reset();
    Super::Deinitialize();
}

bool UExcavationShoringSubsystem::BuildPlan(
    const FShoringLayoutRequest& Request,
    FShoringLayoutPlan& OutPlan,
    FString& OutError) const
{
    OutPlan = FShoringLayoutPlan{};
    OutError.Reset();

    const UWorld* World = GetWorld();
    const UExcavationVolumeSubsystem* Volume =
        World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!Volume)
    {
        OutError = TEXT("Shoring planning requires the authoritative excavation volume subsystem.");
        return false;
    }

    FShoringLayoutRequest BoundRequest = Request;
    const uint64 CurrentRevision = Volume->GetDensityRevision();
    if (BoundRequest.SourceDensityRevision != 0 && BoundRequest.SourceDensityRevision != CurrentRevision)
    {
        OutError = TEXT("The requested plan was prepared against a stale density revision; rebuild the preview.");
        return false;
    }
    BoundRequest.SourceDensityRevision = CurrentRevision;
    return FShoringLayoutPlanner::BuildPlan(BoundRequest, OutPlan, OutError);
}

bool UExcavationShoringSubsystem::PreviewPlan(
    const FShoringLayoutPlan& Plan,
    TArray<APlaceableShoringPiece*>& OutPreviewPieces,
    FString& OutError)
{
    OutPreviewPieces.Reset();
    OutError.Reset();

    FString ValidationError;
    if (!Plan.Validate(ValidationError))
    {
        OutError = ValidationError;
        return false;
    }

    const UExcavationVolumeSubsystem* Volume =
        GetWorld() ? GetWorld()->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!Volume || Plan.SourceDensityRevision == 0 || Volume->GetDensityRevision() != Plan.SourceDensityRevision)
    {
        OutError = TEXT("Shoring preview is stale; rebuild the plan against the current voxel revision.");
        return false;
    }
    if (CommittedPlanIds.Contains(Plan.PlanId))
    {
        OutError = TEXT("This plan is already committed; create a new preview to avoid duplicate piece GUIDs.");
        return false;
    }

    ClearPreview(Plan.PlanId);
    UWorld* World = GetWorld();
    if (!World)
    {
        OutError = TEXT("World is unavailable.");
        return false;
    }

    TArray<TWeakObjectPtr<APlaceableShoringPiece>> SpawnedPreviews;
    SpawnedPreviews.Reserve(Plan.Pieces.Num());
    for (const FShoringPieceSpec& Spec : Plan.Pieces)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        APlaceableShoringPiece* Preview = World->SpawnActor<APlaceableShoringPiece>(
            APlaceableShoringPiece::StaticClass(), Spec.Transform, SpawnParams);
        if (!Preview || !Preview->InitializeFromSpec(Spec, true))
        {
            if (Preview)
            {
                Preview->Destroy();
            }
            for (const TWeakObjectPtr<APlaceableShoringPiece>& Spawned : SpawnedPreviews)
            {
                if (Spawned.IsValid())
                {
                    Spawned->Destroy();
                }
            }
            OutError = TEXT("A shoring preview actor could not be created; the partial preview was cleared.");
            return false;
        }
        SpawnedPreviews.Add(Preview);
        OutPreviewPieces.Add(Preview);
    }

    PreviewPiecesByPlan.Add(Plan.PlanId, MoveTemp(SpawnedPreviews));
    return true;
}

void UExcavationShoringSubsystem::ClearPreview(const FGuid& PlanId)
{
    TArray<TWeakObjectPtr<APlaceableShoringPiece>> PreviewPieces;
    if (!PreviewPiecesByPlan.RemoveAndCopyValue(PlanId, PreviewPieces))
    {
        return;
    }
    for (const TWeakObjectPtr<APlaceableShoringPiece>& Preview : PreviewPieces)
    {
        if (Preview.IsValid())
        {
            Preview->Destroy();
        }
    }
}

bool UExcavationShoringSubsystem::CommitPlan(
    const FShoringLayoutPlan& Plan,
    TArray<APlaceableShoringPiece*>& OutPieces,
    FString& OutError,
    TSubclassOf<APlaceableShoringPiece> PieceClass)
{
    OutPieces.Reset();
    OutError.Reset();

    if (!ValidatePlanForCommit(Plan, OutError))
    {
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        OutError = TEXT("World is unavailable.");
        return false;
    }
    if (!PieceClass)
    {
        PieceClass = APlaceableShoringPiece::StaticClass();
    }

    // Final race guard: no actor or resource mutation occurs if density changed during validation.
    const UExcavationVolumeSubsystem* Volume = World->GetSubsystem<UExcavationVolumeSubsystem>();
    if (!Volume || Volume->GetDensityRevision() != Plan.SourceDensityRevision)
    {
        OutError = TEXT("The voxel revision changed before shoring commit; no pieces were spawned.");
        return false;
    }

    TArray<APlaceableShoringPiece*> Spawned;
    Spawned.Reserve(Plan.Pieces.Num());
    for (const FShoringPieceSpec& Spec : Plan.Pieces)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        APlaceableShoringPiece* Piece = World->SpawnActor<APlaceableShoringPiece>(
            PieceClass, Spec.Transform, SpawnParams);
        if (!Piece || !Piece->InitializeFromSpec(Spec, false))
        {
            if (Piece)
            {
                Piece->Destroy();
            }
            for (APlaceableShoringPiece* SpawnedPiece : Spawned)
            {
                if (IsValid(SpawnedPiece))
                {
                    SpawnedPiece->Destroy();
                }
            }
            OutError = TEXT("Shoring commit failed while creating actors; the partial structure was rolled back.");
            return false;
        }
        Spawned.Add(Piece);
    }

    ClearPreview(Plan.PlanId);
    TArray<FGuid> PieceIds;
    PieceIds.Reserve(Spawned.Num());
    for (APlaceableShoringPiece* Piece : Spawned)
    {
        PiecesById.Add(Piece->GetPieceId(), Piece);
        PieceIds.Add(Piece->GetPieceId());
        OutPieces.Add(Piece);
    }
    PlanToPieces.Add(Plan.PlanId, MoveTemp(PieceIds));
    CommittedPlanIds.Add(Plan.PlanId);
    return true;
}

bool UExcavationShoringSubsystem::ValidatePlanForCommit(
    const FShoringLayoutPlan& Plan,
    FString& OutError) const
{
    OutError.Reset();
    FString PlanError;
    if (!Plan.Validate(PlanError))
    {
        OutError = PlanError;
        return false;
    }

    const UWorld* World = GetWorld();
    const UExcavationVolumeSubsystem* Volume =
        World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!World || !Volume)
    {
        OutError = TEXT("Shoring commit requires a world and authoritative density owner.");
        return false;
    }
    if (Plan.SourceDensityRevision == 0 || Volume->GetDensityRevision() != Plan.SourceDensityRevision)
    {
        OutError = TEXT("Shoring plan revision is stale; no pieces were committed.");
        return false;
    }
    if (CommittedPlanIds.Contains(Plan.PlanId) || PlanToPieces.Contains(Plan.PlanId))
    {
        OutError = TEXT("This plan GUID is already committed.");
        return false;
    }

    for (const FShoringPieceSpec& Spec : Plan.Pieces)
    {
        if (Spec.SupportAnchor.Kind == EShoringAnchorKind::VoxelSurface)
        {
            FVector ResolvedPoint = FVector::ZeroVector;
            if (!ValidateSurfaceAnchor(
                    Spec.SupportAnchor,
                    Plan.SourceDensityRevision,
                    true,
                    CommitAnchorDeviationCm,
                    ResolvedPoint,
                    OutError))
            {
                OutError = FString::Printf(TEXT("Piece %s has an invalid voxel support anchor: %s"),
                    *Spec.PieceId.ToString(), *OutError);
                return false;
            }
        }
        else if (Spec.SupportAnchor.Kind == EShoringAnchorKind::PieceSocket)
        {
            if (!Spec.ConnectedPieceIds.Contains(Spec.SupportAnchor.ConnectedPieceId))
            {
                OutError = TEXT("Piece-socket anchor is not represented by the piece's explicit connection list.");
                return false;
            }
        }

        if (!ValidateWorldCollision(Spec.Transform, Spec.HalfExtentsCm, nullptr, OutError))
        {
            OutError = FString::Printf(TEXT("Piece %s cannot be placed: %s"), *Spec.PieceId.ToString(), *OutError);
            return false;
        }
        if (!HasExistingShoringConflict(Spec.Transform, Spec.HalfExtentsCm, FGuid(), nullptr, OutError))
        {
            return false;
        }
    }

    if (Volume->GetDensityRevision() != Plan.SourceDensityRevision)
    {
        OutError = TEXT("Voxel density changed during support-anchor validation; rebuild the shoring plan.");
        return false;
    }
    return true;
}

bool UExcavationShoringSubsystem::ValidateSurfaceAnchor(
    const FShoringSupportAnchor& Anchor,
    uint64 ExpectedRevision,
    bool bRequireSameRevision,
    float MaxDeviationCm,
    FVector& OutResolvedPositionCm,
    FString& OutError) const
{
    OutResolvedPositionCm = FVector::ZeroVector;
    OutError.Reset();

    if (Anchor.Kind != EShoringAnchorKind::VoxelSurface || !Anchor.IsFinite()
        || !FMath::IsFinite(MaxDeviationCm) || MaxDeviationCm <= 0.0f)
    {
        OutError = TEXT("A voxel-surface anchor must be finite and have a positive validation tolerance.");
        return false;
    }

    UWorld* World = GetWorld();
    const UExcavationVolumeSubsystem* Volume =
        World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    UWorldVoxelInteractionSubsystem* Interaction =
        World ? World->GetSubsystem<UWorldVoxelInteractionSubsystem>() : nullptr;
    if (!World || !Volume || !Interaction)
    {
        OutError = TEXT("Scanner, density owner or world is unavailable.");
        return false;
    }

    const uint64 CurrentRevision = Volume->GetDensityRevision();
    if (bRequireSameRevision
        && (ExpectedRevision != CurrentRevision || Anchor.SourceDensityRevision != ExpectedRevision))
    {
        OutError = TEXT("Support anchor was prepared against an out-of-date voxel revision.");
        return false;
    }

    const FVector Direction = -Anchor.Normal.GetSafeNormal();
    const FVector Start = Anchor.PositionCm + Anchor.Normal.GetSafeNormal() * AnchorProbeOffsetCm;
    FWorldScanHit ScanHit;
    if (!Interaction->TraceFromView(
            Start,
            Direction,
            AnchorProbeReachCm,
            nullptr,
            ScanHit,
            OutError))
    {
        OutError = OutError.IsEmpty() ? TEXT("No voxel surface was found at the planned support anchor.") : OutError;
        return false;
    }

    if (!UWorldVoxelInteractionSubsystem::IsVoxelSurfaceHit(ScanHit.Hit)
        || ScanHit.RegionKind != EWorldRegionKind::EditableVoxel
        || !ScanHit.bCanSupport
        || FVector::Dist(ScanHit.Hit.ImpactPoint, Anchor.PositionCm) > MaxDeviationCm)
    {
        OutError = TEXT("Planned support anchor did not resolve to a nearby, supportable EditableVoxel surface.");
        return false;
    }

    FString SurfaceError;
    if (!Interaction->ValidateVoxelSurfaceHit(ScanHit.Hit, nullptr, SurfaceError))
    {
        OutError = FString::Printf(TEXT("Voxel surface is not current: %s"), *SurfaceError);
        return false;
    }
    const FVector HitNormal = ScanHit.Hit.ImpactNormal.GetSafeNormal();
    if (HitNormal.IsNearlyZero() || FVector::DotProduct(HitNormal, Anchor.Normal.GetSafeNormal()) < 0.25f)
    {
        OutError = TEXT("Current voxel surface normal no longer faces the planned shoring anchor.");
        return false;
    }

    if (Volume->GetDensityRevision() != CurrentRevision)
    {
        OutError = TEXT("Voxel density changed during support-anchor validation.");
        return false;
    }

    OutResolvedPositionCm = ScanHit.Hit.ImpactPoint;
    return true;
}

bool UExcavationShoringSubsystem::ValidateWorldCollision(
    const FTransform& Transform,
    const FVector& HalfExtentsCm,
    const AActor* ActorToIgnore,
    FString& OutError) const
{
    OutError.Reset();
    if (!GetWorld() || !IsFiniteVector(Transform.GetLocation())
        || !IsFiniteVector(HalfExtentsCm)
        || HalfExtentsCm.X <= 0.0f || HalfExtentsCm.Y <= 0.0f || HalfExtentsCm.Z <= 0.0f)
    {
        OutError = TEXT("Placement transform or half-extents are invalid.");
        return false;
    }

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ShoringPiecePlacement), false, ActorToIgnore);
    if (ActorToIgnore)
    {
        QueryParams.AddIgnoredActor(ActorToIgnore);
    }

    TArray<FOverlapResult> Overlaps;
    const bool bHasOverlap = GetWorld()->OverlapMultiByObjectType(
        Overlaps,
        Transform.GetLocation(),
        Transform.GetRotation(),
        FCollisionObjectQueryParams::AllObjects,
        FCollisionShape::MakeBox(HalfExtentsCm),
        QueryParams);
    if (!bHasOverlap)
    {
        return true;
    }

    for (const FOverlapResult& Overlap : Overlaps)
    {
        const AActor* OtherActor = Overlap.GetActor();
        if (!IsValid(OtherActor) || OtherActor == ActorToIgnore
            || IsVoxelSurfaceActor(OtherActor)
            || OtherActor->IsA<APlaceableShoringPiece>()
            || OtherActor->ActorHasTag(ShoringPieceTag))
        {
            continue;
        }
        OutError = FString::Printf(
            TEXT("Proposed shoring geometry overlaps actor '%s'."),
            *OtherActor->GetName());
        return false;
    }
    return true;
}

bool UExcavationShoringSubsystem::HasExistingShoringConflict(
    const FTransform& Transform,
    const FVector& HalfExtentsCm,
    const FGuid& PieceToIgnore,
    const TArray<FGuid>* AllowedConnectedPieceIds,
    FString& OutError) const
{
    OutError.Reset();
    const FBox CandidateBounds = MakeWorldBounds(Transform, HalfExtentsCm);
    if (!CandidateBounds.IsValid)
    {
        OutError = TEXT("Could not calculate shoring piece world bounds.");
        return false;
    }

    for (const TPair<FGuid, TWeakObjectPtr<APlaceableShoringPiece>>& Pair : PiecesById)
    {
        APlaceableShoringPiece* ExistingPiece = Pair.Value.Get();
        if (!IsValid(ExistingPiece) || Pair.Key == PieceToIgnore
            || ExistingPiece->GetPieceState() == EShoringPieceState::Removed
            || (AllowedConnectedPieceIds && AllowedConnectedPieceIds->Contains(Pair.Key)))
        {
            continue;
        }

        const FBox ExistingBounds = ExistingPiece->GetComponentsBoundingBox(true);
        if (HasPenetratingOverlap(CandidateBounds, ExistingBounds, BoundsConflictToleranceCm))
        {
            OutError = FString::Printf(
                TEXT("Shoring piece conflicts with existing piece %s."),
                *ExistingPiece->GetPieceId().ToString());
            return false;
        }
    }
    return true;
}

bool UExcavationShoringSubsystem::MovePiece(
    const FGuid& PieceId,
    const FTransform& NewTransform,
    const FShoringSupportAnchor& NewAnchor,
    FString& OutError)
{
    OutError.Reset();
    APlaceableShoringPiece* Piece = FindPiece(PieceId);
    if (!IsValid(Piece) || Piece->GetPieceState() == EShoringPieceState::Removed)
    {
        OutError = TEXT("Shoring piece does not exist.");
        return false;
    }
    if (!IsFiniteVector(NewTransform.GetLocation()) || !NewAnchor.IsFinite())
    {
        OutError = TEXT("Move request contains an invalid transform or support anchor.");
        return false;
    }

    const UExcavationVolumeSubsystem* Volume =
        GetWorld() ? GetWorld()->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!Volume || NewAnchor.SourceDensityRevision != Volume->GetDensityRevision())
    {
        OutError = TEXT("Move request is stale; rebuild the anchor against the current voxel revision.");
        return false;
    }

    if (NewAnchor.Kind == EShoringAnchorKind::VoxelSurface)
    {
        FVector ResolvedPoint = FVector::ZeroVector;
        if (!ValidateSurfaceAnchor(
                NewAnchor,
                Volume->GetDensityRevision(),
                true,
                CommitAnchorDeviationCm,
                ResolvedPoint,
                OutError))
        {
            return false;
        }
    }
    else if (NewAnchor.Kind == EShoringAnchorKind::PieceSocket
        && (!PiecesById.Contains(NewAnchor.ConnectedPieceId)
            || !Piece->GetConnectedPieceIds().Contains(NewAnchor.ConnectedPieceId)))
    {
        OutError = TEXT("Move request must reference an existing socket that is already connected to this piece.");
        return false;
    }

    if (!ValidateWorldCollision(NewTransform, Piece->GetHalfExtentsCm(), Piece, OutError)
        || !HasExistingShoringConflict(
            NewTransform, Piece->GetHalfExtentsCm(), PieceId, &Piece->GetConnectedPieceIds(), OutError))
    {
        return false;
    }

    Piece->SetActorTransform(NewTransform);
    Piece->UpdateSupportAnchor(NewAnchor);
    SetPlanState(Piece->GetPlanId(), EShoringPieceState::NeedsRevalidation);
    return true;
}

bool UExcavationShoringSubsystem::RemovePiece(const FGuid& PieceId, FString& OutError)
{
    OutError.Reset();
    APlaceableShoringPiece* Piece = FindPiece(PieceId);
    if (!IsValid(Piece))
    {
        OutError = TEXT("Shoring piece does not exist.");
        return false;
    }

    const FGuid PlanId = Piece->GetPlanId();
    PiecesById.Remove(PieceId);
    if (TArray<FGuid>* PlanPieceIds = PlanToPieces.Find(PlanId))
    {
        PlanPieceIds->Remove(PieceId);
    }
    Piece->SetPieceState(EShoringPieceState::Removed);
    Piece->Destroy();

    for (TPair<FGuid, TWeakObjectPtr<APlaceableShoringPiece>>& Pair : PiecesById)
    {
        APlaceableShoringPiece* OtherPiece = Pair.Value.Get();
        if (!IsValid(OtherPiece) || !OtherPiece->GetConnectedPieceIds().Contains(PieceId))
        {
            continue;
        }
        TArray<FGuid> RemainingLinks = OtherPiece->GetConnectedPieceIds();
        RemainingLinks.Remove(PieceId);
        OtherPiece->UpdateConnections(RemainingLinks);
        OtherPiece->SetPieceState(EShoringPieceState::NeedsRevalidation);
    }

    if (const TArray<FGuid>* Remaining = PlanToPieces.Find(PlanId); !Remaining || Remaining->IsEmpty())
    {
        PlanToPieces.Remove(PlanId);
        CommittedPlanIds.Remove(PlanId);
    }
    else
    {
        SetPlanState(PlanId, EShoringPieceState::NeedsRevalidation);
    }
    return true;
}

bool UExcavationShoringSubsystem::RemovePlan(const FGuid& PlanId, FString& OutError)
{
    OutError.Reset();
    TArray<FGuid> PieceIds;
    if (const TArray<FGuid>* Found = PlanToPieces.Find(PlanId))
    {
        PieceIds = *Found;
    }
    if (PieceIds.IsEmpty())
    {
        OutError = TEXT("No committed shoring plan matches the supplied GUID.");
        return false;
    }

    for (const FGuid& PieceId : PieceIds)
    {
        if (APlaceableShoringPiece* Piece = FindPiece(PieceId))
        {
            Piece->SetPieceState(EShoringPieceState::Removed);
            Piece->Destroy();
        }
        PiecesById.Remove(PieceId);
    }
    PlanToPieces.Remove(PlanId);
    CommittedPlanIds.Remove(PlanId);

    for (TPair<FGuid, TWeakObjectPtr<APlaceableShoringPiece>>& Pair : PiecesById)
    {
        APlaceableShoringPiece* OtherPiece = Pair.Value.Get();
        if (!IsValid(OtherPiece))
        {
            continue;
        }
        bool bLostConnection = false;
        TArray<FGuid> RemainingLinks;
        for (const FGuid& ConnectedId : OtherPiece->GetConnectedPieceIds())
        {
            if (PieceIds.Contains(ConnectedId))
            {
                bLostConnection = true;
            }
            else
            {
                RemainingLinks.Add(ConnectedId);
            }
        }
        if (bLostConnection)
        {
            OtherPiece->UpdateConnections(RemainingLinks);
            OtherPiece->SetPieceState(EShoringPieceState::NeedsRevalidation);
        }
    }
    return true;
}

bool UExcavationShoringSubsystem::RevalidatePlan(const FGuid& PlanId, FString& OutError)
{
    OutError.Reset();
    const TArray<FGuid>* PieceIds = PlanToPieces.Find(PlanId);
    const UExcavationVolumeSubsystem* Volume =
        GetWorld() ? GetWorld()->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!PieceIds || PieceIds->IsEmpty() || !Volume)
    {
        OutError = TEXT("Revalidation requires an existing plan and the density owner.");
        return false;
    }

    const uint64 CurrentRevision = Volume->GetDensityRevision();
    for (const FGuid& PieceId : *PieceIds)
    {
        APlaceableShoringPiece* Piece = FindPiece(PieceId);
        if (!IsValid(Piece))
        {
            OutError = TEXT("A registered piece is no longer valid; plan cannot be revalidated.");
            SetPlanState(PlanId, EShoringPieceState::Invalid);
            return false;
        }
        const FShoringSupportAnchor& Anchor = Piece->GetSupportAnchor();
        if (Anchor.Kind == EShoringAnchorKind::VoxelSurface)
        {
            FVector ResolvedPoint = FVector::ZeroVector;
            FString AnchorError;
            if (!ValidateSurfaceAnchor(
                    Anchor,
                    CurrentRevision,
                    false,
                    RevalidationAnchorDeviationCm,
                    ResolvedPoint,
                    AnchorError))
            {
                OutError = FString::Printf(TEXT("Plan support was lost at piece %s: %s"),
                    *PieceId.ToString(), *AnchorError);
                SetPlanState(PlanId, EShoringPieceState::Invalid);
                return false;
            }
            FShoringSupportAnchor UpdatedAnchor = Anchor;
            UpdatedAnchor.SourceDensityRevision = CurrentRevision;
            UpdatedAnchor.PositionCm = ResolvedPoint;
            Piece->UpdateSupportAnchor(UpdatedAnchor);
        }
        else if (Anchor.Kind == EShoringAnchorKind::PieceSocket)
        {
            if (!IsValid(FindPiece(Anchor.ConnectedPieceId)))
            {
                OutError = FString::Printf(TEXT("Plan contains a missing primary socket anchor at piece %s."),
                    *PieceId.ToString());
                SetPlanState(PlanId, EShoringPieceState::Invalid);
                return false;
            }
            for (const FGuid& ConnectedId : Piece->GetConnectedPieceIds())
            {
                if (!IsValid(FindPiece(ConnectedId)))
                {
                    OutError = FString::Printf(TEXT("Plan contains a missing socket connection at piece %s."),
                        *PieceId.ToString());
                    SetPlanState(PlanId, EShoringPieceState::Invalid);
                    return false;
                }
            }
        }
    }

    if (Volume->GetDensityRevision() != CurrentRevision)
    {
        OutError = TEXT("Voxel density changed during shoring revalidation; retry against the new revision.");
        SetPlanState(PlanId, EShoringPieceState::NeedsRevalidation);
        return false;
    }

    for (const FGuid& PieceId : *PieceIds)
    {
        if (APlaceableShoringPiece* Piece = FindPiece(PieceId))
        {
            Piece->SetPieceState(EShoringPieceState::Active);
        }
    }
    return true;
}

APlaceableShoringPiece* UExcavationShoringSubsystem::FindPiece(const FGuid& PieceId) const
{
    const TWeakObjectPtr<APlaceableShoringPiece>* Found = PiecesById.Find(PieceId);
    return Found ? Found->Get() : nullptr;
}

TArray<APlaceableShoringPiece*> UExcavationShoringSubsystem::GetPlanPieces(const FGuid& PlanId) const
{
    TArray<APlaceableShoringPiece*> Result;
    if (const TArray<FGuid>* PieceIds = PlanToPieces.Find(PlanId))
    {
        Result.Reserve(PieceIds->Num());
        for (const FGuid& PieceId : *PieceIds)
        {
            if (APlaceableShoringPiece* Piece = FindPiece(PieceId))
            {
                Result.Add(Piece);
            }
        }
    }
    return Result;
}

bool UExcavationShoringSubsystem::ValidatePlacement(
    const FTransform& Transform,
    const FBox& WorldBounds,
    FString& OutReason) const
{
    OutReason.Reset();
    if (!WorldBounds.IsValid || !IsFiniteVector(WorldBounds.Min) || !IsFiniteVector(WorldBounds.Max)
        || !IsFiniteVector(Transform.GetLocation()))
    {
        OutReason = TEXT("Support placement bounds/transform are invalid.");
        return false;
    }

    const FVector Extent = WorldBounds.GetExtent();
    if (!ValidateWorldCollision(FTransform(Transform.GetRotation(), WorldBounds.GetCenter()), Extent, nullptr, OutReason)
        || !HasExistingShoringConflict(
            FTransform(Transform.GetRotation(), WorldBounds.GetCenter()), Extent, FGuid(), nullptr, OutReason))
    {
        return false;
    }
    return true;
}

void UExcavationShoringSubsystem::OnWorldChunksChanged(const FWorldDirtyChunkSet& ChangedChunks)
{
    if (ChangedChunks.Chunks.IsEmpty())
    {
        return;
    }

    const UExcavationVolumeSubsystem* Volume =
        GetWorld() ? GetWorld()->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!Volume)
    {
        MarkAllPlansForRevalidation();
        return;
    }

    TSet<FGuid> AffectedPlans;
    for (const TPair<FGuid, TWeakObjectPtr<APlaceableShoringPiece>>& Pair : PiecesById)
    {
        const APlaceableShoringPiece* Piece = Pair.Value.Get();
        if (!IsValid(Piece) || Piece->GetSupportAnchor().Kind != EShoringAnchorKind::VoxelSurface)
        {
            continue;
        }

        FIntVector Cell;
        FIntVector Chunk;
        if (Volume->TryWorldToCell(Piece->GetSupportAnchor().PositionCm, Cell)
            && Volume->TryGetChunkCoordinateForCell(Cell, Chunk)
            && ChangedChunks.Chunks.Contains(Chunk))
        {
            AffectedPlans.Add(Piece->GetPlanId());
        }
    }

    for (const FGuid& PlanId : AffectedPlans)
    {
        SetPlanState(PlanId, EShoringPieceState::NeedsRevalidation);
    }

    // A ghost layout is based on the old surface even when the edit is elsewhere; discard all stale previews.
    if (!PreviewPiecesByPlan.IsEmpty())
    {
        TArray<FGuid> PreviewIds;
        PreviewPiecesByPlan.GetKeys(PreviewIds);
        for (const FGuid& PreviewId : PreviewIds)
        {
            ClearPreview(PreviewId);
        }
    }
}

void UExcavationShoringSubsystem::HandleVolumeChanged(const FExcavationEditRecord& Record, bool bIsRollback)
{
    const UExcavationVolumeSubsystem* Volume =
        GetWorld() ? GetWorld()->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!Volume)
    {
        MarkAllPlansForRevalidation();
        return;
    }

    FWorldDirtyChunkSet DirtySet;
    DirtySet.OperationId = Record.OperationId;
    DirtySet.bIncludesNeighbors = true;
    for (const FIntVector& Coordinate : Volume->GetAffectedChunksForEdit(Record))
    {
        DirtySet.AddChunk(Coordinate);
    }
    OnWorldChunksChanged(DirtySet);
}

void UExcavationShoringSubsystem::HandleVolumeReset()
{
    MarkAllPlansForRevalidation();
    TArray<FGuid> PreviewIds;
    PreviewPiecesByPlan.GetKeys(PreviewIds);
    for (const FGuid& PreviewId : PreviewIds)
    {
        ClearPreview(PreviewId);
    }
}

void UExcavationShoringSubsystem::SetPlanState(const FGuid& PlanId, EShoringPieceState State)
{
    if (const TArray<FGuid>* PieceIds = PlanToPieces.Find(PlanId))
    {
        for (const FGuid& PieceId : *PieceIds)
        {
            if (APlaceableShoringPiece* Piece = FindPiece(PieceId))
            {
                Piece->SetPieceState(State);
            }
        }
    }
}

void UExcavationShoringSubsystem::MarkAllPlansForRevalidation()
{
    for (const TPair<FGuid, TArray<FGuid>>& Pair : PlanToPieces)
    {
        SetPlanState(Pair.Key, EShoringPieceState::NeedsRevalidation);
    }
}
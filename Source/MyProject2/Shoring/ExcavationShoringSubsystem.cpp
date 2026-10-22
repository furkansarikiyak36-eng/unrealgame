// Copyright Epic Games, Inc. All Rights Reserved.

#include "ExcavationShoringSubsystem.h"

#include "PlaceableShoringPiece.h"
#include "ShoringLayoutPlanner.h"
#include "ShoringSupportResolver.h"
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

    UWorld* World = GetWorld();
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
            OutPreviewPieces.Reset();
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

    TArray<FShoringSupportAssessment> SupportAssessments;
    if (!EvaluateAndApplyPlanSupport(Plan.PlanId, SupportAssessments, OutError))
    {
        FString RollbackError;
        RemovePlan(Plan.PlanId, RollbackError);
        OutPieces.Reset();
        OutError = FString::Printf(TEXT("Support evaluation failed after spawn; the plan was removed: %s"), *OutError);
        return false;
    }
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

    UWorld* World = GetWorld();
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

    TArray<FShoringSupportNode> ProposedNodes;
    ProposedNodes.Reserve(Plan.Pieces.Num());
    for (const FShoringPieceSpec& Spec : Plan.Pieces)
    {
        FShoringSupportNode& Node = ProposedNodes.AddDefaulted_GetRef();
        Node.PieceId = Spec.PieceId;
        Node.Kind = Spec.Kind;
        Node.AnchorKind = Spec.SupportAnchor.Kind;
        Node.PrimarySupportPieceId = Spec.SupportAnchor.ConnectedPieceId;
        Node.ConnectedPieceIds = Spec.ConnectedPieceIds;
        Node.HealthFraction = 1.0f;
        Node.DesignLoad = Spec.DesignLoad;
        Node.bAnchorCurrent = true; // Voxel anchors were re-traced and validated above.
    }

    TArray<FShoringSupportAssessment> ProposedAssessments;
    if (!FShoringSupportResolver::Evaluate(ProposedNodes, ProposedAssessments, OutError))
    {
        return false;
    }
    for (const FShoringSupportAssessment& Assessment : ProposedAssessments)
    {
        if (Assessment.Condition == EShoringSupportCondition::Unsupported
            || Assessment.Condition == EShoringSupportCondition::Failed
            || Assessment.Condition == EShoringSupportCondition::NeedsRevalidation
            || Assessment.Condition == EShoringSupportCondition::Overloaded)
        {
            OutError = FString::Printf(TEXT("Proposed plan has an invalid or overloaded support at piece %s: %s"),
                *Assessment.PieceId.ToString(), *Assessment.Reason);
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
    FString& OutError,
    bool* bOutTransientFailure) const
{
    OutResolvedPositionCm = FVector::ZeroVector;
    OutError.Reset();
    if (bOutTransientFailure)
    {
        *bOutTransientFailure = false;
    }

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
        if (bOutTransientFailure
            && (SurfaceError.Contains(TEXT("being rebuilt"), ESearchCase::IgnoreCase)
                || SurfaceError.Contains(TEXT("no longer matches the current scan"), ESearchCase::IgnoreCase)))
        {
            *bOutTransientFailure = true;
        }
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
    if (!IsValid(Piece)
        || Piece->GetPieceState() == EShoringPieceState::Removed
        || Piece->GetPieceState() == EShoringPieceState::Failed)
    {
        OutError = TEXT("Shoring piece does not exist or has failed; failed members must be replaced.");
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
    Piece->ClearCollapseAfterReattachment();
    SetPlanState(Piece->GetPlanId(), EShoringPieceState::NeedsRevalidation);
    Piece->SetPieceState(EShoringPieceState::NeedsRevalidation);
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
    bool bHasTransientAnchorFailure = false;
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
            bool bTransientAnchorFailure = false;
            if (!ValidateSurfaceAnchor(
                    Anchor,
                    CurrentRevision,
                    false,
                    RevalidationAnchorDeviationCm,
                    ResolvedPoint,
                    AnchorError,
                    &bTransientAnchorFailure))
            {
                if (bTransientAnchorFailure)
                {
                    // The old collision may still be visible while its replacement is pending.
                    // Fail closed, but do not convert a transient publication gap into collapse.
                    Piece->SetPieceState(EShoringPieceState::NeedsRevalidation);
                    bHasTransientAnchorFailure = true;
                }
                else
                {
                    // A confirmed surface loss invalidates this member and cascades via its GUID links.
                    Piece->SetPieceState(EShoringPieceState::Unsupported);
                }
                continue;
            }
            FShoringSupportAnchor UpdatedAnchor = Anchor;
            UpdatedAnchor.SourceDensityRevision = CurrentRevision;
            UpdatedAnchor.PositionCm = ResolvedPoint;
            Piece->UpdateSupportAnchor(UpdatedAnchor);
        }
        else if (Anchor.Kind == EShoringAnchorKind::PieceSocket)
        {
            // Missing or non-reciprocal sockets are evaluated by the support resolver
            // so their dependent pieces transition to Unsupported rather than staying active.
            FShoringSupportAnchor UpdatedAnchor = Anchor;
            UpdatedAnchor.SourceDensityRevision = CurrentRevision;
            Piece->UpdateSupportAnchor(UpdatedAnchor);
        }
    }

    if (Volume->GetDensityRevision() != CurrentRevision)
    {
        OutError = TEXT("Voxel density changed during shoring revalidation; retry against the new revision.");
        for (const FGuid& PieceId : *PieceIds)
        {
            if (APlaceableShoringPiece* Piece = FindPiece(PieceId))
            {
                if (!Piece->HasCollapsed() && Piece->GetHealth() > KINDA_SMALL_NUMBER)
                {
                    Piece->SetPieceState(EShoringPieceState::NeedsRevalidation);
                }
            }
        }
        return false;
    }

    if (bHasTransientAnchorFailure)
    {
        OutError = TEXT("At least one voxel anchor is waiting for current mesh/collision publication; retry revalidation.");
        for (const FGuid& PieceId : *PieceIds)
        {
            if (APlaceableShoringPiece* Piece = FindPiece(PieceId))
            {
                if (!Piece->HasCollapsed() && Piece->GetHealth() > KINDA_SMALL_NUMBER)
                {
                    Piece->SetPieceState(EShoringPieceState::NeedsRevalidation);
                }
            }
        }
        return false;
    }

    TArray<FShoringSupportAssessment> Assessments;
    if (!EvaluateAndApplyPlanSupport(PlanId, Assessments, OutError, true))
    {
        SetPlanState(PlanId, EShoringPieceState::NeedsRevalidation);
        return false;
    }

    for (const FShoringSupportAssessment& Assessment : Assessments)
    {
        if (Assessment.Condition == EShoringSupportCondition::Unsupported
            || Assessment.Condition == EShoringSupportCondition::Failed
            || Assessment.Condition == EShoringSupportCondition::NeedsRevalidation
            || Assessment.Condition == EShoringSupportCondition::Overloaded)
        {
            OutError = FString::Printf(TEXT("Plan revalidation produced %d unsupported, failed, stale or overloaded support state(s)."),
                static_cast<int32>(Assessment.Condition));
            return false;
        }
    }
    return true;
}

bool UExcavationShoringSubsystem::GetPlanSupportAssessment(
    const FGuid& PlanId,
    TArray<FShoringSupportAssessment>& OutAssessments,
    FString& OutError) const
{
    TArray<FShoringSupportNode> Nodes;
    if (!BuildSupportNodes(PlanId, Nodes, OutError))
    {
        OutAssessments.Reset();
        return false;
    }
    return FShoringSupportResolver::Evaluate(Nodes, OutAssessments, OutError);
}

bool UExcavationShoringSubsystem::BuildSupportNodes(
    const FGuid& PlanId,
    TArray<FShoringSupportNode>& OutNodes,
    FString& OutError,
    bool bAnchorsWereRevalidated) const
{
    OutNodes.Reset();
    OutError.Reset();
    const TArray<FGuid>* PieceIds = PlanToPieces.Find(PlanId);
    const UExcavationVolumeSubsystem* Volume =
        GetWorld() ? GetWorld()->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!PieceIds || PieceIds->IsEmpty() || !Volume)
    {
        OutError = TEXT("Support evaluation requires a committed plan and density owner.");
        return false;
    }

    OutNodes.Reserve(PieceIds->Num());
    for (const FGuid& PieceId : *PieceIds)
    {
        const APlaceableShoringPiece* Piece = FindPiece(PieceId);
        if (!IsValid(Piece))
        {
            OutError = FString::Printf(TEXT("Support plan references missing piece %s."), *PieceId.ToString());
            OutNodes.Reset();
            return false;
        }

        const FShoringSupportAnchor& Anchor = Piece->GetSupportAnchor();
        FShoringSupportNode& Node = OutNodes.AddDefaulted_GetRef();
        Node.PieceId = PieceId;
        Node.Kind = Piece->GetPieceKind();
        Node.AnchorKind = Anchor.Kind;
        Node.PrimarySupportPieceId = Anchor.ConnectedPieceId;
        Node.ConnectedPieceIds = Piece->GetConnectedPieceIds();
        Node.HealthFraction = Piece->GetHealthFraction();
        Node.DesignLoad = Piece->GetDesignLoad();
        Node.bIsCollapsed = Piece->HasCollapsed();
        // Terrain edits invalidate only plans whose anchor chunks were marked dirty.
        // A global density revision increment alone must not stale unrelated supports.
        Node.bAnchorCurrent = bAnchorsWereRevalidated
            || (Piece->GetPieceState() != EShoringPieceState::NeedsRevalidation
                && Piece->GetPieceState() != EShoringPieceState::Invalid);
    }
    return true;
}

bool UExcavationShoringSubsystem::EvaluateAndApplyPlanSupport(
    const FGuid& PlanId,
    TArray<FShoringSupportAssessment>& OutAssessments,
    FString& OutError,
    bool bAnchorsWereRevalidated)
{
    TArray<FShoringSupportNode> Nodes;
    if (!BuildSupportNodes(PlanId, Nodes, OutError, bAnchorsWereRevalidated)
        || !FShoringSupportResolver::Evaluate(Nodes, OutAssessments, OutError))
    {
        return false;
    }

    for (const FShoringSupportAssessment& Assessment : OutAssessments)
    {
        APlaceableShoringPiece* Piece = FindPiece(Assessment.PieceId);
        if (!IsValid(Piece))
        {
            OutError = TEXT("Support evaluator returned a piece that is no longer registered.");
            return false;
        }

        switch (Assessment.Condition)
        {
        case EShoringSupportCondition::Stable:
            Piece->SetPieceState(Piece->GetHealthFraction() < 0.7f
                ? EShoringPieceState::Damaged
                : EShoringPieceState::Active);
            break;
        case EShoringSupportCondition::Warning:
            Piece->SetPieceState(Piece->GetHealthFraction() < 0.7f
                ? EShoringPieceState::Damaged
                : EShoringPieceState::Warning);
            break;
        case EShoringSupportCondition::Overloaded:
            Piece->SetPieceState(EShoringPieceState::Overloaded);
            break;
        case EShoringSupportCondition::Unsupported:
            Piece->SetPieceState(EShoringPieceState::Unsupported);
            break;
        case EShoringSupportCondition::Failed:
            Piece->SetPieceState(EShoringPieceState::Failed);
            break;
        case EShoringSupportCondition::NeedsRevalidation:
        default:
            Piece->SetPieceState(EShoringPieceState::NeedsRevalidation);
            break;
        }
    }
    return true;
}

bool UExcavationShoringSubsystem::ApplyDamageToPiece(
    const FGuid& PieceId,
    float Amount,
    EShoringDamageCause Cause,
    FShoringSupportAssessment& OutAssessment,
    FString& OutError)
{
    OutAssessment = FShoringSupportAssessment{};
    OutError.Reset();
    APlaceableShoringPiece* Piece = FindPiece(PieceId);
    if (!IsValid(Piece))
    {
        OutError = TEXT("Damage target is not a registered shoring piece.");
        return false;
    }
    const FGuid PlanId = Piece->GetPlanId();
    if (!Piece->ApplyDamage(Amount, Cause, OutError))
    {
        return false;
    }

    TArray<FShoringSupportAssessment> Assessments;
    if (!EvaluateAndApplyPlanSupport(PlanId, Assessments, OutError))
    {
        // Damage is already an authoritative state change; make that explicit to the caller.
        OutError = FString::Printf(TEXT("Damage was applied, but support evaluation needs retry: %s"), *OutError);
        return true;
    }
    const FShoringSupportAssessment* Found = Assessments.FindByPredicate(
        [&PieceId](const FShoringSupportAssessment& Value) { return Value.PieceId == PieceId; });
    if (Found)
    {
        OutAssessment = *Found;
    }
    return true;
}

bool UExcavationShoringSubsystem::RepairPiece(const FGuid& PieceId, float Amount, FString& OutError)
{
    OutError.Reset();
    APlaceableShoringPiece* Piece = FindPiece(PieceId);
    if (!IsValid(Piece))
    {
        OutError = TEXT("Repair target is not a registered shoring piece.");
        return false;
    }
    const FGuid PlanId = Piece->GetPlanId();
    if (!Piece->Repair(Amount, OutError))
    {
        return false;
    }

    TArray<FShoringSupportAssessment> Assessments;
    if (!EvaluateAndApplyPlanSupport(PlanId, Assessments, OutError))
    {
        OutError = FString::Printf(TEXT("Repair was applied, but support evaluation needs retry: %s"), *OutError);
    }
    return true;
}

bool UExcavationShoringSubsystem::ReplaceFailedPiece(
    const FGuid& PieceId,
    APlaceableShoringPiece*& OutReplacement,
    FString& OutError,
    TSubclassOf<APlaceableShoringPiece> PieceClass)
{
    OutReplacement = nullptr;
    OutError.Reset();
    APlaceableShoringPiece* FailedPiece = FindPiece(PieceId);
    if (!IsValid(FailedPiece) || FailedPiece->GetPieceState() != EShoringPieceState::Failed)
    {
        OutError = TEXT("Replacement requires a registered piece in Failed state.");
        return false;
    }

    UWorld* World = GetWorld();
    const UExcavationVolumeSubsystem* Volume =
        World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!World || !Volume)
    {
        OutError = TEXT("Replacement requires a world and density owner.");
        return false;
    }
    if (!PieceClass)
    {
        PieceClass = APlaceableShoringPiece::StaticClass();
    }

    FShoringPieceSpec Spec;
    Spec.PieceId = FGuid::NewGuid();
    Spec.PlanId = FailedPiece->GetPlanId();
    Spec.Kind = FailedPiece->GetPieceKind();
    Spec.Transform = FailedPiece->GetActorTransform();
    Spec.HalfExtentsCm = FailedPiece->GetHalfExtentsCm();
    Spec.DesignLoad = FailedPiece->GetDesignLoad();
    Spec.SupportAnchor = FailedPiece->GetSupportAnchor();
    Spec.SupportAnchor.AnchorId = FGuid::NewGuid();
    Spec.SupportAnchor.SourceDensityRevision = Volume->GetDensityRevision();
    Spec.ConnectedPieceIds = FailedPiece->GetConnectedPieceIds();

    if (!ValidateWorldCollision(Spec.Transform, Spec.HalfExtentsCm, FailedPiece, OutError)
        || !HasExistingShoringConflict(Spec.Transform, Spec.HalfExtentsCm, PieceId, nullptr, OutError))
    {
        return false;
    }

    APlaceableShoringPiece* Replacement = World->SpawnActor<APlaceableShoringPiece>(
        PieceClass, Spec.Transform, FActorSpawnParameters());
    if (!Replacement || !Replacement->InitializeFromSpec(Spec, false))
    {
        if (IsValid(Replacement))
        {
            Replacement->Destroy();
        }
        OutError = TEXT("Replacement could not be initialized; original failed actor remains registered.");
        return false;
    }

    const FGuid PlanId = FailedPiece->GetPlanId();
    for (const FGuid& ConnectedId : FailedPiece->GetConnectedPieceIds())
    {
        APlaceableShoringPiece* OtherPiece = FindPiece(ConnectedId);
        if (!IsValid(OtherPiece))
        {
            continue;
        }
        TArray<FGuid> Links = OtherPiece->GetConnectedPieceIds();
        for (FGuid& Link : Links)
        {
            if (Link == PieceId)
            {
                Link = Spec.PieceId;
            }
        }
        OtherPiece->UpdateConnections(Links);

        const FShoringSupportAnchor& OtherAnchor = OtherPiece->GetSupportAnchor();
        if (OtherAnchor.Kind == EShoringAnchorKind::PieceSocket && OtherAnchor.ConnectedPieceId == PieceId)
        {
            FShoringSupportAnchor UpdatedAnchor = OtherAnchor;
            UpdatedAnchor.ConnectedPieceId = Spec.PieceId;
            UpdatedAnchor.AnchorId = FGuid::NewGuid();
            OtherPiece->UpdateSupportAnchor(UpdatedAnchor);
        }
    }

    PiecesById.Remove(PieceId);
    PiecesById.Add(Spec.PieceId, Replacement);
    if (TArray<FGuid>* PlanPieces = PlanToPieces.Find(PlanId))
    {
        PlanPieces->Remove(PieceId);
        PlanPieces->Add(Spec.PieceId);
    }
    FailedPiece->SetPieceState(EShoringPieceState::Removed);
    FailedPiece->Destroy();
    OutReplacement = Replacement;

    TArray<FShoringSupportAssessment> Assessments;
    FString RevalidationError;
    if (!RevalidatePlan(PlanId, RevalidationError))
    {
        OutError = FString::Printf(TEXT("Replacement committed; the plan still needs attention: %s"), *RevalidationError);
    }
    return true;
}

bool UExcavationShoringSubsystem::ExportSnapshot(FShoringWorldSnapshot& OutSnapshot, FString& OutError) const
{
    OutSnapshot = FShoringWorldSnapshot();
    OutError.Reset();
    if (PiecesById.Num() > 4096)
    {
        OutError = TEXT("Shoring world exceeds the 4096-piece save budget.");
        return false;
    }

    FShoringWorldSnapshot Candidate;
    Candidate.Pieces.Reserve(PiecesById.Num());
    for (const TPair<FGuid, TWeakObjectPtr<APlaceableShoringPiece>>& Pair : PiecesById)
    {
        APlaceableShoringPiece* Piece = Pair.Value.Get();
        if (!IsValid(Piece) || Piece->GetPieceState() == EShoringPieceState::Removed
            || Piece->GetPieceState() == EShoringPieceState::Planned)
        {
            continue;
        }
        FShoringPieceSaveData Record;
        Piece->ExportSaveData(Record);
        Candidate.Pieces.Add(Record);
    }
    Candidate.Pieces.Sort([](const FShoringPieceSaveData& A, const FShoringPieceSaveData& B)
    {
        return A.Spec.PieceId.ToString() < B.Spec.PieceId.ToString();
    });
    if (!Candidate.Validate(OutError))
    {
        return false;
    }
    OutSnapshot = MoveTemp(Candidate);
    return true;
}

bool UExcavationShoringSubsystem::ValidateSnapshot(const FShoringWorldSnapshot& Snapshot, FString& OutError) const
{
    OutError.Reset();
    if (!Snapshot.Validate(OutError))
    {
        return false;
    }
    if (Snapshot.Pieces.Num() > 4096)
    {
        OutError = TEXT("Shoring snapshot is above the runtime restore budget.");
        return false;
    }
    return true;
}

bool UExcavationShoringSubsystem::ImportSnapshot(
    const FShoringWorldSnapshot& Snapshot,
    FString& OutError,
    TSubclassOf<APlaceableShoringPiece> PieceClass)
{
    OutError.Reset();
    if (!ValidateSnapshot(Snapshot, OutError))
    {
        return false;
    }
    UWorld* World = GetWorld();
    if (!World)
    {
        OutError = TEXT("Cannot restore shoring without a world.");
        return false;
    }
    if (!PieceClass)
    {
        PieceClass = APlaceableShoringPiece::StaticClass();
    }

    // Spawn and validate the full replacement set first. Existing shoring stays intact on failure.
    TArray<APlaceableShoringPiece*> StagedPieces;
    StagedPieces.Reserve(Snapshot.Pieces.Num());
    for (const FShoringPieceSaveData& Record : Snapshot.Pieces)
    {
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        APlaceableShoringPiece* Staged = World->SpawnActor<APlaceableShoringPiece>(
            PieceClass, Record.Spec.Transform, SpawnParameters);
        if (!IsValid(Staged))
        {
            OutError = TEXT("Failed to stage a shoring actor during world restore.");
            for (APlaceableShoringPiece* Existing : StagedPieces)
            {
                if (IsValid(Existing)) Existing->Destroy();
            }
            return false;
        }
        FString PieceError;
        if (!Staged->InitializeFromSaveData(Record, PieceError))
        {
            Staged->Destroy();
            OutError = FString::Printf(TEXT("Failed to initialize a staged shoring actor: %s"), *PieceError);
            for (APlaceableShoringPiece* Existing : StagedPieces)
            {
                if (IsValid(Existing)) Existing->Destroy();
            }
            return false;
        }
        StagedPieces.Add(Staged);
    }

    TArray<FGuid> ExistingIds;
    PiecesById.GetKeys(ExistingIds);
    for (const FGuid& ExistingId : ExistingIds)
    {
        if (APlaceableShoringPiece* Existing = FindPiece(ExistingId))
        {
            Existing->Destroy();
        }
    }
    PiecesById.Reset();
    PlanToPieces.Reset();
    CommittedPlanIds.Reset();
    TArray<FGuid> PreviewPlanIds;
    PreviewPiecesByPlan.GetKeys(PreviewPlanIds);
    for (const FGuid& PreviewPlanId : PreviewPlanIds)
    {
        ClearPreview(PreviewPlanId);
    }

    for (APlaceableShoringPiece* Piece : StagedPieces)
    {
        const FGuid PieceId = Piece->GetPieceId();
        const FGuid PlanId = Piece->GetPlanId();
        PiecesById.Add(PieceId, Piece);
        PlanToPieces.FindOrAdd(PlanId).Add(PieceId);
        CommittedPlanIds.Add(PlanId);
    }
    return true;
}

void UExcavationShoringSubsystem::GetCommittedPlanIds(TArray<FGuid>& OutPlanIds) const
{
    OutPlanIds.Reset();
    CommittedPlanIds.GenerateKeyArray(OutPlanIds);
    OutPlanIds.Sort([](const FGuid& A, const FGuid& B)
    {
        return A.ToString() < B.ToString();
    });
}

bool UExcavationShoringSubsystem::RevalidateAllPlans(FString& OutError)
{
    OutError.Reset();
    TArray<FGuid> PlanIds;
    GetCommittedPlanIds(PlanIds);
    TArray<FString> Failures;
    for (const FGuid& PlanId : PlanIds)
    {
        FString PlanError;
        if (!RevalidatePlan(PlanId, PlanError))
        {
            Failures.Add(FString::Printf(TEXT("%s: %s"), *PlanId.ToString(), *PlanError));
        }
    }
    if (Failures.Num() > 0)
    {
        OutError = FString::Join(Failures, TEXT("\n"));
        return false;
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
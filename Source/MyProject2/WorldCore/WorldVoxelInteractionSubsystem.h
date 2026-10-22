// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldCore/ExcavationTypes.h"
#include "WorldCore/WorldSystemContracts.h"
#include "WorldVoxelInteractionSubsystem.generated.h"

class AActor;
class UPrimitiveComponent;

/**
 * Connects the Phase 2 scanner to the authoritative Phase 3 density/edit owner.
 * Also provides revision-aware snap candidates for actors placed on voxel surfaces.
 *
 * All geometry edits are requested through this subsystem or IVoxelEditService;
 * visual meshes and collision are refreshed by UExcavationTerrainChunkSubsystem.
 */
UCLASS()
class MYPROJECT2_API UWorldVoxelInteractionSubsystem final
    : public UWorldSubsystem
    , public IVoxelEditService
    , public IPlacementSurfaceProvider
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Demand-driven scanner query from an explicit view ray. */
    bool TraceFromView(
        const FVector& OriginCm,
        const FVector& Direction,
        float ReachCm,
        const AActor* ActorToIgnore,
        FWorldScanHit& OutHit,
        FString& OutError) const;

    /** A single pickaxe action: trace, authorize the current hit, then commit one bounded sphere carve. */
    bool DigFromView(
        const FVector& OriginCm,
        const FVector& Direction,
        float ReachCm,
        float RadiusCm,
        const AActor* ActorToIgnore,
        FWorldVoxelEditResult& OutResult,
        FString& OutError);

    /** True only for an actor/component explicitly marked as a generated voxel surface. */
    static bool IsVoxelSurfaceHit(const FHitResult& Hit);

    /** Rejects stale collision or any surface that the current scanner won't support/snap to. */
    bool ValidateVoxelSurfaceHit(
        const FHitResult& Hit,
        const AActor* ActorToIgnore,
        FString& OutError) const;

    /** Candidate + transform for a new actor. Caller must call ValidatePlacementCommit just before spawning/consuming. */
    bool BuildPlacementPreview(
        const FWorldPlacementQuery& Query,
        const AActor* ActorToIgnore,
        FWorldPlacementResult& OutResult) const;

    /** Re-runs target, revision, orientation and local-overlap checks at commit time. */
    bool ValidatePlacementCommit(
        const FWorldPlacementQuery& Query,
        const FWorldPlacementResult& Preview,
        const AActor* ActorToIgnore,
        FWorldPlacementResult& OutResult) const;

    virtual bool ApplyEdit(
        const FWorldVoxelEditRequest& Request,
        FWorldVoxelEditResult& OutResult) override;

    virtual bool RollbackEdit(
        const FGuid& OperationId,
        FWorldVoxelEditResult& OutResult) override;

    virtual bool GatherCandidates(
        const FWorldPlacementQuery& Query,
        TArray<FWorldSnapCandidate>& OutCandidates) const override;

private:
    bool ApplyAuthorizedEdit(
        const FWorldVoxelEditRequest& Request,
        FWorldVoxelEditResult& OutResult,
        bool bHasCurrentScanAuthorization);

    bool GatherCandidatesForActor(
        const FWorldPlacementQuery& Query,
        const AActor* ActorToIgnore,
        TArray<FWorldSnapCandidate>& OutCandidates,
        FString& OutError) const;

    bool HasPlacementOverlap(
        const FTransform& Transform,
        const FVector& PieceHalfExtentCm,
        const AActor* ActorToIgnore) const;

    bool IsCurrentVoxelSurfaceActor(const AActor* Actor, FString& OutError) const;

    TMap<FGuid, FExcavationEditRecord> EditReceipts;
    TArray<FGuid> EditReceiptOrder;
};

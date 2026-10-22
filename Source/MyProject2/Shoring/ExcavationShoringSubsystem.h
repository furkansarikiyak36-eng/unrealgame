// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Shoring/ShoringTypes.h"
#include "WorldCore/ExcavationTypes.h"
#include "WorldCore/WorldSystemContracts.h"
#include "ExcavationShoringSubsystem.generated.h"

class APlaceableShoringPiece;

/**
 * Owns preview/commit/reposition/remove operations for separate shoring actors.
 * Terrain density remains authoritative in UExcavationVolumeSubsystem.
 */
UCLASS()
class MYPROJECT2_API UExcavationShoringSubsystem final
    : public UWorldSubsystem
    , public IStructureSupportAdapter
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    bool BuildPlan(
        const FShoringLayoutRequest& Request,
        FShoringLayoutPlan& OutPlan,
        FString& OutError) const;

    /** Spawns non-colliding preview actors. A preview never registers as committed structure. */
    bool PreviewPlan(
        const FShoringLayoutPlan& Plan,
        TArray<APlaceableShoringPiece*>& OutPreviewPieces,
        FString& OutError);

    void ClearPreview(const FGuid& PlanId);

    /** Revalidates density revision, wall anchors and placement conflicts before any actor is spawned. */
    bool CommitPlan(
        const FShoringLayoutPlan& Plan,
        TArray<APlaceableShoringPiece*>& OutPieces,
        FString& OutError,
        TSubclassOf<APlaceableShoringPiece> PieceClass = nullptr);

    bool RevalidatePlan(const FGuid& PlanId, FString& OutError);

    bool GetPlanSupportAssessment(
        const FGuid& PlanId,
        TArray<FShoringSupportAssessment>& OutAssessments,
        FString& OutError) const;

    bool ApplyDamageToPiece(
        const FGuid& PieceId,
        float Amount,
        EShoringDamageCause Cause,
        FShoringSupportAssessment& OutAssessment,
        FString& OutError);

    bool RepairPiece(const FGuid& PieceId, float Amount, FString& OutError);

    bool ReplaceFailedPiece(
        const FGuid& PieceId,
        APlaceableShoringPiece*& OutReplacement,
        FString& OutError,
        TSubclassOf<APlaceableShoringPiece> PieceClass = nullptr);

    bool MovePiece(
        const FGuid& PieceId,
        const FTransform& NewTransform,
        const FShoringSupportAnchor& NewAnchor,
        FString& OutError);

    bool RemovePiece(const FGuid& PieceId, FString& OutError);
    bool RemovePlan(const FGuid& PlanId, FString& OutError);

    APlaceableShoringPiece* FindPiece(const FGuid& PieceId) const;
    TArray<APlaceableShoringPiece*> GetPlanPieces(const FGuid& PlanId) const;

    virtual bool ValidatePlacement(
        const FTransform& Transform,
        const FBox& WorldBounds,
        FString& OutReason) const override;

    virtual void OnWorldChunksChanged(const FWorldDirtyChunkSet& ChangedChunks) override;

protected:
    UFUNCTION()
    void HandleVolumeChanged(const FExcavationEditRecord& Record, bool bIsRollback);

    UFUNCTION()
    void HandleVolumeReset();

private:
    bool ValidatePlanForCommit(const FShoringLayoutPlan& Plan, FString& OutError) const;

    bool ValidateSurfaceAnchor(
        const FShoringSupportAnchor& Anchor,
        uint64 ExpectedRevision,
        bool bRequireSameRevision,
        float MaxDeviationCm,
        FVector& OutResolvedPositionCm,
        FString& OutError,
        bool* bOutTransientFailure = nullptr) const;

    bool ValidateWorldCollision(
        const FTransform& Transform,
        const FVector& HalfExtentsCm,
        const AActor* ActorToIgnore,
        FString& OutError) const;

    bool HasExistingShoringConflict(
        const FTransform& Transform,
        const FVector& HalfExtentsCm,
        const FGuid& PieceToIgnore,
        const TArray<FGuid>* AllowedConnectedPieceIds,
        FString& OutError) const;

    bool BuildSupportNodes(
        const FGuid& PlanId,
        TArray<FShoringSupportNode>& OutNodes,
        FString& OutError,
        bool bAnchorsWereRevalidated = false) const;

    bool EvaluateAndApplyPlanSupport(
        const FGuid& PlanId,
        TArray<FShoringSupportAssessment>& OutAssessments,
        FString& OutError,
        bool bAnchorsWereRevalidated = false);

    void SetPlanState(const FGuid& PlanId, EShoringPieceState State);
    void MarkAllPlansForRevalidation();

    TMap<FGuid, TWeakObjectPtr<APlaceableShoringPiece>> PiecesById;
    TMap<FGuid, TArray<FGuid>> PlanToPieces;
    TMap<FGuid, TArray<TWeakObjectPtr<APlaceableShoringPiece>>> PreviewPiecesByPlan;
    TSet<FGuid> CommittedPlanIds;
};
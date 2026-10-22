#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Shoring/PlaceableShoringPiece.h"
#include "ShoringLayoutPlannerSubsystem.generated.h"

UCLASS()
class MYPROJECT2_API UShoringLayoutPlannerSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    /** Top-edge endpoints on trench centreline. This creates a layout preview, not an engineering safety assessment. */
    UFUNCTION(BlueprintCallable, Category = "Shoring|Planner")
    bool BuildTrenchPlan(FVector StartTopCm, FVector EndTopCm, float WidthCm, float DepthCm,
        float StationSpacingCm, TArray<FShoringPlacementCandidate>& OutCandidates, FString& OutFailure) const;

    /** Commit preview candidates as separate physical actors; overlap checks target existing dynamic objects. */
    UFUNCTION(BlueprintCallable, Category = "Shoring|Planner")
    bool CommitPlan(const TArray<FShoringPlacementCandidate>& Candidates, AActor* ActorToIgnore,
        TArray<APlaceableShoringPiece*>& OutSpawnedPieces, FString& OutFailure) const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoring|Planner", meta = (ClampMin = "5.0"))
    float UprightThicknessCm = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoring|Planner", meta = (ClampMin = "2.0"))
    float WallBoardThicknessCm = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoring|Planner", meta = (ClampMin = "5.0"))
    float WallBoardHeightCm = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoring|Planner", meta = (ClampMin = "5.0"))
    float StrutThicknessCm = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoring|Planner", meta = (ClampMin = "10.0"))
    float StrutVerticalSpacingCm = 100.0f;
};

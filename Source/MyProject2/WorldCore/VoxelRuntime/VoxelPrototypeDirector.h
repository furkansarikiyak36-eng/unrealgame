#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldCore/ExcavationTypes.h"
#include "Shoring/PlaceableShoringPiece.h"
#include "VoxelPrototypeDirector.generated.h"

class APlaceableShoringPiece;
class ULandscapeBuilderSubsystem;
class UVoxelChunkMeshSubsystem;
class UVoxelEditService;
class UShoringLayoutPlannerSubsystem;

/**
 * Blueprint-callable façade for a dedicated test map. It does not automatically
 * create terrain or change gameplay input; place one instance in the test level.
 */
UCLASS()
class MYPROJECT2_API AVoxelPrototypeDirector : public AActor
{
    GENERATED_BODY()

public:
    AVoxelPrototypeDirector();

    UFUNCTION(BlueprintCallable, Category = "Voxel Prototype")
    int32 BuildWorldClassification();

    UFUNCTION(BlueprintCallable, Category = "Voxel Prototype")
    bool RebuildRegion(FVector CenterCm, FVector ExtentCm);

    UFUNCTION(BlueprintCallable, Category = "Voxel Prototype")
    bool CarveSphere(FVector CenterCm, float RadiusCm, FString& OutFailure);

    UFUNCTION(BlueprintCallable, Category = "Voxel Prototype")
    bool CarveTunnel(FVector StartCm, FVector EndCm, float RadiusCm, FString& OutFailure);

    UFUNCTION(BlueprintCallable, Category = "Voxel Prototype")
    bool FillSphere(FVector CenterCm, float RadiusCm, FString& OutFailure);

    UFUNCTION(BlueprintCallable, Category = "Voxel Prototype|Shoring")
    bool BuildTrenchPlan(FVector StartTopCm, FVector EndTopCm, float WidthCm, float DepthCm,
        float StationSpacingCm, FString& OutFailure);

    UFUNCTION(BlueprintCallable, Category = "Voxel Prototype|Shoring")
    bool CommitLastTrenchPlan(AActor* ActorToIgnore, FString& OutFailure);

    UFUNCTION(BlueprintCallable, Category = "Voxel Prototype|Shoring")
    void ClearLastTrenchPlan();

    UFUNCTION(BlueprintPure, Category = "Voxel Prototype|Shoring")
    int32 GetLastTrenchPlanPieceCount() const { return LastTrenchPlan.Num(); }

    UFUNCTION(BlueprintPure, Category = "Voxel Prototype|Shoring")
    int32 GetLastSpawnedShoringPieceCount() const { return LastSpawnedPieces.Num(); }

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voxel Prototype")
    TArray<FShoringPlacementCandidate> LastTrenchPlan;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Voxel Prototype")
    TArray<TObjectPtr<APlaceableShoringPiece>> LastSpawnedPieces;

private:
    ULandscapeBuilderSubsystem* GetBuilder() const;
    UVoxelChunkMeshSubsystem* GetMeshSubsystem() const;
    UVoxelEditService* GetEditService() const;
    UShoringLayoutPlannerSubsystem* GetShoringPlanner() const;
};

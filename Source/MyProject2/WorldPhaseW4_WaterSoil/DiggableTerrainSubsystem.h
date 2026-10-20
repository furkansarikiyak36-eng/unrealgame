#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldCore/WorldTypes.h"
#include "DiggableTerrainSubsystem.generated.h"

class UWorldStateSubsystem;

UENUM(BlueprintType)
enum class EDigBrushShape : uint8
{
    GridCell UMETA(DisplayName="Grid Cell"),
    Circle UMETA(DisplayName="Circular Brush"),
    Square UMETA(DisplayName="Square Brush")
};

USTRUCT(BlueprintType)
struct FDigOperationResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) bool bSucceeded = false;
    UPROPERTY(BlueprintReadOnly) int32 CellsChanged = 0;
    UPROPERTY(BlueprintReadOnly) float ExcavatedVolumeM3 = 0.0f;
    UPROPERTY(BlueprintReadOnly) float SoilConsumedM3 = 0.0f;
    UPROPERTY(BlueprintReadOnly) TArray<FIntPoint> AffectedChunks;
    UPROPERTY(BlueprintReadOnly) FText FailureReason;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnDiggableTerrainChanged, const TArray<FIntPoint>&, AffectedChunks, int32, ChangedCells, float, ExcavatedVolumeM3, float, FilledVolumeM3);

/**
 * Hybrid terrain editing over the authoritative chunk/cell world model.
 * Grid cells remain authoritative; brush shapes select and weight cells.
 * A terrain renderer subscribes to OnTerrainChanged and rebuilds only affected regions.
 */
UCLASS()
class MYPROJECT2_API UDiggableTerrainSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Digging", meta=(ClampMin="0.05")) float MinimumSoilDepthMeters = 0.05f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Digging", meta=(ClampMin="0.1")) float MaximumBrushRadiusMeters = 12.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Digging", meta=(ClampMin="0.01")) float MaximumDigDepthPerActionMeters = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Digging", meta=(ClampMin="0.01")) float LooseSoilExpansionRatio = 1.25f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Farming", meta=(ClampMin="0", ClampMax="1")) float TillageCompactionReduction = 0.35f;
    UPROPERTY(BlueprintAssignable, Category="Digging") FOnDiggableTerrainChanged OnTerrainChanged;

    UFUNCTION(BlueprintCallable, Category="Digging")
    FDigOperationResult DigAtWorld(const FVector& WorldLocation, float RadiusMeters, float DepthMeters, EDigBrushShape Shape = EDigBrushShape::Circle);

    UFUNCTION(BlueprintCallable, Category="Digging")
    FDigOperationResult FillAtWorld(const FVector& WorldLocation, float RadiusMeters, float AvailableLooseSoilM3, EDigBrushShape Shape = EDigBrushShape::Circle);

    UFUNCTION(BlueprintCallable, Category="Farming")
    FDigOperationResult PrepareSoilForPlanting(const FVector& WorldLocation, float RadiusMeters, EDigBrushShape Shape = EDigBrushShape::Circle);

    UFUNCTION(BlueprintCallable, Category="Farming")
    bool CanPlantAtWorld(const FVector& WorldLocation, float RequiredRootDepthMeters, FText& OutReason) const;

    UFUNCTION(BlueprintCallable, Category="Digging")
    bool SetGroundTreatmentAtWorld(const FVector& WorldLocation, EWorldGroundTreatment Treatment);

protected:
    UPROPERTY() TObjectPtr<UWorldStateSubsystem> WorldState;

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    struct FCellCandidate
    {
        FVector WorldCenter = FVector::ZeroVector;
        float Weight = 1.0f;
    };

    void GatherCandidates(const FVector& WorldLocation, float RadiusMeters, EDigBrushShape Shape, TArray<FCellCandidate>& OutCandidates) const;
    bool ResolveExistingCell(const FVector& WorldLocation, UChunkState*& OutChunk, FChunkCell*& OutCell, FIntPoint& OutChunkCoord) const;
    void BroadcastChange(const TSet<FIntPoint>& ChangedChunks, int32 ChangedCells, float ExcavatedM3, float FilledM3);
};

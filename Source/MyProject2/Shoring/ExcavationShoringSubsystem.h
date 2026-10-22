#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ExcavationTypes.h"
#include "ExcavationShoringSubsystem.generated.h"

class APlaceableShoringPiece;

/**
 * Invalidates shoring anchors near terrain edits. It intentionally does not
 * guess that a support is still valid after the terrain data changed.
 */
UCLASS()
class MYPROJECT2_API UExcavationShoringSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Shoring|Support")
    int32 RevalidateAllVoxelAnchors();

    UFUNCTION(BlueprintPure, Category = "Shoring|Support")
    int32 GetLastInvalidatedPieceCount() const { return LastInvalidatedPieceCount; }

protected:
    UFUNCTION()
    void HandleVolumeChanged(const FExcavationEditRecord& Record, bool bIsRollback);

    UFUNCTION()
    void HandleVolumeReset();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoring|Support", meta = (ClampMin = "0.0"))
    float InvalidationRadiusCm = 150.0f;

private:
    int32 LastInvalidatedPieceCount = 0;
    void InvalidateNearBounds(const FBox& Bounds);
};

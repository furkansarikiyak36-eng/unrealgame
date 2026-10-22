#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "VoxelSurfaceAnchorProvider.generated.h"

class AActor;
class UPrimitiveComponent;

USTRUCT(BlueprintType)
struct MYPROJECT2_API FVoxelSurfaceAnchor
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Placement")
    FVector PointCm = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Placement")
    FVector Normal = FVector::UpVector;

    UPROPERTY(BlueprintReadOnly, Category = "Placement")
    float DistanceCm = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Placement")
    TObjectPtr<AActor> TargetActor = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Placement")
    TObjectPtr<UPrimitiveComponent> TargetComponent = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Placement")
    bool bValid = false;
};

/** Raycast-to-surface adapter used by digging and placement; it does not make commit decisions. */
UCLASS()
class MYPROJECT2_API UVoxelSurfaceAnchorProvider : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "Placement|Voxel")
    bool TraceEditableSurface(FVector StartCm, FVector EndCm, float MaxAngleFromUpDegrees,
        FVoxelSurfaceAnchor& OutAnchor, FString& OutFailure) const;

    UFUNCTION(BlueprintCallable, Category = "Placement|Voxel")
    bool MakeSurfaceAlignedTransform(const FVoxelSurfaceAnchor& Anchor, FVector LocalOffsetCm,
        FTransform& OutTransform) const;
};

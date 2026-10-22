#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VoxelExcavationToolComponent.generated.h"

/**
 * First-player-action bridge: camera trace -> editable-surface validation -> voxel edit
 * -> stamina payment. A failed payment rolls the edit back.
 */
UCLASS(ClassGroup = (Voxel), meta = (BlueprintSpawnableComponent))
class MYPROJECT2_API UVoxelExcavationToolComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UVoxelExcavationToolComponent();

    UFUNCTION(BlueprintCallable, Category = "Voxel|Excavation")
    bool ExcavateFromView(float ReachCm, float RadiusCm, FString& OutFailure);

    UFUNCTION(BlueprintCallable, Category = "Voxel|Excavation")
    bool ExcavateTunnelBetween(FVector StartCm, FVector EndCm, float RadiusCm, FString& OutFailure);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Excavation", meta = (ClampMin = "50.0", ClampMax = "2000.0"))
    float DefaultReachCm = 450.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Excavation", meta = (ClampMin = "10.0", ClampMax = "200.0"))
    float DefaultRadiusCm = 40.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Excavation", meta = (ClampMin = "0.0"))
    float DigStaminaCost = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Excavation", meta = (ClampMin = "0.0"))
    float TunnelStaminaCostPerMeter = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Excavation")
    bool bRequirePlayerSurvivalAdapter = true;

private:
    bool PayStaminaOrRollback(const FExcavationEditRecord& Record, float Cost, FString& OutFailure);
};

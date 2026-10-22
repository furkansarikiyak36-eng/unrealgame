#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ExcavationTypes.h"
#include "VoxelEditService.generated.h"

/** Gate for edits: classification first, authoritative sparse voxel edit second. */
UCLASS()
class MYPROJECT2_API UVoxelEditService : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "Voxel|Excavation")
    bool CarveSphere(FVector CenterCm, float RadiusCm, FExcavationEditRecord& OutRecord, FString& OutFailure);

    UFUNCTION(BlueprintCallable, Category = "Voxel|Excavation")
    bool CarveCapsule(FVector StartCm, FVector EndCm, float RadiusCm, FExcavationEditRecord& OutRecord, FString& OutFailure);

    UFUNCTION(BlueprintCallable, Category = "Voxel|Excavation")
    bool CarveBox(FVector MinCm, FVector MaxCm, FExcavationEditRecord& OutRecord, FString& OutFailure);

    UFUNCTION(BlueprintCallable, Category = "Voxel|Excavation")
    bool FillSphere(FVector CenterCm, float RadiusCm, FExcavationEditRecord& OutRecord, FString& OutFailure);

private:
    bool ValidateEditableShape(EExcavationShapeType ShapeType, FVector CenterCm, FVector EndCm,
        FVector HalfExtentCm, float RadiusCm, FString& OutFailure);
    bool EnsureServices(class ULandscapeBuilderSubsystem*& OutBuilder, class UExcavationVolumeSubsystem*& OutVolume);
};

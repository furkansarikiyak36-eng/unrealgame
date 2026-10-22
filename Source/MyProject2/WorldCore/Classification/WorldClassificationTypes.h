#pragma once

#include "CoreMinimal.h"
#include "WorldClassificationTypes.generated.h"

UENUM(BlueprintType)
enum class EWorldTerrainMode : uint8
{
    Unknown UMETA(DisplayName = "Unknown / Review"),
    EditableVoxel UMETA(DisplayName = "Editable Voxel"),
    StaticGeometry UMETA(DisplayName = "Static Geometry"),
    Transition UMETA(DisplayName = "Voxel / Static Transition"),
    Water UMETA(DisplayName = "Water")
};

USTRUCT(BlueprintType)
struct MYPROJECT2_API FWorldClassificationResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Classification")
    EWorldTerrainMode Mode = EWorldTerrainMode::Unknown;

    UPROPERTY(BlueprintReadOnly, Category = "World Classification")
    int32 Priority = MIN_int32;

    UPROPERTY(BlueprintReadOnly, Category = "World Classification")
    bool bExplicitRule = false;

    UPROPERTY(BlueprintReadOnly, Category = "World Classification")
    FName SourceLabel = NAME_None;
};

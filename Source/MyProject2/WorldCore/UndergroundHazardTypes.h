#pragma once

#include "CoreMinimal.h"
#include "UndergroundHazardTypes.generated.h"

UENUM(BlueprintType)
enum class EUndergroundSupportType : uint8
{
    TimberPost UMETA(DisplayName = "Timber Post"),
    TimberBrace UMETA(DisplayName = "Timber Brace"),
    StonePillar UMETA(DisplayName = "Stone Pillar"),
    ReinforcedBeam UMETA(DisplayName = "Reinforced Beam")
};

USTRUCT(BlueprintType)
struct MYPROJECT2_API FUndergroundWaterPocket
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Underground|Water")
    FIntVector Cell = FIntVector::ZeroValue;

    /** Water volume in cubic meters, separate from the 2D surface-water cell. */
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Underground|Water")
    float VolumeM3 = 0.0f;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Underground|Water")
    float FlowRateM3PerSecond = 0.0f;
};

USTRUCT(BlueprintType)
struct MYPROJECT2_API FUndergroundSupportSaveRecord
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Underground|Support")
    FGuid SupportId;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Underground|Support")
    FVector LocationCm = FVector::ZeroVector;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Underground|Support")
    FRotator Rotation = FRotator::ZeroRotator;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Underground|Support")
    EUndergroundSupportType Type = EUndergroundSupportType::TimberPost;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Underground|Support")
    float Integrity = 100.0f;
};

/** Domain-owned snapshot appended to the shared core save envelope in Phase 4. */
USTRUCT(BlueprintType)
struct MYPROJECT2_API FUndergroundHazardSnapshot
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Underground|Save")
    int32 SchemaVersion = 1;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Underground|Save")
    TArray<FUndergroundSupportSaveRecord> Supports;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Underground|Save")
    TArray<FUndergroundWaterPocket> WaterPockets;
};

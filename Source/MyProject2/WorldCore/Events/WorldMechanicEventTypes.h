#pragma once

#include "CoreMinimal.h"
#include "WorldMechanicEventTypes.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EWorldMechanicEventType : uint8
{
    None,
    ClassificationRebuilt,
    VoxelEditCommitted,
    VoxelEditRolledBack,
    VoxelWorldReset,
    TerrainMeshPublished,
    PlacementCommitted,
    PlacementRejected,
    ShoringPlanCommitted,
    ShoringSupportInvalidated,
    StructureDamaged,
    StructureBroken,
    FireStateChanged,
    RaidStarted,
    RaidEnded,
    HarvestCommitted,
    CropStateChanged,
    AnimalProductCreated,
    WaterStateChanged,
    SoilStateChanged,
    WorldSaveCompleted,
    WorldLoadCompleted,
    SurvivalEnvironmentChanged
};

/** Common envelope for cross-system world events; payload interpretation is determined by Type. */
USTRUCT(BlueprintType)
struct MYPROJECT2_API FWorldMechanicEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Event")
    FGuid EventId;

    UPROPERTY(BlueprintReadOnly, Category = "World Event")
    EWorldMechanicEventType Type = EWorldMechanicEventType::None;

    UPROPERTY(BlueprintReadOnly, Category = "World Event")
    FVector PositionCm = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "World Event")
    FVector ExtentCm = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "World Event")
    float ScalarValue = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "World Event")
    float SecondaryValue = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "World Event")
    float TertiaryValue = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "World Event")
    FName SourceTag = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "World Event")
    FGuid OperationId;

    UPROPERTY(BlueprintReadOnly, Category = "World Event")
    TObjectPtr<AActor> SourceActor = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "World Event")
    uint32 WorldVersion = 0;

    UPROPERTY(BlueprintReadOnly, Category = "World Event")
    bool bHighPriority = false;

    bool IsValid() const
    {
        return Type != EWorldMechanicEventType::None
            && FMath::IsFinite(PositionCm.X) && FMath::IsFinite(PositionCm.Y) && FMath::IsFinite(PositionCm.Z)
            && FMath::IsFinite(ExtentCm.X) && FMath::IsFinite(ExtentCm.Y) && FMath::IsFinite(ExtentCm.Z)
            && FMath::IsFinite(ScalarValue)
            && FMath::IsFinite(SecondaryValue) && FMath::IsFinite(TertiaryValue);
    }
};

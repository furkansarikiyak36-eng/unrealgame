#pragma once

#include "CoreMinimal.h"
#include "ExcavationTypes.generated.h"

/**
 * Describes the intent of an edit against the sparse excavation override layer.
 * Rendering/collision generation is deliberately outside this data contract.
 */
UENUM(BlueprintType)
enum class EExcavationEditType : uint8
{
    Carve UMETA(DisplayName = "Carve"),
    Fill UMETA(DisplayName = "Fill")
};

/** Current explicit density for a cell modified by an excavation edit. */
USTRUCT(BlueprintType)
struct MYPROJECT2_API FExcavationCellOverride
{
    GENERATED_BODY()

    /** Normalized density in [0, 1]. 0 is empty, 1 is solid. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Excavation")
    float Density = 0.0f;

    /** Latest committed edit that owns this cell override. Used for safe rollback. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Excavation")
    FGuid LastOperationId;
};

/** Before/after information for one cell, captured before an edit mutates state. */
USTRUCT(BlueprintType)
struct MYPROJECT2_API FExcavationCellChange
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Excavation")
    FIntVector Cell = FIntVector::ZeroValue;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Excavation")
    bool bHadPreviousOverride = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Excavation")
    float PreviousDensity = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Excavation")
    FGuid PreviousOperationId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Excavation")
    float NewDensity = 0.0f;
};

/**
 * Reversible edit receipt. The receipt is an in-memory transaction record,
 * not yet the versioned save-game format; persistence is owned by a later phase.
 */
USTRUCT(BlueprintType)
struct MYPROJECT2_API FExcavationEditRecord
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Excavation")
    FGuid OperationId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Excavation")
    int32 SchemaVersion = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Excavation")
    EExcavationEditType EditType = EExcavationEditType::Carve;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Excavation")
    FVector CenterCm = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Excavation")
    float RadiusCm = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Excavation")
    TArray<FExcavationCellChange> Changes;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Excavation")
    bool bCommitted = false;
};

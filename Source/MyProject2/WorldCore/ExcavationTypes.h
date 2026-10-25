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

UENUM(BlueprintType)
enum class EExcavationShapeType : uint8
{
    Sphere UMETA(DisplayName = "Sphere"),
    Capsule UMETA(DisplayName = "Capsule"),
    Box UMETA(DisplayName = "Box")
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
    EExcavationShapeType ShapeType = EExcavationShapeType::Sphere;

    /** Sphere center, capsule start, or box center. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Excavation")
    FVector CenterCm = FVector::ZeroVector;

    /** Capsule end point; unused for sphere and box. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Excavation")
    FVector EndCm = FVector::ZeroVector;

    /** Box half-extents; unused for sphere and capsule. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Excavation")
    FVector HalfExtentCm = FVector::ZeroVector;

    /** Sphere/capsule radius; zero for box. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Excavation")
    float RadiusCm = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Excavation")
    TArray<FExcavationCellChange> Changes;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Excavation")
    bool bCommitted = false;

    /** Cells inside the shape left alone because their material cannot be dug (rock); set even when the edit fails. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Excavation")
    int32 BlockedCellCount = 0;

    /** Cells that went from solid to empty (carve) or empty to solid (fill): the soil moved, in cells. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Excavation")
    int32 MovedCellCount = 0;
};

/** Persistable sparse override row. Runtime rollback ownership IDs are intentionally not saved. */
USTRUCT(BlueprintType)
struct MYPROJECT2_API FExcavationCellSnapshot
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Excavation|Save")
    FIntVector Cell = FIntVector::ZeroValue;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Excavation|Save")
    float Density = 0.0f;
};

/** Versioned data transfer contract consumed by the later world-persistence adapter. */
USTRUCT(BlueprintType)
struct MYPROJECT2_API FExcavationVolumeSnapshot
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Excavation|Save")
    int32 SchemaVersion = 1;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Excavation|Save")
    float CellSizeCm = 50.0f;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Excavation|Save")
    int32 CellsPerChunkAxis = 32;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Excavation|Save")
    float DefaultBaseDensity = 1.0f;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Excavation|Save")
    TArray<FExcavationCellSnapshot> Overrides;
};

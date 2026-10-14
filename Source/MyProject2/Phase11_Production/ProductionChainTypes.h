#pragma once

#include "CoreMinimal.h"
#include "InventoryCore/InventoryTypes.h"
#include "ProductionChainTypes.generated.h"

UENUM(BlueprintType)
enum class EProductionJobStatus : uint8
{
    Queued,
    Running,
    BlockedPower,
    BlockedWorkers,
    BlockedOutput,
    Completed,
    Cancelled
};

USTRUCT(BlueprintType)
struct MYPROJECT2_API FProductionIngredient
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Production|Recipe")
    FName ItemID = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Production|Recipe", meta=(ClampMin="1"))
    int32 Count = 1;
};

USTRUCT(BlueprintType)
struct MYPROJECT2_API FProductionOutput
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Production|Recipe")
    FName ItemID = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Production|Recipe", meta=(ClampMin="1"))
    int32 Count = 1;
};

/** A queue row owns reserved input payloads and precomputed output payloads, not a second stock ledger. */
USTRUCT(BlueprintType)
struct MYPROJECT2_API FProductionJob
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Production|Queue")
    FGuid JobId;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Production|Queue")
    FName RecipeID = NAME_None;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Production|Queue")
    FText DisplayName;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Production|Queue")
    TArray<FInventoryItem> ReservedInputs;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Production|Queue")
    TArray<FInventoryItem> Outputs;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Production|Queue")
    int32 BatchCount = 1;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Production|Queue")
    float ProgressSeconds = 0.0f;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Production|Queue")
    float RequiredDurationSeconds = 1.0f;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Production|Queue")
    float RequiredPowerWatts = 0.0f;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Production|Queue")
    int32 RequiredWorkers = 0;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Production|Queue")
    EProductionJobStatus Status = EProductionJobStatus::Queued;
};

/** Save-boundary value snapshot. Inventory stock is saved by its existing Inventory/Storage owner. */
USTRUCT(BlueprintType)
struct MYPROJECT2_API FProductionChainSnapshot
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Production|Save")
    int32 SchemaVersion = 1;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Production|Save")
    FName StationKey = NAME_None;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Production|Save")
    int64 Revision = 0;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Production|Save")
    bool bQueuePaused = false;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category="Production|Save")
    TArray<FProductionJob> Jobs;
};

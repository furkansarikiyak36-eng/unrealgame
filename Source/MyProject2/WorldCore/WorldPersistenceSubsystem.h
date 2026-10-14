#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ExcavationTypes.h"
#include "UndergroundHazardTypes.h"
#include "InventoryCore/InventoryTypes.h"
#include "Engine/SaveGame.h"
#include "WorldPersistenceSubsystem.generated.h"

class UExcavationVolumeSubsystem;
class UUndergroundHazardSubsystem;

UENUM(BlueprintType)
enum class EWorldCoreCommandResult : uint8
{
    Applied UMETA(DisplayName = "Applied"),
    AlreadyApplied UMETA(DisplayName = "Already Applied"),
    InvalidCommand UMETA(DisplayName = "Invalid Command"),
    Rejected UMETA(DisplayName = "Rejected")
};

/** Shared, versioned identity for one persisted world state. */
USTRUCT(BlueprintType)
struct MYPROJECT2_API FWorldCoreSaveManifest
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "World Core|Save")
    int32 SchemaVersion = 1;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "World Core|Save")
    FGuid WorldId;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "World Core|Save")
    int32 WorldSeed = 0;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "World Core|Save")
    int64 WorldRevision = 0;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "World Core|Save")
    FString SavedAtUtc;
};

/**
 * Versioned core save envelope.
 *
 * The shared envelope stores domain snapshots as save-boundary transfers; runtime
 * ownership remains in each domain subsystem.
 */
UCLASS()
class MYPROJECT2_API UWorldCoreSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "World Core|Save")
    FWorldCoreSaveManifest Manifest;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "World Core|Save")
    FExcavationVolumeSnapshot Excavation;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "World Core|Save")
    FUndergroundHazardSnapshot UndergroundHazards;

    /** Stackable player resources only; physical ACarvableLog/stone actors remain world actors. */
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "World Core|Save")
    bool bHasPlayerInventory = false;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "World Core|Save")
    TArray<FInventoryItem> PlayerInventoryItems;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "World Core|Save")
    bool bHasStorageInventories = false;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "World Core|Save")
    TArray<FInventoryStorageSnapshot> StorageInventories;

    /** Bounded rolling command ledger used to reject duplicate client retries after reload. */
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "World Core|Save")
    TArray<FGuid> ProcessedCommandIds;

    /** Bounded rolling event keys; commit and rollback are distinct events for the same operation. */
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "World Core|Save")
    TArray<FString> RecentWorldEventKeys;
};

/**
 * Shared world identity, revision/event contract, idempotent excavation command
 * ingress, and versioned core save adapter.
 *
 * UExcavationVolumeSubsystem remains the sole runtime owner of excavation cells.
 * This subsystem observes its events and stores snapshots only at the save boundary.
 */
UCLASS()
class MYPROJECT2_API UWorldPersistenceSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintPure, Category = "World Core")
    FGuid GetWorldId() const { return WorldId; }

    UFUNCTION(BlueprintPure, Category = "World Core")
    int32 GetWorldSeed() const { return WorldSeed; }

    UFUNCTION(BlueprintPure, Category = "World Core")
    int64 GetWorldRevision() const { return WorldRevision; }

    UFUNCTION(BlueprintPure, Category = "World Core")
    bool IsCoreStateDirty() const { return WorldRevision != LastSavedRevision; }

    UFUNCTION(BlueprintPure, Category = "World Core")
    int32 GetRememberedCommandCount() const { return ProcessedCommandIdsInOrder.Num(); }

    /** Execute a sphere edit once per caller-supplied command ID. */
    UFUNCTION(BlueprintCallable, Category = "World Core|Commands")
    EWorldCoreCommandResult ApplySphereEditOnce(
        const FGuid& CommandId,
        const FVector& CenterCm,
        float RadiusCm,
        EExcavationEditType EditType,
        FExcavationEditRecord& OutRecord);

    /** Execute a capsule edit once per caller-supplied command ID. */
    UFUNCTION(BlueprintCallable, Category = "World Core|Commands")
    EWorldCoreCommandResult ApplyCapsuleEditOnce(
        const FGuid& CommandId,
        const FVector& StartCm,
        const FVector& EndCm,
        float RadiusCm,
        EExcavationEditType EditType,
        FExcavationEditRecord& OutRecord);

    /** Execute an axis-aligned box edit once per caller-supplied command ID. */
    UFUNCTION(BlueprintCallable, Category = "World Core|Commands")
    EWorldCoreCommandResult ApplyBoxEditOnce(
        const FGuid& CommandId,
        const FVector& MinWorldCm,
        const FVector& MaxWorldCm,
        EExcavationEditType EditType,
        FExcavationEditRecord& OutRecord);

    /** Save the current core manifest and excavation delta to a named UE save slot. */
    UFUNCTION(BlueprintCallable, Category = "World Core|Persistence")
    bool SaveCoreStateToSlot(const FString& SlotName, int32 UserIndex = 0);

    /**
     * Validate the envelope and ledgers before atomically importing excavation data.
     * Existing excavation state is replaced only after the snapshot importer accepts it.
     */
    UFUNCTION(BlueprintCallable, Category = "World Core|Persistence")
    bool LoadCoreStateFromSlot(const FString& SlotName, int32 UserIndex = 0);

protected:
    UFUNCTION()
    void HandleExcavationChanged(const FExcavationEditRecord& Record, bool bIsRollback);

    UFUNCTION()
    void HandleExcavationVolumeReset();

    UFUNCTION()
    void HandleUndergroundWaterSeepage(const FVector& LocationCm, float VolumeAddedM3);

    UFUNCTION()
    void HandleUndergroundRoofCollapse(const FVector& LocationCm, int32 CellsCollapsed);

    UFUNCTION()
    void HandleUndergroundSupportChanged(const FGuid& SupportId, bool bIsOperational);

    UExcavationVolumeSubsystem* GetExcavationSubsystem() const;
    static bool IsValidSlotName(const FString& SlotName);
    void IncrementWorldRevision();
    void RememberCommandId(const FGuid& CommandId);
    void RememberWorldEventKey(const FString& EventKey);

    static constexpr int32 CurrentSaveSchemaVersion = 3;
    static constexpr int32 MaxRetainedLedgerEntries = 100000;

    UPROPERTY(Transient)
    FGuid WorldId;

    UPROPERTY(Transient)
    int32 WorldSeed = 0;

    UPROPERTY(Transient)
    int64 WorldRevision = 0;

    /** -1 marks a fresh, not-yet-saved core world. */
    UPROPERTY(Transient)
    int64 LastSavedRevision = -1;

    TSet<FGuid> ProcessedCommandIdSet;
    TArray<FGuid> ProcessedCommandIdsInOrder;
    TSet<FString> RecentWorldEventKeySet;
    TArray<FString> RecentWorldEventKeysInOrder;

    /** Suppresses a synthetic revision bump while a saved snapshot is being applied. */
    bool bApplyingLoadedState = false;
};

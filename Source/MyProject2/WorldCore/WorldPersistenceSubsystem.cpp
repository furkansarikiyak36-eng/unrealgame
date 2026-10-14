#include "WorldPersistenceSubsystem.h"

#include "ExcavationVolumeSubsystem.h"
#include "UndergroundHazardSubsystem.h"
#include "ForestCharacter.h"
#include "InventoryCore/InventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/EngineUtils.h"
#include "Engine/World.h"


namespace
{
    AForestCharacter* FindWorldSavePlayer(UWorld* World)
    {
        if (!World)
        {
            return nullptr;
        }
        if (AForestCharacter* Player = Cast<AForestCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0)))
        {
            return Player;
        }
        for (TActorIterator<AForestCharacter> It(World); It; ++It)
        {
            if (IsValid(*It))
            {
                return *It;
            }
        }
        return nullptr;
    }
}

void UWorldPersistenceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UExcavationVolumeSubsystem>();
    Collection.InitializeDependency<UUndergroundHazardSubsystem>();

    WorldId = FGuid::NewGuid();
    WorldSeed = static_cast<int32>(
        (WorldId.A ^ WorldId.B ^ WorldId.C ^ WorldId.D) & 0x7fffffff);
    if (WorldSeed == 0)
    {
        WorldSeed = 1;
    }

    if (UExcavationVolumeSubsystem* Excavation = GetExcavationSubsystem())
    {
        Excavation->OnVolumeChanged.AddDynamic(
            this, &UWorldPersistenceSubsystem::HandleExcavationChanged);
        Excavation->OnVolumeReset.AddDynamic(
            this, &UWorldPersistenceSubsystem::HandleExcavationVolumeReset);
    }

    if (UWorld* World = GetWorld())
    {
        if (UUndergroundHazardSubsystem* Hazards = World->GetSubsystem<UUndergroundHazardSubsystem>())
        {
            Hazards->OnWaterSeepage.AddDynamic(this, &UWorldPersistenceSubsystem::HandleUndergroundWaterSeepage);
            Hazards->OnRoofCollapse.AddDynamic(this, &UWorldPersistenceSubsystem::HandleUndergroundRoofCollapse);
            Hazards->OnSupportChanged.AddDynamic(this, &UWorldPersistenceSubsystem::HandleUndergroundSupportChanged);
        }
    }
}

void UWorldPersistenceSubsystem::Deinitialize()
{
    if (UExcavationVolumeSubsystem* Excavation = GetExcavationSubsystem())
    {
        Excavation->OnVolumeChanged.RemoveDynamic(
            this, &UWorldPersistenceSubsystem::HandleExcavationChanged);
        Excavation->OnVolumeReset.RemoveDynamic(
            this, &UWorldPersistenceSubsystem::HandleExcavationVolumeReset);
    }

    if (UWorld* World = GetWorld())
    {
        if (UUndergroundHazardSubsystem* Hazards = World->GetSubsystem<UUndergroundHazardSubsystem>())
        {
            Hazards->OnWaterSeepage.RemoveDynamic(this, &UWorldPersistenceSubsystem::HandleUndergroundWaterSeepage);
            Hazards->OnRoofCollapse.RemoveDynamic(this, &UWorldPersistenceSubsystem::HandleUndergroundRoofCollapse);
            Hazards->OnSupportChanged.RemoveDynamic(this, &UWorldPersistenceSubsystem::HandleUndergroundSupportChanged);
        }
    }

    ProcessedCommandIdSet.Reset();
    ProcessedCommandIdsInOrder.Reset();
    RecentWorldEventKeySet.Reset();
    RecentWorldEventKeysInOrder.Reset();
    Super::Deinitialize();
}

UExcavationVolumeSubsystem* UWorldPersistenceSubsystem::GetExcavationSubsystem() const
{
    UWorld* World = GetWorld();
    return World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
}

bool UWorldPersistenceSubsystem::IsValidSlotName(const FString& SlotName)
{
    if (SlotName.TrimStartAndEnd().IsEmpty() || SlotName.Len() > 128)
    {
        return false;
    }

    return !SlotName.Contains(TEXT("/")) && !SlotName.Contains(TEXT("\\"));
}

EWorldCoreCommandResult UWorldPersistenceSubsystem::ApplySphereEditOnce(
    const FGuid& CommandId,
    const FVector& CenterCm,
    float RadiusCm,
    EExcavationEditType EditType,
    FExcavationEditRecord& OutRecord)
{
    OutRecord = FExcavationEditRecord();
    if (!CommandId.IsValid())
    {
        return EWorldCoreCommandResult::InvalidCommand;
    }
    if (ProcessedCommandIdSet.Contains(CommandId))
    {
        return EWorldCoreCommandResult::AlreadyApplied;
    }

    UExcavationVolumeSubsystem* Excavation = GetExcavationSubsystem();
    if (!Excavation || !Excavation->ApplySphereEdit(CenterCm, RadiusCm, EditType, OutRecord))
    {
        return EWorldCoreCommandResult::Rejected;
    }

    RememberCommandId(CommandId);
    return EWorldCoreCommandResult::Applied;
}

EWorldCoreCommandResult UWorldPersistenceSubsystem::ApplyCapsuleEditOnce(
    const FGuid& CommandId,
    const FVector& StartCm,
    const FVector& EndCm,
    float RadiusCm,
    EExcavationEditType EditType,
    FExcavationEditRecord& OutRecord)
{
    OutRecord = FExcavationEditRecord();
    if (!CommandId.IsValid())
    {
        return EWorldCoreCommandResult::InvalidCommand;
    }
    if (ProcessedCommandIdSet.Contains(CommandId))
    {
        return EWorldCoreCommandResult::AlreadyApplied;
    }

    UExcavationVolumeSubsystem* Excavation = GetExcavationSubsystem();
    if (!Excavation || !Excavation->ApplyCapsuleEdit(StartCm, EndCm, RadiusCm, EditType, OutRecord))
    {
        return EWorldCoreCommandResult::Rejected;
    }

    RememberCommandId(CommandId);
    return EWorldCoreCommandResult::Applied;
}

EWorldCoreCommandResult UWorldPersistenceSubsystem::ApplyBoxEditOnce(
    const FGuid& CommandId,
    const FVector& MinWorldCm,
    const FVector& MaxWorldCm,
    EExcavationEditType EditType,
    FExcavationEditRecord& OutRecord)
{
    OutRecord = FExcavationEditRecord();
    if (!CommandId.IsValid())
    {
        return EWorldCoreCommandResult::InvalidCommand;
    }
    if (ProcessedCommandIdSet.Contains(CommandId))
    {
        return EWorldCoreCommandResult::AlreadyApplied;
    }

    UExcavationVolumeSubsystem* Excavation = GetExcavationSubsystem();
    if (!Excavation || !Excavation->ApplyBoxEdit(MinWorldCm, MaxWorldCm, EditType, OutRecord))
    {
        return EWorldCoreCommandResult::Rejected;
    }

    RememberCommandId(CommandId);
    return EWorldCoreCommandResult::Applied;
}

void UWorldPersistenceSubsystem::HandleExcavationChanged(
    const FExcavationEditRecord& Record,
    bool bIsRollback)
{
    if (bApplyingLoadedState || !Record.OperationId.IsValid())
    {
        return;
    }

    // Commit and rollback are separate events, even though the reversible receipt
    // intentionally keeps the same operation ID for both directions.
    const FString EventKey = Record.OperationId.ToString(EGuidFormats::Digits)
        + (bIsRollback ? TEXT(":R") : TEXT(":C"));
    if (RecentWorldEventKeySet.Contains(EventKey))
    {
        return;
    }

    RememberWorldEventKey(EventKey);
    IncrementWorldRevision();
}

void UWorldPersistenceSubsystem::HandleExcavationVolumeReset()
{
    if (!bApplyingLoadedState)
    {
        IncrementWorldRevision();
    }
}


void UWorldPersistenceSubsystem::HandleUndergroundWaterSeepage(const FVector& LocationCm, float VolumeAddedM3)
{
    if (!bApplyingLoadedState && FMath::IsFinite(VolumeAddedM3) && VolumeAddedM3 > 0.0f)
    {
        IncrementWorldRevision();
    }
}

void UWorldPersistenceSubsystem::HandleUndergroundRoofCollapse(const FVector& LocationCm, int32 CellsCollapsed)
{
    if (!bApplyingLoadedState && CellsCollapsed > 0)
    {
        IncrementWorldRevision();
    }
}

void UWorldPersistenceSubsystem::HandleUndergroundSupportChanged(const FGuid& SupportId, bool bIsOperational)
{
    if (!bApplyingLoadedState && SupportId.IsValid())
    {
        IncrementWorldRevision();
    }
}

void UWorldPersistenceSubsystem::IncrementWorldRevision()
{
    if (WorldRevision < TNumericLimits<int64>::Max())
    {
        ++WorldRevision;
    }
}

void UWorldPersistenceSubsystem::RememberCommandId(const FGuid& CommandId)
{
    if (!CommandId.IsValid() || ProcessedCommandIdSet.Contains(CommandId))
    {
        return;
    }

    ProcessedCommandIdSet.Add(CommandId);
    ProcessedCommandIdsInOrder.Add(CommandId);

    while (ProcessedCommandIdsInOrder.Num() > MaxRetainedLedgerEntries)
    {
        const FGuid RemovedId = ProcessedCommandIdsInOrder[0];
        ProcessedCommandIdsInOrder.RemoveAt(0, 1, EAllowShrinking::No);
        ProcessedCommandIdSet.Remove(RemovedId);
    }
}

void UWorldPersistenceSubsystem::RememberWorldEventKey(const FString& EventKey)
{
    if (EventKey.IsEmpty() || RecentWorldEventKeySet.Contains(EventKey))
    {
        return;
    }

    RecentWorldEventKeySet.Add(EventKey);
    RecentWorldEventKeysInOrder.Add(EventKey);

    while (RecentWorldEventKeysInOrder.Num() > MaxRetainedLedgerEntries)
    {
        const FString RemovedKey = RecentWorldEventKeysInOrder[0];
        RecentWorldEventKeysInOrder.RemoveAt(0, 1, EAllowShrinking::No);
        RecentWorldEventKeySet.Remove(RemovedKey);
    }
}

bool UWorldPersistenceSubsystem::SaveCoreStateToSlot(const FString& SlotName, int32 UserIndex)
{
    if (!IsValidSlotName(SlotName) || UserIndex < 0 || !WorldId.IsValid())
    {
        return false;
    }

    UExcavationVolumeSubsystem* Excavation = GetExcavationSubsystem();
    if (!Excavation)
    {
        return false;
    }

    UWorldCoreSaveGame* SaveObject = Cast<UWorldCoreSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UWorldCoreSaveGame::StaticClass()));
    if (!SaveObject)
    {
        return false;
    }

    FExcavationVolumeSnapshot Snapshot;
    if (!Excavation->ExportSnapshot(Snapshot))
    {
        return false;
    }

    SaveObject->Manifest.SchemaVersion = CurrentSaveSchemaVersion;
    SaveObject->Manifest.WorldId = WorldId;
    SaveObject->Manifest.WorldSeed = WorldSeed;
    SaveObject->Manifest.WorldRevision = WorldRevision;
    SaveObject->Manifest.SavedAtUtc = FDateTime::UtcNow().ToIso8601();
    SaveObject->Excavation = MoveTemp(Snapshot);

    UWorld* World = GetWorld();
    UUndergroundHazardSubsystem* Hazards = World ? World->GetSubsystem<UUndergroundHazardSubsystem>() : nullptr;
    FUndergroundHazardSnapshot HazardSnapshot;
    if (!Hazards || !Hazards->ExportSnapshot(HazardSnapshot))
    {
        return false;
    }
    SaveObject->UndergroundHazards = MoveTemp(HazardSnapshot);

    // Preserve the currently playable character's stack inventory in the same
    // versioned envelope. World-piece actors (notably ACarvableLog) stay world-owned.
    if (AForestCharacter* Player = FindWorldSavePlayer(World))
    {
        if (UInventoryComponent* Inventory = Player->GetInventoryComponent())
        {
            SaveObject->bHasPlayerInventory = true;
            const TArray<FInventorySlot> Slots = Inventory->GetSlots();
            for (const FInventorySlot& Slot : Slots)
            {
                if (Slot.IsOccupied() && !Slot.bLocked)
                {
                    SaveObject->PlayerInventoryItems.Add(Slot.Item);
                }
            }
        }
    }

    SaveObject->ProcessedCommandIds = ProcessedCommandIdsInOrder;
    SaveObject->RecentWorldEventKeys = RecentWorldEventKeysInOrder;

    if (!UGameplayStatics::SaveGameToSlot(SaveObject, SlotName, UserIndex))
    {
        return false;
    }

    LastSavedRevision = WorldRevision;
    return true;
}

bool UWorldPersistenceSubsystem::LoadCoreStateFromSlot(const FString& SlotName, int32 UserIndex)
{
    if (!IsValidSlotName(SlotName) || UserIndex < 0
        || !UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
    {
        return false;
    }

    UWorldCoreSaveGame* LoadedSave = Cast<UWorldCoreSaveGame>(
        UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
    if (!LoadedSave)
    {
        return false;
    }

    const FWorldCoreSaveManifest& Manifest = LoadedSave->Manifest;
    if ((Manifest.SchemaVersion != 1 && Manifest.SchemaVersion != 2 && Manifest.SchemaVersion != CurrentSaveSchemaVersion)
        || !Manifest.WorldId.IsValid()
        || Manifest.WorldRevision < 0
        || LoadedSave->ProcessedCommandIds.Num() > MaxRetainedLedgerEntries
        || LoadedSave->RecentWorldEventKeys.Num() > MaxRetainedLedgerEntries)
    {
        return false;
    }

    TSet<FGuid> NewProcessedCommandIdSet;
    NewProcessedCommandIdSet.Reserve(LoadedSave->ProcessedCommandIds.Num());
    for (const FGuid& CommandId : LoadedSave->ProcessedCommandIds)
    {
        if (!CommandId.IsValid() || NewProcessedCommandIdSet.Contains(CommandId))
        {
            return false;
        }
        NewProcessedCommandIdSet.Add(CommandId);
    }

    TSet<FString> NewRecentWorldEventKeySet;
    NewRecentWorldEventKeySet.Reserve(LoadedSave->RecentWorldEventKeys.Num());
    for (const FString& EventKey : LoadedSave->RecentWorldEventKeys)
    {
        if (EventKey.IsEmpty() || EventKey.Len() > 64
            || NewRecentWorldEventKeySet.Contains(EventKey))
        {
            return false;
        }
        NewRecentWorldEventKeySet.Add(EventKey);
    }

    UExcavationVolumeSubsystem* Excavation = GetExcavationSubsystem();
    UWorld* LoadWorld = GetWorld();
    UUndergroundHazardSubsystem* Hazards = LoadWorld ? LoadWorld->GetSubsystem<UUndergroundHazardSubsystem>() : nullptr;
    if (!Excavation || !Hazards || !Hazards->ValidateSnapshot(LoadedSave->UndergroundHazards))
    {
        return false;
    }

    AForestCharacter* SavedPlayer = nullptr;
    UInventoryComponent* PlayerInventory = nullptr;
    TArray<FInventoryItem> PreviousPlayerInventory;
    if (LoadedSave->bHasPlayerInventory)
    {
        SavedPlayer = FindWorldSavePlayer(LoadWorld);
        PlayerInventory = SavedPlayer ? SavedPlayer->GetInventoryComponent() : nullptr;
        // Validate the full inventory snapshot before any domain starts importing.
        if (!PlayerInventory || !PlayerInventory->CanRestoreFromSavedItems(LoadedSave->PlayerInventoryItems))
        {
            return false;
        }
        const TArray<FInventorySlot> ExistingSlots = PlayerInventory->GetSlots();
        for (const FInventorySlot& Slot : ExistingSlots)
        {
            if (Slot.IsOccupied() && !Slot.bLocked)
            {
                PreviousPlayerInventory.Add(Slot.Item);
            }
        }
    }

    // Each domain validates its snapshot before replacing its owned runtime state.
    // Suppress the import-reset revision so the saved revision remains authoritative.
    bApplyingLoadedState = true;
    const bool bImported = Excavation->ImportSnapshot(LoadedSave->Excavation, true);
    if (!bImported)
    {
        bApplyingLoadedState = false;
        return false;
    }

    const bool bHazardsImported = Hazards->ImportSnapshot(LoadedSave->UndergroundHazards);
    if (!bHazardsImported)
    {
        bApplyingLoadedState = false;
        return false;
    }

    // Inventory was preflighted against the exact active slot/weight policy above.
    // Restore it only after the other snapshots have accepted their own state.
    if (PlayerInventory && !PlayerInventory->RestoreFromSavedItems(LoadedSave->PlayerInventoryItems))
    {
        UE_LOG(LogTemp, Error, TEXT("Core save load: prevalidated player inventory restore failed."));
        if (!PlayerInventory->RestoreFromSavedItems(PreviousPlayerInventory))
        {
            UE_LOG(LogTemp, Error, TEXT("Core save load: player inventory rollback also failed."));
        }
        bApplyingLoadedState = false;
        return false;
    }
    bApplyingLoadedState = false;

    WorldId = Manifest.WorldId;
    WorldSeed = Manifest.WorldSeed;
    WorldRevision = Manifest.WorldRevision;
    LastSavedRevision = Manifest.WorldRevision;

    ProcessedCommandIdSet = MoveTemp(NewProcessedCommandIdSet);
    ProcessedCommandIdsInOrder = LoadedSave->ProcessedCommandIds;
    RecentWorldEventKeySet = MoveTemp(NewRecentWorldEventKeySet);
    RecentWorldEventKeysInOrder = LoadedSave->RecentWorldEventKeys;
    return true;
}

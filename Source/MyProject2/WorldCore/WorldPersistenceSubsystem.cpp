#include "WorldPersistenceSubsystem.h"

#include "ExcavationVolumeSubsystem.h"
#include "UndergroundHazardSubsystem.h"
#include "WorldStateSubsystem.h"
#include "RegionLogisticsSubsystem.h"
#include "Phase11_Production/ProductionChainComponent.h"
#include "ForestCharacter.h"
#include "InventoryCore/InventoryComponent.h"
#include "InventoryCore/InventoryStorageActor.h"
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
    Collection.InitializeDependency<URegionLogisticsSubsystem>();

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
        if (URegionLogisticsSubsystem* Logistics = World->GetSubsystem<URegionLogisticsSubsystem>())
        {
            Logistics->OnLogisticsChanged.AddDynamic(this, &UWorldPersistenceSubsystem::HandleRegionLogisticsChanged);
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
        if (URegionLogisticsSubsystem* Logistics = World->GetSubsystem<URegionLogisticsSubsystem>())
        {
            Logistics->OnLogisticsChanged.RemoveDynamic(this, &UWorldPersistenceSubsystem::HandleRegionLogisticsChanged);
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

void UWorldPersistenceSubsystem::HandleRegionLogisticsChanged(int64 NewRevision)
{
    (void)NewRevision;
    if (!bApplyingLoadedState)
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

void UWorldPersistenceSubsystem::NotifyProductionChanged(FName StationKey)
{
    if (!bApplyingLoadedState && !StationKey.IsNone())
    {
        IncrementWorldRevision();
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

    UWorldStateSubsystem* WorldState = World ? World->GetSubsystem<UWorldStateSubsystem>() : nullptr;
    FWorldStateSnapshot WorldStateSnapshot;
    if (!WorldState || !WorldState->ExportSnapshot(WorldStateSnapshot))
    {
        UE_LOG(LogTemp, Error, TEXT("Core save rejected: world-state snapshot failed validation."));
        return false;
    }
    SaveObject->bHasWorldState = true;
    SaveObject->WorldState = MoveTemp(WorldStateSnapshot);

    URegionLogisticsSubsystem* Logistics = World ? World->GetSubsystem<URegionLogisticsSubsystem>() : nullptr;
    FRegionLogisticsSnapshot RegionLogisticsSnapshot;
    if (!Logistics || !Logistics->ExportSnapshot(RegionLogisticsSnapshot))
    {
        UE_LOG(LogTemp, Error, TEXT("Core save rejected: region-logistics snapshot failed validation."));
        return false;
    }
    SaveObject->bHasRegionLogistics = true;
    SaveObject->RegionLogistics = MoveTemp(RegionLogisticsSnapshot);

    SaveObject->bHasProductionChains = true;
    TSet<FName> SeenProductionStationKeys;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!IsValid(Actor))
        {
            continue;
        }
        TArray<UProductionChainComponent*> ActorProductionChains;
        Actor->GetComponents<UProductionChainComponent>(ActorProductionChains);
        for (UProductionChainComponent* Production : ActorProductionChains)
        {
            if (!IsValid(Production))
            {
                continue;
            }
            const FName StationKey = Production->GetStableStationKey();
            FProductionChainSnapshot ProductionSnapshot;
            if (StationKey.IsNone() || SeenProductionStationKeys.Contains(StationKey)
                || SaveObject->ProductionChains.Num() >= MaxProductionStationsInSave
                || !Production->ExportSnapshot(ProductionSnapshot))
            {
                UE_LOG(LogTemp, Error, TEXT("Core save rejected: production station keys must be valid/unique and queue snapshots valid."));
                return false;
            }
            SeenProductionStationKeys.Add(StationKey);
            SaveObject->ProductionChains.Add(MoveTemp(ProductionSnapshot));
        }
    }
    SaveObject->ProductionChains.Sort([](const FProductionChainSnapshot& A, const FProductionChainSnapshot& B)
    {
        return A.StationKey.ToString() < B.StationKey.ToString();
    });

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

    SaveObject->bHasStorageInventories = true;
    TSet<FName> SeenStorageKeys;
    for (TActorIterator<AInventoryStorageActor> It(World); It; ++It)
    {
        AInventoryStorageActor* Storage = *It;
        if (!IsValid(Storage) || !Storage->GetInventory())
        {
            return false;
        }

        const FName StorageKey = Storage->GetStorageKey();
        if (StorageKey.IsNone() || SeenStorageKeys.Contains(StorageKey))
        {
            UE_LOG(LogTemp, Error, TEXT("Core save rejected: storage keys must be valid and unique."));
            return false;
        }
        SeenStorageKeys.Add(StorageKey);

        FInventoryStorageSnapshot StorageSnapshot;
        StorageSnapshot.StorageKey = StorageKey;
        for (const FInventorySlot& Slot : Storage->GetInventory()->GetSlots())
        {
            if (Slot.IsOccupied() && !Slot.bLocked)
            {
                StorageSnapshot.Items.Add(Slot.Item);
            }
        }
        SaveObject->StorageInventories.Add(MoveTemp(StorageSnapshot));
    }
    SaveObject->StorageInventories.Sort([](const FInventoryStorageSnapshot& A, const FInventoryStorageSnapshot& B)
    {
        return A.StorageKey.ToString() < B.StorageKey.ToString();
    });

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
    if ((Manifest.SchemaVersion != 1 && Manifest.SchemaVersion != 2 && Manifest.SchemaVersion != 3 && Manifest.SchemaVersion != 4 && Manifest.SchemaVersion != 5 && Manifest.SchemaVersion != CurrentSaveSchemaVersion)
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
    UWorldStateSubsystem* WorldState = LoadWorld ? LoadWorld->GetSubsystem<UWorldStateSubsystem>() : nullptr;
    URegionLogisticsSubsystem* Logistics = LoadWorld ? LoadWorld->GetSubsystem<URegionLogisticsSubsystem>() : nullptr;
    if (!Excavation || !Hazards || !Hazards->ValidateSnapshot(LoadedSave->UndergroundHazards) ||
        (LoadedSave->bHasWorldState && (!WorldState || !WorldState->ValidateSnapshot(LoadedSave->WorldState))) ||
        (LoadedSave->bHasRegionLogistics && (!Logistics || !Logistics->ValidateSnapshot(LoadedSave->RegionLogistics))) ||
        LoadedSave->ProductionChains.Num() > MaxProductionStationsInSave)
    {
        return false;
    }

    TMap<FName, UProductionChainComponent*> ProductionChainsByKey;
    TMap<FName, FProductionChainSnapshot> PreviousProductionSnapshots;
    for (TActorIterator<AActor> It(LoadWorld); It; ++It)
    {
        AActor* Actor = *It;
        if (!IsValid(Actor))
        {
            continue;
        }
        TArray<UProductionChainComponent*> ActorProductionChains;
        Actor->GetComponents<UProductionChainComponent>(ActorProductionChains);
        for (UProductionChainComponent* Production : ActorProductionChains)
        {
            if (!IsValid(Production))
            {
                return false;
            }
            const FName StationKey = Production->GetStableStationKey();
            FProductionChainSnapshot PreviousSnapshot;
            if (StationKey.IsNone() || ProductionChainsByKey.Contains(StationKey)
                || !Production->ExportSnapshot(PreviousSnapshot))
            {
                UE_LOG(LogTemp, Error, TEXT("Core load rejected: active production station keys must be valid and unique."));
                return false;
            }
            ProductionChainsByKey.Add(StationKey, Production);
            PreviousProductionSnapshots.Add(StationKey, MoveTemp(PreviousSnapshot));
        }
    }
    if (LoadedSave->bHasProductionChains)
    {
        if (LoadedSave->ProductionChains.Num() != ProductionChainsByKey.Num())
        {
            return false;
        }
        TSet<FName> SnapshotStationKeys;
        for (const FProductionChainSnapshot& ProductionSnapshot : LoadedSave->ProductionChains)
        {
            if (ProductionSnapshot.StationKey.IsNone() || SnapshotStationKeys.Contains(ProductionSnapshot.StationKey))
            {
                return false;
            }
            UProductionChainComponent* const* Found = ProductionChainsByKey.Find(ProductionSnapshot.StationKey);
            if (!Found || !IsValid(*Found) || !(*Found)->ValidateSnapshot(ProductionSnapshot))
            {
                return false;
            }
            SnapshotStationKeys.Add(ProductionSnapshot.StationKey);
        }
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
        for (const FInventorySlot& Slot : PlayerInventory->GetSlots())
        {
            if (Slot.IsOccupied() && !Slot.bLocked)
            {
                PreviousPlayerInventory.Add(Slot.Item);
            }
        }
    }

    TMap<FName, AInventoryStorageActor*> StorageActorsByKey;
    TMap<FName, TArray<FInventoryItem>> PreviousStorageItems;
    for (TActorIterator<AInventoryStorageActor> It(LoadWorld); It; ++It)
    {
        AInventoryStorageActor* Storage = *It;
        if (!IsValid(Storage) || !Storage->GetInventory())
        {
            return false;
        }
        const FName Key = Storage->GetStorageKey();
        if (Key.IsNone() || StorageActorsByKey.Contains(Key))
        {
            UE_LOG(LogTemp, Error, TEXT("Core load rejected: active storage keys must be valid and unique."));
            return false;
        }
        StorageActorsByKey.Add(Key, Storage);

        TArray<FInventoryItem> ExistingItems;
        for (const FInventorySlot& Slot : Storage->GetInventory()->GetSlots())
        {
            if (Slot.IsOccupied() && !Slot.bLocked)
            {
                ExistingItems.Add(Slot.Item);
            }
        }
        PreviousStorageItems.Add(Key, MoveTemp(ExistingItems));
    }

    if (LoadedSave->bHasStorageInventories)
    {
        if (LoadedSave->StorageInventories.Num() != StorageActorsByKey.Num())
        {
            return false;
        }

        TSet<FName> SnapshotStorageKeys;
        for (const FInventoryStorageSnapshot& StorageSnapshot : LoadedSave->StorageInventories)
        {
            if (StorageSnapshot.StorageKey.IsNone() ||
                SnapshotStorageKeys.Contains(StorageSnapshot.StorageKey))
            {
                return false;
            }
            SnapshotStorageKeys.Add(StorageSnapshot.StorageKey);

            AInventoryStorageActor* const* FoundStorage = StorageActorsByKey.Find(StorageSnapshot.StorageKey);
            if (!FoundStorage || !IsValid(*FoundStorage) ||
                !(*FoundStorage)->GetInventory()->CanRestoreFromSavedItems(StorageSnapshot.Items))
            {
                return false;
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

    if (LoadedSave->bHasWorldState && !WorldState->ImportSnapshot(LoadedSave->WorldState))
    {
        UE_LOG(LogTemp, Error, TEXT("Core save load: prevalidated world-state import failed."));
        bApplyingLoadedState = false;
        return false;
    }

    // Inventory snapshots are restored only after all domain snapshots have accepted state.
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

    if (LoadedSave->bHasStorageInventories)
    {
        for (const FInventoryStorageSnapshot& StorageSnapshot : LoadedSave->StorageInventories)
        {
            AInventoryStorageActor* const* FoundStorage = StorageActorsByKey.Find(StorageSnapshot.StorageKey);
            AInventoryStorageActor* Storage = FoundStorage ? *FoundStorage : nullptr;
            if (!Storage || !Storage->GetInventory()->RestoreFromSavedItems(StorageSnapshot.Items))
            {
                UE_LOG(LogTemp, Error, TEXT("Core save load: storage inventory restore failed for key '%s'."),
                    *StorageSnapshot.StorageKey.ToString());
                if (PlayerInventory)
                {
                    PlayerInventory->RestoreFromSavedItems(PreviousPlayerInventory);
                }
                for (const TPair<FName, TArray<FInventoryItem>>& Previous : PreviousStorageItems)
                {
                    if (AInventoryStorageActor* const* PreviousActor = StorageActorsByKey.Find(Previous.Key))
                    {
                        (*PreviousActor)->GetInventory()->RestoreFromSavedItems(Previous.Value);
                    }
                }
                bApplyingLoadedState = false;
                return false;
            }
        }
    }
    if (LoadedSave->bHasRegionLogistics && !Logistics->ImportSnapshot(LoadedSave->RegionLogistics))
    {
        UE_LOG(LogTemp, Error, TEXT("Core save load: prevalidated region-logistics import failed."));
        bApplyingLoadedState = false;
        return false;
    }
    if (LoadedSave->bHasProductionChains)
    {
        bool bProductionImported = true;
        for (const FProductionChainSnapshot& ProductionSnapshot : LoadedSave->ProductionChains)
        {
            UProductionChainComponent* const* Found = ProductionChainsByKey.Find(ProductionSnapshot.StationKey);
            if (!Found || !IsValid(*Found) || !(*Found)->ImportSnapshot(ProductionSnapshot))
            {
                bProductionImported = false;
                break;
            }
        }
        if (!bProductionImported)
        {
            UE_LOG(LogTemp, Error, TEXT("Core save load: validated production queue import failed; restoring previous production queues/inventory."));
            if (PlayerInventory)
            {
                PlayerInventory->RestoreFromSavedItems(PreviousPlayerInventory);
            }
            for (const TPair<FName, TArray<FInventoryItem>>& Previous : PreviousStorageItems)
            {
                if (AInventoryStorageActor* const* PreviousActor = StorageActorsByKey.Find(Previous.Key))
                {
                    (*PreviousActor)->GetInventory()->RestoreFromSavedItems(Previous.Value);
                }
            }
            for (const TPair<FName, FProductionChainSnapshot>& Previous : PreviousProductionSnapshots)
            {
                if (UProductionChainComponent* const* PreviousComponent = ProductionChainsByKey.Find(Previous.Key))
                {
                    (*PreviousComponent)->ImportSnapshot(Previous.Value);
                }
            }
            bApplyingLoadedState = false;
            return false;
        }
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

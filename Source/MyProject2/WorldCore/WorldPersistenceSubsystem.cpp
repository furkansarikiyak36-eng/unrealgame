#include "WorldPersistenceSubsystem.h"

#include "ExcavationVolumeSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

void UWorldPersistenceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UExcavationVolumeSubsystem>();

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
    if (Manifest.SchemaVersion != CurrentSaveSchemaVersion
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
    if (!Excavation)
    {
        return false;
    }

    // The importer validates the full sparse snapshot before its single state swap.
    // Suppress the import-reset revision so the saved revision remains authoritative.
    bApplyingLoadedState = true;
    const bool bImported = Excavation->ImportSnapshot(LoadedSave->Excavation, true);
    bApplyingLoadedState = false;
    if (!bImported)
    {
        return false;
    }

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

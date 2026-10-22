#include "Phase7_Integration/SaveGameSubsystem.h"

#include "Phase1_Physical/LogActor.h"
#include "Core/LogDefinition.h"
#include "ForestCharacter.h"
#include "SurvivalComponent.h"
#include "Survival/UndergroundExposureComponent.h"
#include "WorldCore/ExcavationVolumeSubsystem.h"
#include "Shoring/ExcavationShoringSubsystem.h"
#include "Shoring/PlaceableShoringPiece.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace
{
    bool IsFiniteTransform(const FTransform& Transform)
    {
        const FVector Location = Transform.GetLocation();
        const FVector Scale = Transform.GetScale3D();
        const FQuat Rotation = Transform.GetRotation();
        return FMath::IsFinite(Location.X) && FMath::IsFinite(Location.Y) && FMath::IsFinite(Location.Z)
            && FMath::IsFinite(Scale.X) && FMath::IsFinite(Scale.Y) && FMath::IsFinite(Scale.Z)
            && Scale.X > 0.0f && Scale.Y > 0.0f && Scale.Z > 0.0f
            && FMath::IsFinite(Rotation.X) && FMath::IsFinite(Rotation.Y)
            && FMath::IsFinite(Rotation.Z) && FMath::IsFinite(Rotation.W)
            && FMath::IsNearlyEqual(Rotation.SizeSquared(), 1.0, 0.01);
    }

    void CaptureLogActors(UWorld* World, TArray<FLogSaveData>& OutLogs)
    {
        OutLogs.Reset();
        for (TActorIterator<ALogActor> It(World); It; ++It)
        {
            ALogActor* Log = *It;
            if (!IsValid(Log) || Log->GetLogState() == ELogState::Destroyed)
            {
                continue;
            }
            FLogSaveData Data;
            Data.Transform = Log->GetActorTransform();
            Data.State = static_cast<uint8>(Log->GetLogState());
            Data.CurrentLength = Log->GetCurrentLength();
            Data.Shape = static_cast<uint8>(Log->GetCurrentShape());
            Data.Definition = Log->GetDefinition();
            Data.Health = Log->GetHealth();
            OutLogs.Add(Data);
        }
    }

    bool StageLogs(
        UWorld* World,
        TSubclassOf<ALogActor> LogClass,
        const TArray<FLogSaveData>& DataRows,
        TArray<ALogActor*>& OutStaged,
        FString& OutError)
    {
        OutStaged.Reset();
        OutError.Reset();
        if (DataRows.Num() > 100000)
        {
            OutError = TEXT("Saved log actor count is above the restore limit.");
            return false;
        }
        OutStaged.Reserve(DataRows.Num());
        for (const FLogSaveData& Data : DataRows)
        {
            if (!IsFiniteTransform(Data.Transform) || !FMath::IsFinite(Data.Health)
                || Data.Health < 0.0f || Data.Health > 100.0f
                || !FMath::IsFinite(Data.CurrentLength) || Data.CurrentLength <= 0.0f
                || Data.CurrentLength > 100000.0f
                || Data.State > static_cast<uint8>(ELogState::Destroyed)
                || Data.Shape > static_cast<uint8>(EWoodShape::Firewood))
            {
                OutError = TEXT("A saved log record contains invalid transform, health, length, state or shape.");
                for (ALogActor* Staged : OutStaged) if (IsValid(Staged)) Staged->Destroy();
                OutStaged.Reset();
                return false;
            }

            UClass* SpawnClass = LogClass.Get();
            if (!SpawnClass) SpawnClass = ALogActor::StaticClass();
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            ALogActor* Log = World->SpawnActor<ALogActor>(SpawnClass, Data.Transform, Params);
            if (!IsValid(Log))
            {
                OutError = TEXT("Failed to stage a log actor; existing world actors were not removed.");
                for (ALogActor* Staged : OutStaged) if (IsValid(Staged)) Staged->Destroy();
                OutStaged.Reset();
                return false;
            }

            if (Data.Definition) Log->InitializeFromDefinition(Data.Definition);
            Log->SetActorTransform(Data.Transform);
            Log->ApplyCut(static_cast<EWoodShape>(Data.Shape), Data.CurrentLength);
            Log->SetHealth(Data.Health);
            if (!Log->SetLogState(static_cast<ELogState>(Data.State)))
            {
                Log->Destroy();
                OutError = TEXT("A staged log actor rejected its saved state.");
                for (ALogActor* Staged : OutStaged) if (IsValid(Staged)) Staged->Destroy();
                OutStaged.Reset();
                return false;
            }
            OutStaged.Add(Log);
        }
        return true;
    }

    void DestroyActors(const TArray<ALogActor*>& Actors)
    {
        for (ALogActor* Actor : Actors)
        {
            if (IsValid(Actor)) Actor->Destroy();
        }
    }
}

bool USaveGameSubsystem::SaveWorld(const FString& SlotName)
{
    LastOperationError.Reset();
    UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
    if (!World || SlotName.IsEmpty())
    {
        LastOperationError = TEXT("Saving requires a valid world and non-empty slot name.");
        return false;
    }

    UExcavationVolumeSubsystem* Volume = World->GetSubsystem<UExcavationVolumeSubsystem>();
    UExcavationShoringSubsystem* Shoring = World->GetSubsystem<UExcavationShoringSubsystem>();
    if (!Volume || !Shoring)
    {
        LastOperationError = TEXT("World persistence requires the authoritative voxel and shoring subsystems.");
        return false;
    }

    UWorldSaveGame* Save = Cast<UWorldSaveGame>(UGameplayStatics::CreateSaveGameObject(UWorldSaveGame::StaticClass()));
    if (!Save)
    {
        LastOperationError = TEXT("Could not allocate the versioned world save object.");
        return false;
    }
    Save->SaveSchemaVersion = 2;
    Save->MapPackageName = FName(*World->GetMapName());
    CaptureLogActors(World, Save->Logs);

    FString Error;
    if (!Volume->ExportSnapshot(Save->Excavation))
    {
        LastOperationError = TEXT("Could not export voxel density snapshot (grid or snapshot budget invalid).");
        return false;
    }
    if (!Shoring->ExportSnapshot(Save->Shoring, Error))
    {
        LastOperationError = FString::Printf(TEXT("Could not export shoring snapshot: %s"), *Error);
        return false;
    }

    AForestCharacter* Player = nullptr;
    for (TActorIterator<AForestCharacter> It(World); It; ++It)
    {
        if (IsValid(*It))
        {
            Player = *It;
            break;
        }
    }
    Save->bHasPlayerState = IsValid(Player);
    if (Player)
    {
        if (!Player->CanSaveWorldState(Error))
        {
            LastOperationError = Error;
            return false;
        }
        if (!IsFiniteTransform(Player->GetActorTransform())
            || !IsFiniteTransform(Player->GetRespawnTransformForSave()))
        {
            LastOperationError = TEXT("Player transform or respawn transform is invalid.");
            return false;
        }
        Save->PlayerTransform = Player->GetActorTransform();
        Save->RespawnTransform = Player->GetRespawnTransformForSave();
        Player->ExportInventorySaveData(Save->InventoryItemIds, Save->InventoryCounts);
        if (!AForestCharacter::ValidateInventorySaveData(Save->InventoryItemIds, Save->InventoryCounts, Error))
        {
            LastOperationError = FString::Printf(TEXT("Could not export player inventory: %s"), *Error);
            return false;
        }

        USurvivalComponent* Survival = Player->GetSurvivalMutable();
        UUndergroundExposureComponent* Exposure = Player->GetExposureMutable();
        if (!Survival || !Exposure)
        {
            LastOperationError = TEXT("Player is missing a required survival/exposure component.");
            return false;
        }
        Save->Survival = Survival->CaptureSnapshot();
        if (!Save->Survival.Validate(Error))
        {
            LastOperationError = FString::Printf(TEXT("Player survival state is invalid: %s"), *Error);
            return false;
        }
        Save->bHasExposureState = true;
        Save->Exposure = Exposure->CaptureSnapshot();
        if (!Exposure->ValidateSnapshot(Save->Exposure, Error))
        {
            LastOperationError = FString::Printf(TEXT("Player exposure state is invalid: %s"), *Error);
            return false;
        }
    }

    if (!UGameplayStatics::SaveGameToSlot(Save, SlotName, 0))
    {
        LastOperationError = TEXT("SaveGameToSlot failed.");
        return false;
    }
    CachedLogs = Save->Logs;
    return true;
}

bool USaveGameSubsystem::LoadWorld(const FString& SlotName)
{
    LastOperationError.Reset();
    UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
    if (!World || SlotName.IsEmpty() || !UGameplayStatics::DoesSaveGameExist(SlotName, 0))
    {
        LastOperationError = TEXT("Loading requires a valid world and an existing slot.");
        return false;
    }

    USaveGame* LoadedObject = UGameplayStatics::LoadGameFromSlot(SlotName, 0);
    ULogSaveGame* LogSave = Cast<ULogSaveGame>(LoadedObject);
    if (!LogSave)
    {
        LastOperationError = TEXT("The slot is not a recognized MyProject2 save.");
        return false;
    }

    if (UWorldSaveGame* CompleteSave = Cast<UWorldSaveGame>(LoadedObject))
    {
        return LoadCompleteWorld(CompleteSave, World, LastOperationError);
    }
    return LoadLegacyLogs(LogSave, World, LastOperationError);
}

bool USaveGameSubsystem::LoadLegacyLogs(ULogSaveGame* Save, UWorld* World, FString& OutError)
{
    OutError.Reset();
    if (!IsValid(Save) || !IsValid(World))
    {
        OutError = TEXT("Legacy log save or world is invalid.");
        return false;
    }

    TArray<ALogActor*> StagedLogs;
    if (!StageLogs(World, LogClass, Save->Logs, StagedLogs, OutError))
    {
        return false;
    }

    TArray<ALogActor*> OldLogs;
    for (TActorIterator<ALogActor> It(World); It; ++It)
    {
        if (IsValid(*It) && !StagedLogs.Contains(*It)) OldLogs.Add(*It);
    }
    for (ALogActor* Old : OldLogs) Old->Destroy();
    CachedLogs = Save->Logs;
    return true;
}

bool USaveGameSubsystem::LoadCompleteWorld(UWorldSaveGame* Save, UWorld* World, FString& OutError)
{
    OutError.Reset();
    if (!IsValid(Save) || !IsValid(World))
    {
        OutError = TEXT("Complete save or world is invalid.");
        return false;
    }
    if (Save->SaveSchemaVersion != 2)
    {
        OutError = TEXT("Unsupported complete-world save schema version.");
        return false;
    }
    if (Save->MapPackageName != NAME_None && Save->MapPackageName != FName(*World->GetMapName()))
    {
        OutError = TEXT("The save belongs to a different map. Cross-map restore is not supported.");
        return false;
    }

    UExcavationVolumeSubsystem* Volume = World->GetSubsystem<UExcavationVolumeSubsystem>();
    UExcavationShoringSubsystem* Shoring = World->GetSubsystem<UExcavationShoringSubsystem>();
    if (!Volume || !Shoring)
    {
        OutError = TEXT("Complete-world restore requires the voxel and shoring subsystems.");
        return false;
    }

    FString Error;
    if (!Volume->ValidateImportSnapshot(Save->Excavation, true, Error))
    {
        OutError = FString::Printf(TEXT("Voxel snapshot validation failed: %s"), *Error);
        return false;
    }
    if (!Shoring->ValidateSnapshot(Save->Shoring, Error))
    {
        OutError = FString::Printf(TEXT("Shoring snapshot validation failed: %s"), *Error);
        return false;
    }

    AForestCharacter* Player = nullptr;
    for (TActorIterator<AForestCharacter> It(World); It; ++It)
    {
        if (IsValid(*It))
        {
            Player = *It;
            break;
        }
    }
    if (Save->bHasPlayerState)
    {
        if (!Player)
        {
            OutError = TEXT("Save contains a player state, but this world has no AForestCharacter.");
            return false;
        }
        if (!IsFiniteTransform(Save->PlayerTransform) || !IsFiniteTransform(Save->RespawnTransform)
            || !Save->Survival.Validate(Error)
            || !AForestCharacter::ValidateInventorySaveData(Save->InventoryItemIds, Save->InventoryCounts, Error))
        {
            OutError = FString::Printf(TEXT("Player snapshot validation failed: %s"), *Error);
            return false;
        }
        if (!Player->GetSurvivalMutable() || !Player->GetExposureMutable())
        {
            OutError = TEXT("Player is missing a required survival/exposure component.");
            return false;
        }
        if (Save->bHasExposureState && !Player->GetExposureMutable()->ValidateSnapshot(Save->Exposure, Error))
        {
            OutError = FString::Printf(TEXT("Exposure snapshot validation failed: %s"), *Error);
            return false;
        }
    }

    // Capture rollback data before any authoritative state changes.
    FExcavationVolumeSnapshot OldVoxel;
    FShoringWorldSnapshot OldShoring;
    if (!Volume->ExportSnapshot(OldVoxel) || !Shoring->ExportSnapshot(OldShoring, Error))
    {
        OutError = Error.IsEmpty() ? TEXT("Could not capture rollback snapshots for transactional restore.") : Error;
        return false;
    }

    FSurvivalComponentSnapshot OldSurvival;
    FSurvivalExposureSnapshot OldExposure;
    FTransform OldPlayerTransform = FTransform::Identity;
    FTransform OldRespawnTransform = FTransform::Identity;
    TArray<uint8> OldInventoryIds;
    TArray<int32> OldInventoryCounts;
    if (Player)
    {
        OldSurvival = Player->GetSurvivalMutable()->CaptureSnapshot();
        OldExposure = Player->GetExposureMutable()->CaptureSnapshot();
        OldPlayerTransform = Player->GetActorTransform();
        OldRespawnTransform = Player->GetRespawnTransformForSave();
        Player->ExportInventorySaveData(OldInventoryIds, OldInventoryCounts);
    }

    TArray<ALogActor*> StagedLogs;
    if (!StageLogs(World, LogClass, Save->Logs, StagedLogs, OutError))
    {
        return false;
    }

    if (!Volume->ImportSnapshot(Save->Excavation, true))
    {
        DestroyActors(StagedLogs);
        OutError = TEXT("Voxel snapshot import failed; no shoring/player/log replacements were committed.");
        return false;
    }

    if (!Shoring->ImportSnapshot(Save->Shoring, OutError))
    {
        Volume->ImportSnapshot(OldVoxel, true);
        DestroyActors(StagedLogs);
        OutError = FString::Printf(TEXT("Shoring restore failed; voxel snapshot was rolled back. %s"), *OutError);
        return false;
    }

    if (Save->bHasPlayerState && Player)
    {
        bool bPlayerRestored = Player->GetSurvivalMutable()->RestoreSnapshot(Save->Survival, Error)
            && (!Save->bHasExposureState || Player->GetExposureMutable()->RestoreSnapshot(Save->Exposure, Error))
            && Player->ImportInventorySaveData(Save->InventoryItemIds, Save->InventoryCounts, Error);
        if (bPlayerRestored)
        {
            Player->SetRespawnTransformForSave(Save->RespawnTransform);
            Player->SetActorTransform(Save->PlayerTransform, false, nullptr, ETeleportType::TeleportPhysics);
        }
        if (!bPlayerRestored)
        {
            Player->GetSurvivalMutable()->RestoreSnapshot(OldSurvival, Error);
            Player->GetExposureMutable()->RestoreSnapshot(OldExposure, Error);
            Player->ImportInventorySaveData(OldInventoryIds, OldInventoryCounts, Error);
            Player->SetRespawnTransformForSave(OldRespawnTransform);
            Player->SetActorTransform(OldPlayerTransform, false, nullptr, ETeleportType::TeleportPhysics);
            Shoring->ImportSnapshot(OldShoring, Error);
            Volume->ImportSnapshot(OldVoxel, true);
            DestroyActors(StagedLogs);
            OutError = TEXT("Player state restore failed; voxel/shoring/player state was rolled back.");
            return false;
        }
    }

    TArray<ALogActor*> OldLogs;
    for (TActorIterator<ALogActor> It(World); It; ++It)
    {
        if (IsValid(*It) && !StagedLogs.Contains(*It)) OldLogs.Add(*It);
    }
    for (ALogActor* Old : OldLogs) Old->Destroy();

    CachedLogs = Save->Logs;
    return true;
}

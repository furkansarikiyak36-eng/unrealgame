#include "Phase7_Integration/SaveGameSubsystem.h"

#include "Phase1_Physical/LogActor.h"
#include "Core/LogDefinition.h"
#include "ForestCharacter.h"
#include "StoneCore/StoneDefinition.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

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

    template<typename TActor>
    void DestroyStaged(TArray<TActor*>& Actors)
    {
        for (TActor* Actor : Actors)
        {
            if (IsValid(Actor))
            {
                Actor->Destroy();
            }
        }
        Actors.Reset();
    }

    void CaptureLegacyLogs(UWorld* World, TArray<FLogSaveData>& OutLogs)
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

    bool CaptureCarvableLogs(UWorld* World, TArray<FCarvableLogSaveData>& OutLogs, FString& OutError)
    {
        OutLogs.Reset();
        OutError.Reset();
        for (TActorIterator<ACarvableLog> It(World); It; ++It)
        {
            ACarvableLog* Log = *It;
            if (!IsValid(Log))
            {
                continue;
            }
            if (!Log->IsReadyShapedForPersistence())
            {
                OutError = FString::Printf(
                    TEXT("Save blocked: carvable log '%s' is runtime-cut, carried, burning, moving, or has no source mesh. Runtime cut geometry needs a separate history adapter."),
                    *GetNameSafe(Log));
                OutLogs.Reset();
                return false;
            }

            FCarvableLogSaveData Data;
            Data.Transform = Log->GetActorTransform();
            Data.SourceMesh = Log->SourceMesh;
            Data.Damage = Log->GetDamage();
            Data.bAnchored = Log->IsAnchored();
            Data.bTemporary = Log->IsTemporary();
            if (!Data.Validate(OutError))
            {
                OutLogs.Reset();
                return false;
            }
            OutLogs.Add(Data);
            if (OutLogs.Num() > 100000)
            {
                OutError = TEXT("Save blocked: carvable-log actor count exceeds the save budget.");
                OutLogs.Reset();
                return false;
            }
        }
        return true;
    }

    bool CaptureStoneActors(UWorld* World, TArray<FStoneActorSaveData>& OutStones, FString& OutError)
    {
        OutStones.Reset();
        OutError.Reset();
        for (TActorIterator<AStoneActor> It(World); It; ++It)
        {
            AStoneActor* Stone = *It;
            if (!IsValid(Stone))
            {
                continue;
            }

            const EStructuralNodeState State = Stone->GetStructuralState_Implementation();
            if (State != EStructuralNodeState::Loose && State != EStructuralNodeState::Installed)
            {
                OutError = FString::Printf(
                    TEXT("Save blocked: stone '%s' is in a transient or terminal state."),
                    *GetNameSafe(Stone));
                OutStones.Reset();
                return false;
            }

            if (State == EStructuralNodeState::Loose && Stone->MeshComponent
                && Stone->MeshComponent->IsSimulatingPhysics()
                && (Stone->MeshComponent->GetPhysicsLinearVelocity().SizeSquared() > 100.0f
                    || Stone->MeshComponent->GetPhysicsAngularVelocityInDegrees().SizeSquared() > 25.0f))
            {
                OutError = FString::Printf(TEXT("Save blocked: loose stone '%s' is still moving."), *GetNameSafe(Stone));
                OutStones.Reset();
                return false;
            }

            FStoneActorSaveData Data;
            Data.Transform = Stone->GetActorTransform();
            Data.BaseDefinition = Stone->GetBaseDefinition();
            Data.ActorClass = Stone->GetClass();
            Data.Health = Stone->GetHealth();
            if (const UStoneDefinition* Definition = Stone->GetDefinition())
            {
                Data.Shape = Definition->Shape;
                Data.Dimensions = Definition->Dimensions;
            }
            Data.State = State;
            if (!Data.Validate(OutError))
            {
                OutStones.Reset();
                return false;
            }
            OutStones.Add(Data);
            if (OutStones.Num() > 100000)
            {
                OutError = TEXT("Save blocked: stone actor count exceeds the save budget.");
                OutStones.Reset();
                return false;
            }
        }
        return true;
    }

    bool CaptureStoneMasonry(UWorld* World, TArray<FStoneMasonrySaveData>& OutMasonry, FString& OutError)
    {
        OutMasonry.Reset();
        OutError.Reset();
        for (TActorIterator<AStoneMasonryActor> It(World); It; ++It)
        {
            AStoneMasonryActor* Masonry = *It;
            if (!IsValid(Masonry))
            {
                continue;
            }
            if (Masonry->IsCollapsed())
            {
                OutError = FString::Printf(
                    TEXT("Save blocked: collapsed masonry '%s' needs a rubble/physics-state adapter."),
                    *GetNameSafe(Masonry));
                OutMasonry.Reset();
                return false;
            }

            FStoneMasonrySaveData Data;
            Data.Transform = Masonry->GetActorTransform();
            Data.Mesh = Masonry->GetStoneMeshForSave();
            Data.ActorClass = Masonry->GetClass();
            Data.MasonryType = Masonry->GetMasonryType();
            Data.bHasThroughStones = Masonry->HasThroughStones();
            Data.bHasHearting = Masonry->HasHearting();
            Data.BatterRatio = Masonry->GetBatterRatio();
            Data.bHasCrossingJoints = Masonry->HasCrossingJoints();
            Data.CompressiveStrengthMPa = Masonry->GetCompressiveStrengthMPa();
            Data.StructuralHealth = Masonry->GetStructuralHealthForSave();
            Data.SelfWeight = Masonry->GetSelfWeight();
            Data.CompressiveCapacityKN = Masonry->GetCompressiveCapacityKN();
            Data.bIsGrounded = Masonry->IsGrounded();
            if (!Data.Validate(OutError))
            {
                OutMasonry.Reset();
                return false;
            }
            OutMasonry.Add(Data);
            if (OutMasonry.Num() > 100000)
            {
                OutError = TEXT("Save blocked: stone-masonry actor count exceeds the save budget.");
                OutMasonry.Reset();
                return false;
            }
        }
        return true;
    }

    bool StageLegacyLogs(UWorld* World, TSubclassOf<ALogActor> PreferredClass,
        const TArray<FLogSaveData>& Rows, TArray<ALogActor*>& OutStaged, FString& OutError)
    {
        OutStaged.Reset();
        OutError.Reset();
        if (Rows.Num() > 100000)
        {
            OutError = TEXT("Legacy log snapshot exceeds the restore budget.");
            return false;
        }

        for (const FLogSaveData& Data : Rows)
        {
            if (!IsFiniteTransform(Data.Transform) || !FMath::IsFinite(Data.Health)
                || Data.Health < 0.0f || Data.Health > 100.0f
                || !FMath::IsFinite(Data.CurrentLength) || Data.CurrentLength <= 0.0f
                || Data.CurrentLength > 100000.0f
                || Data.State > static_cast<uint8>(ELogState::Destroyed)
                || Data.Shape > static_cast<uint8>(EWoodShape::Firewood))
            {
                OutError = TEXT("Legacy log snapshot contains invalid data.");
                return false;
            }

            UClass* SpawnClass = PreferredClass.Get();
            if (!SpawnClass)
            {
                SpawnClass = ALogActor::StaticClass();
            }
            ALogActor* Log = World->SpawnActor<ALogActor>(SpawnClass, Data.Transform);
            if (!IsValid(Log))
            {
                OutError = TEXT("Could not stage a legacy log actor.");
                return false;
            }
            if (Data.Definition)
            {
                Log->InitializeFromDefinition(Data.Definition);
            }
            Log->SetActorTransform(Data.Transform);
            Log->ApplyCut(static_cast<EWoodShape>(Data.Shape), Data.CurrentLength);
            Log->SetHealth(Data.Health);
            if (Log->GetLogState() != static_cast<ELogState>(Data.State)
                && !Log->SetLogState(static_cast<ELogState>(Data.State)))
            {
                Log->Destroy();
                OutError = TEXT("A staged legacy log rejected its saved state.");
                return false;
            }
            OutStaged.Add(Log);
        }
        return true;
    }

    bool StageCarvableLogs(UWorld* World, const TArray<FCarvableLogSaveData>& Rows,
        TArray<ACarvableLog*>& OutStaged, FString& OutError)
    {
        OutStaged.Reset();
        OutError.Reset();
        if (Rows.Num() > 100000)
        {
            OutError = TEXT("Carvable-log snapshot exceeds the restore budget.");
            return false;
        }

        for (const FCarvableLogSaveData& Data : Rows)
        {
            if (!Data.Validate(OutError))
            {
                return false;
            }
            ACarvableLog* Log = World->SpawnActorDeferred<ACarvableLog>(
                ACarvableLog::StaticClass(), Data.Transform, nullptr, nullptr,
                ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
            if (!IsValid(Log))
            {
                OutError = TEXT("Could not stage a carvable-log actor.");
                return false;
            }
            Log->SourceMesh = Data.SourceMesh;
            Log->bStartAsleep = true;
            Log->FinishSpawning(Data.Transform);
            if (!Log->RestorePersistentState(Data.Damage, Data.bAnchored, Data.bTemporary))
            {
                Log->Destroy();
                OutError = TEXT("A staged carvable log could not restore its ready-shaped state.");
                return false;
            }
            OutStaged.Add(Log);
        }
        return true;
    }

    bool StageStoneActors(UWorld* World, const TArray<FStoneActorSaveData>& Rows,
        TArray<AStoneActor*>& OutStaged, FString& OutError)
    {
        OutStaged.Reset();
        OutError.Reset();
        if (Rows.Num() > 100000)
        {
            OutError = TEXT("Stone snapshot exceeds the restore budget.");
            return false;
        }

        for (const FStoneActorSaveData& Data : Rows)
        {
            if (!Data.Validate(OutError))
            {
                return false;
            }
            TSubclassOf<AStoneActor> SpawnClass = Data.ActorClass;
            if (!SpawnClass)
            {
                SpawnClass = AStoneActor::StaticClass();
            }
            AStoneActor* Stone = World->SpawnActorDeferred<AStoneActor>(
                SpawnClass, Data.Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
            if (!IsValid(Stone))
            {
                OutError = TEXT("Could not stage a stone actor.");
                return false;
            }
            if (!Stone->RestorePersistentState(Data.BaseDefinition, Data.Shape, Data.Dimensions,
                Data.Health, Data.State, OutError))
            {
                Stone->Destroy();
                return false;
            }
            Stone->FinishSpawning(Data.Transform);
            OutStaged.Add(Stone);
        }
        return true;
    }

    bool StageStoneMasonry(UWorld* World, const TArray<FStoneMasonrySaveData>& Rows,
        TArray<AStoneMasonryActor*>& OutStaged, FString& OutError)
    {
        OutStaged.Reset();
        OutError.Reset();
        if (Rows.Num() > 100000)
        {
            OutError = TEXT("Stone-masonry snapshot exceeds the restore budget.");
            return false;
        }

        for (const FStoneMasonrySaveData& Data : Rows)
        {
            if (!Data.Validate(OutError))
            {
                return false;
            }
            TSubclassOf<AStoneMasonryActor> SpawnClass = Data.ActorClass;
            if (!SpawnClass)
            {
                SpawnClass = AStoneMasonryActor::StaticClass();
            }
            AStoneMasonryActor* Masonry = World->SpawnActorDeferred<AStoneMasonryActor>(
                SpawnClass, Data.Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
            if (!IsValid(Masonry))
            {
                OutError = TEXT("Could not stage a stone-masonry actor.");
                return false;
            }
            if (!Masonry->RestorePersistentState(Data.Mesh, Data.MasonryType, Data.bHasThroughStones,
                Data.bHasHearting, Data.BatterRatio, Data.bHasCrossingJoints, Data.CompressiveStrengthMPa,
                Data.StructuralHealth, Data.SelfWeight, Data.CompressiveCapacityKN, Data.bIsGrounded, OutError))
            {
                Masonry->Destroy();
                return false;
            }
            Masonry->FinishSpawning(Data.Transform);
            OutStaged.Add(Masonry);
        }
        return true;
    }
}

bool FCarvableLogSaveData::Validate(FString& OutError) const
{
    OutError.Reset();
    if (!IsFiniteTransform(Transform) || !IsValid(SourceMesh)
        || !FMath::IsFinite(Damage) || Damage < 0.0f || Damage > 1.0f)
    {
        OutError = TEXT("Carvable-log save record contains an invalid transform, source mesh or damage value.");
        return false;
    }
    return true;
}

bool FStoneActorSaveData::Validate(FString& OutError) const
{
    OutError.Reset();
    const bool bDimensionsValid = FMath::IsFinite(Dimensions.X) && FMath::IsFinite(Dimensions.Y)
        && FMath::IsFinite(Dimensions.Z) && Dimensions.X > 0.0f && Dimensions.Y > 0.0f && Dimensions.Z > 0.0f;
    const bool bStateValid = State == EStructuralNodeState::Loose || State == EStructuralNodeState::Installed;
    if (!IsFiniteTransform(Transform) || !IsValid(BaseDefinition) || !ActorClass
        || !FMath::IsFinite(Health) || Health <= 0.0f
        || Health > FMath::Max(1.0f, BaseDefinition ? BaseDefinition->CompressiveStrength * 0.5f : 1.0f)
        || !bDimensionsValid || static_cast<uint8>(Shape) > static_cast<uint8>(EStoneShape::Keystone)
        || !bStateValid)
    {
        OutError = TEXT("Stone save record contains an invalid transform, definition, class, health, shape, dimensions or state.");
        return false;
    }
    return true;
}

bool FStoneMasonrySaveData::Validate(FString& OutError) const
{
    OutError.Reset();
    if (!IsFiniteTransform(Transform) || !IsValid(Mesh) || !ActorClass
        || (MasonryType != EMasonryType::DryStone && MasonryType != EMasonryType::MortaredStone)
        || !FMath::IsFinite(BatterRatio) || BatterRatio < 0.0f || BatterRatio > 1.0f
        || !FMath::IsFinite(CompressiveStrengthMPa) || CompressiveStrengthMPa <= 0.0f
        || !FMath::IsFinite(StructuralHealth) || StructuralHealth <= 0.0f || StructuralHealth > 300.0f
        || !FMath::IsFinite(SelfWeight) || SelfWeight <= 0.0f
        || !FMath::IsFinite(CompressiveCapacityKN) || CompressiveCapacityKN <= 0.0f)
    {
        OutError = TEXT("Stone masonry save record contains an invalid mesh, transform, masonry type or structural value.");
        return false;
    }
    return true;
}

bool USaveGameSubsystem::SaveWorld(const FString& SlotName)
{
    LastOperationError.Reset();
    UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
    if (!World || SlotName.IsEmpty())
    {
        LastOperationError = TEXT("Saving requires a valid world and a non-empty slot name.");
        return false;
    }

    ULogSaveGame* Save = Cast<ULogSaveGame>(
        UGameplayStatics::CreateSaveGameObject(ULogSaveGame::StaticClass()));
    if (!Save)
    {
        LastOperationError = TEXT("Could not allocate the save-game object.");
        return false;
    }

    CaptureLegacyLogs(World, Save->Logs);
    FString Error;
    if (!CaptureCarvableLogs(World, Save->CarvableLogs, Error)
        || !CaptureStoneActors(World, Save->Stones, Error)
        || !CaptureStoneMasonry(World, Save->StoneMasonry, Error))
    {
        LastOperationError = Error;
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
        if (Player->GetCarriedItem() != EForestItem::None)
        {
            LastOperationError = TEXT("Save blocked: drop carried/dragged resources before saving this slice.");
            return false;
        }
        Save->PlayerTransform = Player->GetActorTransform();
        Player->ExportInventorySaveData(Save->InventoryItemIds, Save->InventoryCounts);
        if (!AForestCharacter::ValidateInventorySaveData(Save->InventoryItemIds, Save->InventoryCounts, Error))
        {
            LastOperationError = FString::Printf(TEXT("Player inventory could not be saved: %s"), *Error);
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

    ULogSaveGame* Save = Cast<ULogSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
    if (!Save)
    {
        LastOperationError = TEXT("The slot is not a recognized MyProject2 save.");
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
        FString InventoryError;
        if (!Player || !IsFiniteTransform(Save->PlayerTransform)
            || !AForestCharacter::ValidateInventorySaveData(Save->InventoryItemIds, Save->InventoryCounts, InventoryError))
        {
            LastOperationError = !Player
                ? TEXT("Save contains player state, but this world has no AForestCharacter.")
                : (InventoryError.IsEmpty() ? TEXT("Saved player transform is invalid.") : InventoryError);
            return false;
        }
    }

    FString Error;
    TArray<ALogActor*> StagedLegacyLogs;
    TArray<ACarvableLog*> StagedCarvableLogs;
    TArray<AStoneActor*> StagedStones;
    TArray<AStoneMasonryActor*> StagedMasonry;

    if (!StageLegacyLogs(World, LogClass, Save->Logs, StagedLegacyLogs, Error)
        || !StageCarvableLogs(World, Save->CarvableLogs, StagedCarvableLogs, Error)
        || !StageStoneActors(World, Save->Stones, StagedStones, Error)
        || !StageStoneMasonry(World, Save->StoneMasonry, StagedMasonry, Error))
    {
        DestroyStaged(StagedLegacyLogs);
        DestroyStaged(StagedCarvableLogs);
        DestroyStaged(StagedStones);
        DestroyStaged(StagedMasonry);
        LastOperationError = Error;
        return false;
    }

    if (Save->bHasPlayerState)
    {
        if (!Player->ImportInventorySaveData(Save->InventoryItemIds, Save->InventoryCounts, Error))
        {
            DestroyStaged(StagedLegacyLogs);
            DestroyStaged(StagedCarvableLogs);
            DestroyStaged(StagedStones);
            DestroyStaged(StagedMasonry);
            LastOperationError = FString::Printf(TEXT("Player inventory restore failed: %s"), *Error);
            return false;
        }
    }

    TArray<ALogActor*> OldLegacyLogs;
    for (TActorIterator<ALogActor> It(World); It; ++It)
    {
        if (IsValid(*It) && !StagedLegacyLogs.Contains(*It))
        {
            OldLegacyLogs.Add(*It);
        }
    }
    for (ALogActor* Actor : OldLegacyLogs) Actor->Destroy();

    TArray<ACarvableLog*> OldCarvableLogs;
    for (TActorIterator<ACarvableLog> It(World); It; ++It)
    {
        if (IsValid(*It) && !StagedCarvableLogs.Contains(*It))
        {
            OldCarvableLogs.Add(*It);
        }
    }
    for (ACarvableLog* Actor : OldCarvableLogs) Actor->Destroy();

    TArray<AStoneActor*> OldStones;
    for (TActorIterator<AStoneActor> It(World); It; ++It)
    {
        if (IsValid(*It) && !StagedStones.Contains(*It))
        {
            OldStones.Add(*It);
        }
    }
    for (AStoneActor* Actor : OldStones) Actor->Destroy();

    TArray<AStoneMasonryActor*> OldMasonry;
    for (TActorIterator<AStoneMasonryActor> It(World); It; ++It)
    {
        if (IsValid(*It) && !StagedMasonry.Contains(*It))
        {
            OldMasonry.Add(*It);
        }
    }
    for (AStoneMasonryActor* Actor : OldMasonry) Actor->Destroy();

    if (Save->bHasPlayerState && Player)
    {
        Player->SetActorTransform(Save->PlayerTransform, false, nullptr, ETeleportType::TeleportPhysics);
        Player->GetCharacterMovement()->StopMovementImmediately();
    }

    CachedLogs = Save->Logs;
    return true;
}

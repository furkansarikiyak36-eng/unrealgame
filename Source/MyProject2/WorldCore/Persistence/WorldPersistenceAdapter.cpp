#include "WorldPersistenceAdapter.h"

#include "WorldCore/Persistence/WorldRuntimeSaveGame.h"
#include "WorldCore/ExcavationVolumeSubsystem.h"
#include "Shoring/PlaceableShoringPiece.h"
#include "Shoring/ExcavationShoringSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "SurvivalComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

bool UWorldPersistenceAdapter::SaveWorldToSlot(const FString& SlotName, int32 UserIndex, FString& OutFailure)
{
    OutFailure.Reset();
    UWorld* World = GetWorld();
    if (!World || SlotName.IsEmpty())
    {
        OutFailure = TEXT("World and a non-empty save slot are required.");
        return false;
    }

    UExcavationVolumeSubsystem* Volume = World->GetSubsystem<UExcavationVolumeSubsystem>();
    if (!Volume)
    {
        OutFailure = TEXT("Excavation subsystem is unavailable.");
        return false;
    }

    UWorldRuntimeSaveGame* SaveObject = Cast<UWorldRuntimeSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UWorldRuntimeSaveGame::StaticClass()));
    if (!SaveObject || !Volume->ExportSnapshot(SaveObject->ExcavationSnapshot))
    {
        OutFailure = TEXT("Could not create a valid excavation snapshot.");
        return false;
    }

    for (TActorIterator<APlaceableShoringPiece> It(World); It; ++It)
    {
        const APlaceableShoringPiece* Piece = *It;
        if (!IsValid(Piece)) continue;

        FSavedShoringPieceRecord Record;
        Record.PieceGuid = Piece->PieceGuid;
        Record.PieceType = Piece->PieceType;
        Record.AnchorId = Piece->AnchorId;
        Record.Transform = Piece->GetActorTransform();
        Record.DimensionsCm = Piece->DimensionsCm;
        Record.StructuralHealth = Piece->StructuralHealth;
        Record.bHasVoxelAnchor = Piece->bHasVoxelAnchor;
        Record.SupportAnchorWorldCm = Piece->SupportAnchorWorldCm;
        Record.SupportNormal = Piece->SupportNormal;
        Record.RequiredAnchorIds = Piece->RequiredAnchorIds;
        SaveObject->ShoringPieces.Add(Record);
    }

    if (APawn* Player = UGameplayStatics::GetPlayerPawn(World, 0))
    {
        SaveObject->bHasPlayerTransform = true;
        SaveObject->PlayerTransform = Player->GetActorTransform();
        if (USurvivalComponent* Survival = Player->FindComponentByClass<USurvivalComponent>())
        {
            SaveObject->bHasSurvivalState = Survival->ExportState(SaveObject->SurvivalState);
        }
    }

    if (!UGameplayStatics::SaveGameToSlot(SaveObject, SlotName, UserIndex))
    {
        OutFailure = TEXT("SaveGameToSlot failed.");
        return false;
    }
    return true;
}

bool UWorldPersistenceAdapter::LoadWorldFromSlot(const FString& SlotName, int32 UserIndex, FString& OutFailure)
{
    OutFailure.Reset();
    UWorld* World = GetWorld();
    if (!World || SlotName.IsEmpty() || !UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
    {
        OutFailure = TEXT("The requested save slot does not exist.");
        return false;
    }

    UWorldRuntimeSaveGame* SaveObject = Cast<UWorldRuntimeSaveGame>(
        UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
    UExcavationVolumeSubsystem* Volume = World->GetSubsystem<UExcavationVolumeSubsystem>();
    if (!SaveObject || !Volume || SaveObject->SchemaVersion != 1)
    {
        OutFailure = TEXT("Save schema or required subsystem is invalid.");
        return false;
    }

    // Validate all actor records before mutating the live world.
    for (const FSavedShoringPieceRecord& Record : SaveObject->ShoringPieces)
    {
        if (!FMath::IsFinite(Record.Transform.GetLocation().X)
            || !FMath::IsFinite(Record.Transform.GetLocation().Y)
            || !FMath::IsFinite(Record.Transform.GetLocation().Z)
            || !FMath::IsFinite(Record.DimensionsCm.X)
            || !FMath::IsFinite(Record.DimensionsCm.Y)
            || !FMath::IsFinite(Record.DimensionsCm.Z)
            || Record.DimensionsCm.X <= 0.0f || Record.DimensionsCm.Y <= 0.0f || Record.DimensionsCm.Z <= 0.0f
            || !FMath::IsFinite(Record.StructuralHealth)
            || !FMath::IsFinite(Record.SupportAnchorWorldCm.X)
            || !FMath::IsFinite(Record.SupportAnchorWorldCm.Y)
            || !FMath::IsFinite(Record.SupportAnchorWorldCm.Z)
            || !FMath::IsFinite(Record.SupportNormal.X)
            || !FMath::IsFinite(Record.SupportNormal.Y)
            || !FMath::IsFinite(Record.SupportNormal.Z))
        {
            OutFailure = TEXT("Save contains an invalid shoring piece record.");
            return false;
        }
    }

    FExcavationVolumeSnapshot PreviousSnapshot;
    if (!Volume->ExportSnapshot(PreviousSnapshot))
    {
        OutFailure = TEXT("Could not back up the current excavation state.");
        return false;
    }

    if (!Volume->ImportSnapshot(SaveObject->ExcavationSnapshot, true))
    {
        OutFailure = TEXT("Excavation snapshot failed validation; live state was not changed.");
        return false;
    }

    TArray<APlaceableShoringPiece*> NewPieces;
    NewPieces.Reserve(SaveObject->ShoringPieces.Num());
    for (const FSavedShoringPieceRecord& Record : SaveObject->ShoringPieces)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        APlaceableShoringPiece* Piece = World->SpawnActor<APlaceableShoringPiece>(
            APlaceableShoringPiece::StaticClass(), Record.Transform, SpawnParams);
        if (!Piece)
        {
            for (APlaceableShoringPiece* NewPiece : NewPieces) if (IsValid(NewPiece)) NewPiece->Destroy();
            Volume->ImportSnapshot(PreviousSnapshot, true);
            OutFailure = TEXT("Could not restore all shoring pieces; excavation state was rolled back.");
            return false;
        }
        if (Piece->PieceMesh) Piece->PieceMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Piece->InitializeFromSavedState(Record.PieceType, Record.PieceGuid, Record.AnchorId,
            Record.Transform, Record.DimensionsCm, Record.StructuralHealth,
            Record.bHasVoxelAnchor, Record.SupportAnchorWorldCm, Record.SupportNormal, Record.RequiredAnchorIds);
        NewPieces.Add(Piece);
    }

    // Only replace the previous shoring actors after every replacement has spawned.
    TArray<APlaceableShoringPiece*> OldPieces;
    for (TActorIterator<APlaceableShoringPiece> It(World); It; ++It)
    {
        APlaceableShoringPiece* Piece = *It;
        if (IsValid(Piece) && !NewPieces.Contains(Piece)) OldPieces.Add(Piece);
    }
    for (APlaceableShoringPiece* Piece : OldPieces) if (IsValid(Piece)) Piece->Destroy();
    for (APlaceableShoringPiece* Piece : NewPieces)
    {
        if (IsValid(Piece) && Piece->PieceMesh)
        {
            Piece->PieceMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        }
    }
    if (UExcavationShoringSubsystem* Shoring = World->GetSubsystem<UExcavationShoringSubsystem>())
    {
        Shoring->RevalidateAllVoxelAnchors();
    }

    if (SaveObject->bHasPlayerTransform)
    {
        if (APawn* Player = UGameplayStatics::GetPlayerPawn(World, 0))
        {
            Player->SetActorTransform(SaveObject->PlayerTransform, false, nullptr, ETeleportType::TeleportPhysics);
            if (SaveObject->bHasSurvivalState)
            {
                if (USurvivalComponent* Survival = Player->FindComponentByClass<USurvivalComponent>())
                {
                    Survival->ImportState(SaveObject->SurvivalState);
                }
            }
        }
    }
    return true;
}

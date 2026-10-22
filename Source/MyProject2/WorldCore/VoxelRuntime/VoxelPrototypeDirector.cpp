#include "VoxelPrototypeDirector.h"

#include "Engine/World.h"
#include "WorldCore/Classification/LandscapeBuilderSubsystem.h"
#include "WorldCore/Excavation/VoxelEditService.h"
#include "WorldCore/VoxelRuntime/VoxelChunkMeshSubsystem.h"
#include "WorldCore/Persistence/WorldPersistenceAdapter.h"
#include "Shoring/ShoringLayoutPlannerSubsystem.h"
#include "Shoring/PlaceableShoringPiece.h"

AVoxelPrototypeDirector::AVoxelPrototypeDirector()
{
    PrimaryActorTick.bCanEverTick = false;
}

ULandscapeBuilderSubsystem* AVoxelPrototypeDirector::GetBuilder() const
{
    UWorld* World = GetWorld();
    return World ? World->GetSubsystem<ULandscapeBuilderSubsystem>() : nullptr;
}

UVoxelChunkMeshSubsystem* AVoxelPrototypeDirector::GetMeshSubsystem() const
{
    UWorld* World = GetWorld();
    return World ? World->GetSubsystem<UVoxelChunkMeshSubsystem>() : nullptr;
}

UVoxelEditService* AVoxelPrototypeDirector::GetEditService() const
{
    UWorld* World = GetWorld();
    return World ? World->GetSubsystem<UVoxelEditService>() : nullptr;
}

UShoringLayoutPlannerSubsystem* AVoxelPrototypeDirector::GetShoringPlanner() const
{
    UWorld* World = GetWorld();
    return World ? World->GetSubsystem<UShoringLayoutPlannerSubsystem>() : nullptr;
}

int32 AVoxelPrototypeDirector::BuildWorldClassification()
{
    ULandscapeBuilderSubsystem* Builder = GetBuilder();
    return Builder ? Builder->BuildClassificationSnapshot() : 0;
}

bool AVoxelPrototypeDirector::RebuildRegion(FVector CenterCm, FVector ExtentCm)
{
    UVoxelChunkMeshSubsystem* Mesh = GetMeshSubsystem();
    if (!Mesh) return false;
    Mesh->RequestRebuildRegion(CenterCm, ExtentCm);
    return true;
}

bool AVoxelPrototypeDirector::CarveSphere(FVector CenterCm, float RadiusCm, FString& OutFailure)
{
    UVoxelEditService* Edits = GetEditService();
    if (!Edits)
    {
        OutFailure = TEXT("Voxel edit service is unavailable.");
        return false;
    }

    FExcavationEditRecord Record;
    return Edits->CarveSphere(CenterCm, RadiusCm, Record, OutFailure);
}

bool AVoxelPrototypeDirector::CarveTunnel(FVector StartCm, FVector EndCm, float RadiusCm, FString& OutFailure)
{
    UVoxelEditService* Edits = GetEditService();
    if (!Edits)
    {
        OutFailure = TEXT("Voxel edit service is unavailable.");
        return false;
    }

    FExcavationEditRecord Record;
    return Edits->CarveCapsule(StartCm, EndCm, RadiusCm, Record, OutFailure);
}

bool AVoxelPrototypeDirector::FillSphere(FVector CenterCm, float RadiusCm, FString& OutFailure)
{
    UVoxelEditService* Edits = GetEditService();
    if (!Edits)
    {
        OutFailure = TEXT("Voxel edit service is unavailable.");
        return false;
    }

    FExcavationEditRecord Record;
    return Edits->FillSphere(CenterCm, RadiusCm, Record, OutFailure);
}

bool AVoxelPrototypeDirector::BuildTrenchPlan(FVector StartTopCm, FVector EndTopCm, float WidthCm,
    float DepthCm, float StationSpacingCm, FString& OutFailure)
{
    UShoringLayoutPlannerSubsystem* Planner = GetShoringPlanner();
    if (!Planner)
    {
        OutFailure = TEXT("Shoring planner is unavailable.");
        LastTrenchPlan.Reset();
        return false;
    }
    return Planner->BuildTrenchPlan(StartTopCm, EndTopCm, WidthCm, DepthCm,
        StationSpacingCm, LastTrenchPlan, OutFailure);
}

bool AVoxelPrototypeDirector::CommitLastTrenchPlan(AActor* ActorToIgnore, FString& OutFailure)
{
    UShoringLayoutPlannerSubsystem* Planner = GetShoringPlanner();
    if (!Planner || LastTrenchPlan.IsEmpty())
    {
        OutFailure = TEXT("No trench layout preview is available.");
        return false;
    }

    TArray<APlaceableShoringPiece*> Spawned;
    if (!Planner->CommitPlan(LastTrenchPlan, ActorToIgnore, Spawned, OutFailure))
    {
        return false;
    }

    LastSpawnedPieces.Reset();
    LastSpawnedPieces.Reserve(Spawned.Num());
    for (APlaceableShoringPiece* Piece : Spawned)
    {
        if (IsValid(Piece)) LastSpawnedPieces.Add(Piece);
    }
    return true;
}

void AVoxelPrototypeDirector::ClearLastTrenchPlan()
{
    LastTrenchPlan.Reset();
}


bool AVoxelPrototypeDirector::SavePrototypeWorld(const FString& SlotName, FString& OutFailure)
{
    UWorld* World = GetWorld();
    UWorldPersistenceAdapter* Persistence = World ? World->GetSubsystem<UWorldPersistenceAdapter>() : nullptr;
    if (!Persistence)
    {
        OutFailure = TEXT("World persistence adapter is unavailable.");
        return false;
    }
    return Persistence->SaveWorldToSlot(SlotName, 0, OutFailure);
}

bool AVoxelPrototypeDirector::LoadPrototypeWorld(const FString& SlotName, FString& OutFailure)
{
    UWorld* World = GetWorld();
    UWorldPersistenceAdapter* Persistence = World ? World->GetSubsystem<UWorldPersistenceAdapter>() : nullptr;
    if (!Persistence)
    {
        OutFailure = TEXT("World persistence adapter is unavailable.");
        return false;
    }
    return Persistence->LoadWorldFromSlot(SlotName, 0, OutFailure);
}

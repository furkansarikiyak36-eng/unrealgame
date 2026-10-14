#include "StoneWorkingComponent.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "StoneCore/StoneDefinition.h"
#include "Engine/World.h"

UStoneWorkingComponent::UStoneWorkingComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UStoneWorkingComponent::CrushBoulder(AStoneActor* Boulder, int32 Pieces)
{
    if (!Boulder || Pieces <= 0 || !StoneClass || !GetWorld())
    {
        return false;
    }

    UStoneDefinition* SourceDefinition = Boulder->GetDefinition();
    if (!SourceDefinition)
    {
        // Do not convert an undefined source into generic/default stone fragments.
        return false;
    }

    const FVector Origin = Boulder->GetActorLocation();
    TArray<AStoneActor*> SpawnedPieces;
    SpawnedPieces.Reserve(Pieces);

    for (int32 Index = 0; Index < Pieces; ++Index)
    {
        const FVector Offset(
            FMath::RandRange(-50.f, 50.f),
            FMath::RandRange(-50.f, 50.f),
            FMath::RandRange(0.f, 20.f));

        AStoneActor* Piece = GetWorld()->SpawnActor<AStoneActor>(
            StoneClass, Origin + Offset, FRotator::ZeroRotator);
        if (!Piece)
        {
            for (AStoneActor* Spawned : SpawnedPieces)
            {
                if (IsValid(Spawned))
                {
                    Spawned->Destroy();
                }
            }
            return false;
        }

        if (UStoneDefinition* SourceDefinition = Boulder->GetDefinition())
        {
            Piece->InitializeFromDefinition(SourceDefinition);
            // Give the fragment its own shape definition before the source actor is destroyed.
            Piece->ApplyShape(SourceDefinition->Shape, SourceDefinition->Dimensions);
        }
        SpawnedPieces.Add(Piece);
    }

    // Do not consume the source unless every requested piece was created.
    if (SpawnedPieces.Num() != Pieces || !Boulder->Destroy())
    {
        for (AStoneActor* Spawned : SpawnedPieces)
        {
            if (IsValid(Spawned))
            {
                Spawned->Destroy();
            }
        }
        return false;
    }

    return true;
}

bool UStoneWorkingComponent::ProduceBuildingStone(AStoneActor* Boulder)
{
    return CrushBoulder(Boulder, 4);
}

bool UStoneWorkingComponent::ProduceFlatStone(AStoneActor* Stone)
{
    if (!Stone || !Stone->GetDefinition()) return false;
    Stone->ApplyShape(EStoneShape::Flat, FVector(40, 40, 10));
    return true;
}

bool UStoneWorkingComponent::CarveSurface(AStoneActor* Stone)
{
    if (!Stone || !Stone->GetDefinition()) return false;
    Stone->ApplyShape(EStoneShape::Carved, FVector(35, 25, 20));
    return true;
}

bool UStoneWorkingComponent::CutCorner(AStoneActor* Stone)
{
    if (!Stone || !Stone->GetDefinition()) return false;
    const FVector Dimensions = Stone->GetDefinition()->Dimensions;
    Stone->ApplyShape(EStoneShape::Carved,
        FVector(Dimensions.X * 0.9f, Dimensions.Y * 0.9f, Dimensions.Z));
    return true;
}

bool UStoneWorkingComponent::HollowGroove(AStoneActor* Stone)
{
    // A success return must represent an actual shape change.
    // A proper hollow-cut geometry operation is not implemented yet.
    (void)Stone;
    return false;
}

bool UStoneWorkingComponent::EngraveJoint(AStoneActor* Stone)
{
    // Connection geometry/state must be changed before this can report success.
    (void)Stone;
    return false;
}

bool UStoneWorkingComponent::SmoothSurface(AStoneActor* Stone)
{
    // The current actor has no smoothing geometry operation.
    (void)Stone;
    return false;
}

bool UStoneWorkingComponent::ProduceGravel(AStoneActor* Stone, int32 Pieces)
{
    return CrushBoulder(Stone, Pieces);
}

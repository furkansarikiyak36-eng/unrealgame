#include "ExcavationShoringSubsystem.h"

#include "WorldCore/ExcavationVolumeSubsystem.h"
#include "Shoring/PlaceableShoringPiece.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "WorldCore/Events/WorldMechanicEventSubsystem.h"

void UExcavationShoringSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    if (UWorld* World = GetWorld())
    {
        if (UExcavationVolumeSubsystem* Volume = World->GetSubsystem<UExcavationVolumeSubsystem>())
        {
            Volume->OnVolumeChanged.AddDynamic(this, &UExcavationShoringSubsystem::HandleVolumeChanged);
            Volume->OnVolumeReset.AddDynamic(this, &UExcavationShoringSubsystem::HandleVolumeReset);
        }
    }
}

void UExcavationShoringSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        if (UExcavationVolumeSubsystem* Volume = World->GetSubsystem<UExcavationVolumeSubsystem>())
        {
            Volume->OnVolumeChanged.RemoveDynamic(this, &UExcavationShoringSubsystem::HandleVolumeChanged);
            Volume->OnVolumeReset.RemoveDynamic(this, &UExcavationShoringSubsystem::HandleVolumeReset);
        }
    }
    Super::Deinitialize();
}

void UExcavationShoringSubsystem::HandleVolumeChanged(const FExcavationEditRecord& Record, bool bIsRollback)
{
    if (Record.Changes.IsEmpty())
    {
        return;
    }

    FBox Bounds(ForceInit);
    if (UWorld* World = GetWorld())
    {
        if (UExcavationVolumeSubsystem* Volume = World->GetSubsystem<UExcavationVolumeSubsystem>())
        {
            for (const FExcavationCellChange& Change : Record.Changes)
            {
                Bounds += Volume->CellToWorldCenter(Change.Cell);
            }
        }
    }

    if (Bounds.IsValid)
    {
        Bounds = Bounds.ExpandBy(FMath::Max(0.0f, InvalidationRadiusCm));
        InvalidateNearBounds(Bounds);
    }
}

void UExcavationShoringSubsystem::HandleVolumeReset()
{
    LastInvalidatedPieceCount = 0;
    if (UWorld* World = GetWorld())
    {
        for (TActorIterator<APlaceableShoringPiece> It(World); It; ++It)
        {
            if (IsValid(*It) && ((*It)->bHasVoxelAnchor || !(*It)->RequiredAnchorIds.IsEmpty()))
            {
                (*It)->MarkSupportDirty();
                ++LastInvalidatedPieceCount;
            }
        }
    }
}

void UExcavationShoringSubsystem::InvalidateNearBounds(const FBox& Bounds)
{
    UWorld* World = GetWorld();
    if (!World || !Bounds.IsValid) return;

    LastInvalidatedPieceCount = 0;
    TSet<FName> InvalidatedAnchors;
    TArray<APlaceableShoringPiece*> Pieces;
    for (TActorIterator<APlaceableShoringPiece> It(World); It; ++It)
    {
        APlaceableShoringPiece* Piece = *It;
        if (!IsValid(Piece)) continue;
        Pieces.Add(Piece);
        if (Piece->bHasVoxelAnchor && Bounds.IsInsideOrOn(Piece->SupportAnchorWorldCm))
        {
            InvalidatedAnchors.Add(Piece->AnchorId);
            Piece->MarkSupportDirty();
            ++LastInvalidatedPieceCount;
        }
    }
    for (APlaceableShoringPiece* Piece : Pieces)
    {
        if (!IsValid(Piece) || Piece->bHasVoxelAnchor) continue;
        const bool bDependsOnInvalidatedAnchor = Piece->RequiredAnchorIds.ContainsByPredicate(
            [&InvalidatedAnchors](FName Required) { return InvalidatedAnchors.Contains(Required); });
        if (bDependsOnInvalidatedAnchor)
        {
            Piece->MarkSupportDirty();
            ++LastInvalidatedPieceCount;
        }
    }

    if (LastInvalidatedPieceCount > 0)
    {
        if (UWorldMechanicEventSubsystem* Events = World->GetSubsystem<UWorldMechanicEventSubsystem>())
        {
            FWorldMechanicEvent Event;
            Event.Type = EWorldMechanicEventType::ShoringSupportInvalidated;
            Event.PositionCm = Bounds.GetCenter();
            Event.ExtentCm = Bounds.GetExtent();
            Event.ScalarValue = static_cast<float>(LastInvalidatedPieceCount);
            Event.SourceTag = TEXT("ExcavationShoringSubsystem");
            Event.bHighPriority = true;
            Events->EnqueueEvent(MoveTemp(Event));
        }
    }
}

int32 UExcavationShoringSubsystem::RevalidateAllVoxelAnchors()
{
    UWorld* World = GetWorld();
    if (!World) return 0;

    // Anchors are validated in passes so a board/strut can find posts validated in the prior pass.
    const int32 MaxPasses = 4;
    for (int32 Pass = 0; Pass < MaxPasses; ++Pass)
    {
        for (TActorIterator<APlaceableShoringPiece> It(World); It; ++It)
        {
            if (IsValid(*It) && ((*It)->bHasVoxelAnchor || !(*It)->RequiredAnchorIds.IsEmpty()))
            {
                (*It)->RevalidateVoxelAnchor();
            }
        }
    }

    int32 ValidCount = 0;
    for (TActorIterator<APlaceableShoringPiece> It(World); It; ++It)
    {
        if (IsValid(*It) && (*It)->bValidatedPlacement) ++ValidCount;
    }
    return ValidCount;
}

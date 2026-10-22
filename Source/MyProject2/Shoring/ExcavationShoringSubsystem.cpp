#include "ExcavationShoringSubsystem.h"

#include "WorldCore/ExcavationVolumeSubsystem.h"
#include "Shoring/PlaceableShoringPiece.h"
#include "Engine/World.h"
#include "EngineUtils.h"

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
    if (UWorld* World = GetWorld())
    {
        for (TActorIterator<APlaceableShoringPiece> It(World); It; ++It)
        {
            if (IsValid(*It) && (*It)->bHasVoxelAnchor)
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
    for (TActorIterator<APlaceableShoringPiece> It(World); It; ++It)
    {
        APlaceableShoringPiece* Piece = *It;
        if (!IsValid(Piece) || !Piece->bHasVoxelAnchor) continue;
        if (Bounds.IsInsideOrOn(Piece->SupportAnchorWorldCm))
        {
            Piece->MarkSupportDirty();
            ++LastInvalidatedPieceCount;
        }
    }
}

int32 UExcavationShoringSubsystem::RevalidateAllVoxelAnchors()
{
    int32 ValidCount = 0;
    if (UWorld* World = GetWorld())
    {
        for (TActorIterator<APlaceableShoringPiece> It(World); It; ++It)
        {
            if (IsValid(*It) && (*It)->bHasVoxelAnchor && (*It)->RevalidateVoxelAnchor())
            {
                ++ValidCount;
            }
        }
    }
    return ValidCount;
}

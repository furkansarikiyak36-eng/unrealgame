#include "LandscapeBuilderSubsystem.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "LandscapeProxy.h"
#include "Components/PrimitiveComponent.h"

bool ULandscapeBuilderSubsystem::ActorHasTerrainTag(const AActor* Actor, FName Tag)
{
    if (!IsValid(Actor))
    {
        return false;
    }
    if (Actor->ActorHasTag(Tag))
    {
        return true;
    }

    TInlineComponentArray<UPrimitiveComponent*> Components;
    Actor->GetComponents(Components);
    for (const UPrimitiveComponent* Component : Components)
    {
        if (IsValid(Component) && Component->ComponentHasTag(Tag))
        {
            return true;
        }
    }
    return false;
}

void ULandscapeBuilderSubsystem::AddActorRegion(AActor* Actor, const FBox& Bounds,
    EWorldTerrainMode Mode, int32 Priority, bool bExplicit, FName SourceLabel)
{
    if (!Bounds.IsValid)
    {
        return;
    }

    FRegion Region;
    Region.Bounds = Bounds;
    Region.Mode = Mode;
    Region.Priority = Priority;
    Region.bExplicit = bExplicit;
    Region.SourceLabel = SourceLabel.IsNone() && IsValid(Actor)
        ? FName(*Actor->GetName()) : SourceLabel;
    Regions.Add(MoveTemp(Region));
}

int32 ULandscapeBuilderSubsystem::BuildClassificationSnapshot()
{
    Regions.Reset();
    LastScannedActorCount = 0;
    UWorld* World = GetWorld();
    if (!World)
    {
        return 0;
    }

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!IsValid(Actor) || Actor->IsActorBeingDestroyed())
        {
            continue;
        }

        ++LastScannedActorCount;
        FVector Origin;
        FVector Extent;
        Actor->GetActorBounds(false, Origin, Extent);
        if (Extent.IsNearlyZero())
        {
            continue;
        }

        const bool bVoxelTag = ActorHasTerrainTag(Actor, TEXT("Terrain.Mode.VoxelEditable"));
        const bool bStaticTag = ActorHasTerrainTag(Actor, TEXT("Terrain.Mode.StaticGeometry"))
            || ActorHasTerrainTag(Actor, TEXT("World.Object.Rock"));
        const bool bTransitionTag = ActorHasTerrainTag(Actor, TEXT("Terrain.Mode.Transition"));
        const bool bUnknownTag = ActorHasTerrainTag(Actor, TEXT("Terrain.Mode.Unknown"));

        if (bStaticTag)
        {
            AddActorRegion(Actor, FBox::BuildAABB(Origin, Extent), EWorldTerrainMode::StaticGeometry,
                1000, true, FName(*Actor->GetName()));
            continue;
        }
        if (bTransitionTag)
        {
            AddActorRegion(Actor, FBox::BuildAABB(Origin, Extent), EWorldTerrainMode::Transition,
                900, true, FName(*Actor->GetName()));
            continue;
        }
        if (bUnknownTag)
        {
            AddActorRegion(Actor, FBox::BuildAABB(Origin, Extent), EWorldTerrainMode::Unknown,
                800, true, FName(*Actor->GetName()));
            continue;
        }
        if (bVoxelTag)
        {
            AddActorRegion(Actor, FBox::BuildAABB(Origin, Extent), EWorldTerrainMode::EditableVoxel,
                700, true, FName(*Actor->GetName()));
            continue;
        }

        if (bTreatLandscapeAsEditableByDefault && Cast<ALandscapeProxy>(Actor))
        {
            FVector ExpandedExtent = Extent;
            ExpandedExtent.Z += LandscapeMarginBelowCm + LandscapeMarginAboveCm;
            Origin.Z += (LandscapeMarginAboveCm - LandscapeMarginBelowCm) * 0.5f;
            AddActorRegion(Actor, FBox::BuildAABB(Origin, ExpandedExtent),
                EWorldTerrainMode::EditableVoxel, 100, false, FName(*Actor->GetName()));
        }
    }

    return Regions.Num();
}

bool ULandscapeBuilderSubsystem::RegisterRegionOverride(FVector CenterCm, FVector ExtentCm,
    EWorldTerrainMode Mode, int32 Priority, FName SourceLabel)
{
    if (!FMath::IsFinite(CenterCm.X) || !FMath::IsFinite(CenterCm.Y) || !FMath::IsFinite(CenterCm.Z)
        || !FMath::IsFinite(ExtentCm.X) || !FMath::IsFinite(ExtentCm.Y) || !FMath::IsFinite(ExtentCm.Z)
        || ExtentCm.X <= 0.0f || ExtentCm.Y <= 0.0f || ExtentCm.Z <= 0.0f
        || Mode == EWorldTerrainMode::Unknown && Priority < 0)
    {
        return false;
    }

    AddActorRegion(nullptr, FBox::BuildAABB(CenterCm, ExtentCm), Mode, Priority, true, SourceLabel);
    return true;
}

void ULandscapeBuilderSubsystem::ClearRegionOverrides()
{
    // Rebuild from world actors to avoid accidentally losing the baseline region map.
    BuildClassificationSnapshot();
}

FWorldClassificationResult ULandscapeBuilderSubsystem::QueryModeAtWorld(FVector WorldPositionCm) const
{
    FWorldClassificationResult Result;
    for (const FRegion& Region : Regions)
    {
        if (Region.Bounds.IsInsideOrOn(WorldPositionCm) && Region.Priority > Result.Priority)
        {
            Result.Mode = Region.Mode;
            Result.Priority = Region.Priority;
            Result.bExplicitRule = Region.bExplicit;
            Result.SourceLabel = Region.SourceLabel;
        }
    }
    return Result;
}

bool ULandscapeBuilderSubsystem::IsVoxelEditableAtWorld(FVector WorldPositionCm) const
{
    return QueryModeAtWorld(WorldPositionCm).Mode == EWorldTerrainMode::EditableVoxel;
}

// Copyright Epic Games, Inc. All Rights Reserved.

#include "WorldClassificationSubsystem.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "LandscapeProxy.h"

namespace
{
    constexpr int32 ManualOverridePriority = 100;
    constexpr int32 LandscapeLayerPriority = 80;
    constexpr int32 ExplicitRegionTagPriority = 60;
    constexpr int32 GeometrySamplePriority = 20;
    constexpr float SurfaceProbeMarginCm = 100.0f;

    bool IsFiniteBox(const FBox& Box)
    {
        return Box.IsValid
            && WorldSystemContractValidation::IsFiniteVector(Box.Min)
            && WorldSystemContractValidation::IsFiniteVector(Box.Max);
    }

    bool BoxesIntersectInclusive(const FBox& A, const FBox& B)
    {
        return A.IsValid && B.IsValid
            && A.Min.X <= B.Max.X && A.Max.X >= B.Min.X
            && A.Min.Y <= B.Max.Y && A.Max.Y >= B.Min.Y
            && A.Min.Z <= B.Max.Z && A.Max.Z >= B.Min.Z;
    }

    bool BoxContainsBox(const FBox& Outer, const FBox& Inner)
    {
        constexpr float Epsilon = 0.01f;
        return Outer.IsValid && Inner.IsValid
            && Outer.Min.X <= Inner.Min.X + Epsilon
            && Outer.Min.Y <= Inner.Min.Y + Epsilon
            && Outer.Min.Z <= Inner.Min.Z + Epsilon
            && Outer.Max.X >= Inner.Max.X - Epsilon
            && Outer.Max.Y >= Inner.Max.Y - Epsilon
            && Outer.Max.Z >= Inner.Max.Z - Epsilon;
    }

    void SortTags(TArray<FName>& Tags)
    {
        Tags.Sort([](const FName& A, const FName& B)
        {
            return A.ToString() < B.ToString();
        });
    }

    struct FClassificationCandidate
    {
        FBox Bounds = FBox(ForceInit);
        FWorldRegionClassification Classification;
        int32 Priority = INDEX_NONE;
        FString StableKey;
        bool bManualOverride = false;
    };
}

void UWorldClassificationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CachedRegions.Reset();
    CachedBuildBounds = FBox(ForceInit);
    CachedRegionSizeCm = FVector::ZeroVector;
    CachedGridDimensions = FIntVector::ZeroValue;
    bHasBuiltClassification = false;
    ClassificationRevision = 0;
}

void UWorldClassificationSubsystem::Deinitialize()
{
    CachedRegions.Reset();
    ManualOverrides.Reset();
    CachedBuildBounds = FBox(ForceInit);
    CachedRegionSizeCm = FVector::ZeroVector;
    CachedGridDimensions = FIntVector::ZeroValue;
    bHasBuiltClassification = false;
    Super::Deinitialize();
}

bool UWorldClassificationSubsystem::IsValidRegionKind(EWorldRegionKind Kind) const
{
    switch (Kind)
    {
    case EWorldRegionKind::EditableVoxel:
    case EWorldRegionKind::StaticGeometry:
    case EWorldRegionKind::Transition:
    case EWorldRegionKind::Water:
        return true;
    case EWorldRegionKind::Unknown:
    default:
        return false;
    }
}

bool UWorldClassificationSubsystem::TryClassifyRegionTag(
    FName Tag,
    EWorldRegionKind& OutKind,
    EWorldClassificationSource& OutSource)
{
    OutKind = EWorldRegionKind::Unknown;
    OutSource = EWorldClassificationSource::DefaultUnknown;

    struct FTagRule
    {
        FName Name;
        EWorldRegionKind Kind;
        EWorldClassificationSource Source;
    };

    static const FTagRule Rules[] =
    {
        { FName(TEXT("World.Region.EditableVoxel")), EWorldRegionKind::EditableVoxel, EWorldClassificationSource::ActorTag },
        { FName(TEXT("World.Region.StaticGeometry")), EWorldRegionKind::StaticGeometry, EWorldClassificationSource::ActorTag },
        { FName(TEXT("World.Region.Transition")), EWorldRegionKind::Transition, EWorldClassificationSource::ActorTag },
        { FName(TEXT("World.Region.Water")), EWorldRegionKind::Water, EWorldClassificationSource::ActorTag },
        { FName(TEXT("World.LandscapeLayer.EditableVoxel")), EWorldRegionKind::EditableVoxel, EWorldClassificationSource::LandscapeLayer },
        { FName(TEXT("World.LandscapeLayer.StaticGeometry")), EWorldRegionKind::StaticGeometry, EWorldClassificationSource::LandscapeLayer },
        { FName(TEXT("World.LandscapeLayer.Transition")), EWorldRegionKind::Transition, EWorldClassificationSource::LandscapeLayer },
        { FName(TEXT("World.LandscapeLayer.Water")), EWorldRegionKind::Water, EWorldClassificationSource::LandscapeLayer }
    };

    for (const FTagRule& Rule : Rules)
    {
        if (Tag == Rule.Name)
        {
            OutKind = Rule.Kind;
            OutSource = Rule.Source;
            return true;
        }
    }
    return false;
}

bool UWorldClassificationSubsystem::SetManualOverride(
    FName OverrideId,
    const FBox& Bounds,
    EWorldRegionKind Kind,
    const FString& Reason)
{
    if (OverrideId.IsNone() || !IsFiniteBox(Bounds) || !IsValidRegionKind(Kind))
    {
        return false;
    }

    FWorldManualRegionOverride NewOverride;
    NewOverride.Id = OverrideId;
    NewOverride.Bounds = Bounds;
    NewOverride.Kind = Kind;
    NewOverride.Reason = Reason.IsEmpty()
        ? FString::Printf(TEXT("Manual override '%s'"), *OverrideId.ToString())
        : Reason;

    const int32 ExistingIndex = ManualOverrides.IndexOfByPredicate(
        [OverrideId](const FWorldManualRegionOverride& Existing)
        {
            return Existing.Id == OverrideId;
        });

    if (ExistingIndex != INDEX_NONE)
    {
        ManualOverrides[ExistingIndex] = MoveTemp(NewOverride);
    }
    else
    {
        ManualOverrides.Add(MoveTemp(NewOverride));
    }

    ManualOverrides.Sort([](const FWorldManualRegionOverride& A, const FWorldManualRegionOverride& B)
    {
        return A.Id.ToString() < B.Id.ToString();
    });

    InvalidateClassification();
    return true;
}

bool UWorldClassificationSubsystem::RemoveManualOverride(FName OverrideId)
{
    const int32 Removed = ManualOverrides.RemoveAll(
        [OverrideId](const FWorldManualRegionOverride& Existing)
        {
            return Existing.Id == OverrideId;
        });
    if (Removed == 0)
    {
        return false;
    }

    InvalidateClassification();
    return true;
}

void UWorldClassificationSubsystem::ClearManualOverrides()
{
    if (!ManualOverrides.IsEmpty())
    {
        ManualOverrides.Reset();
        InvalidateClassification();
    }
}

void UWorldClassificationSubsystem::InvalidateClassification()
{
    CachedRegions.Reset();
    CachedBuildBounds = FBox(ForceInit);
    CachedRegionSizeCm = FVector::ZeroVector;
    CachedGridDimensions = FIntVector::ZeroValue;
    bHasBuiltClassification = false;
    ++ClassificationRevision;
    if (ClassificationRevision == 0)
    {
        ++ClassificationRevision;
    }
}

FBox UWorldClassificationSubsystem::MakeCellBounds(const FIntVector& Coordinate) const
{
    if (!CachedBuildBounds.IsValid || CachedGridDimensions.X <= 0
        || CachedGridDimensions.Y <= 0 || CachedGridDimensions.Z <= 0)
    {
        return FBox(ForceInit);
    }

    const FVector CellMin = CachedBuildBounds.Min + FVector(
        Coordinate.X * CachedRegionSizeCm.X,
        Coordinate.Y * CachedRegionSizeCm.Y,
        Coordinate.Z * CachedRegionSizeCm.Z);
    const FVector CellMax = FVector(
        FMath::Min(CellMin.X + CachedRegionSizeCm.X, CachedBuildBounds.Max.X),
        FMath::Min(CellMin.Y + CachedRegionSizeCm.Y, CachedBuildBounds.Max.Y),
        FMath::Min(CellMin.Z + CachedRegionSizeCm.Z, CachedBuildBounds.Max.Z));
    return FBox(CellMin, CellMax);
}

bool UWorldClassificationSubsystem::GetCellCoordinate(
    const FVector& WorldPositionCm,
    FIntVector& OutCoordinate) const
{
    if (!bHasBuiltClassification
        || !WorldSystemContractValidation::IsFiniteVector(WorldPositionCm)
        || !CachedBuildBounds.IsValid
        || CachedGridDimensions.X <= 0
        || CachedGridDimensions.Y <= 0
        || CachedGridDimensions.Z <= 0)
    {
        return false;
    }

    if (WorldPositionCm.X < CachedBuildBounds.Min.X || WorldPositionCm.X > CachedBuildBounds.Max.X
        || WorldPositionCm.Y < CachedBuildBounds.Min.Y || WorldPositionCm.Y > CachedBuildBounds.Max.Y
        || WorldPositionCm.Z < CachedBuildBounds.Min.Z || WorldPositionCm.Z > CachedBuildBounds.Max.Z)
    {
        return false;
    }

    OutCoordinate = FIntVector(
        FMath::Clamp(FMath::FloorToInt((WorldPositionCm.X - CachedBuildBounds.Min.X) / CachedRegionSizeCm.X), 0, CachedGridDimensions.X - 1),
        FMath::Clamp(FMath::FloorToInt((WorldPositionCm.Y - CachedBuildBounds.Min.Y) / CachedRegionSizeCm.Y), 0, CachedGridDimensions.Y - 1),
        FMath::Clamp(FMath::FloorToInt((WorldPositionCm.Z - CachedBuildBounds.Min.Z) / CachedRegionSizeCm.Z), 0, CachedGridDimensions.Z - 1));
    return true;
}

bool UWorldClassificationSubsystem::BuildClassification(
    const FWorldClassificationBuildRequest& Request,
    FWorldClassificationBuildResult& OutResult)
{
    OutResult = FWorldClassificationBuildResult{};

    FString ValidationError;
    if (!Request.Validate(ValidationError))
    {
        OutResult.Error = ValidationError;
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        OutResult.Error = TEXT("WorldClassificationSubsystem has no world.");
        return false;
    }

    const FVector RequestedSize = Request.Bounds.GetSize();
    const FIntVector Dimensions(
        FMath::Max(1, FMath::CeilToInt(RequestedSize.X / Request.RegionSizeCm.X)),
        FMath::Max(1, FMath::CeilToInt(RequestedSize.Y / Request.RegionSizeCm.Y)),
        FMath::Max(1, FMath::CeilToInt(RequestedSize.Z / Request.RegionSizeCm.Z)));

    const int64 RegionCount = static_cast<int64>(Dimensions.X)
        * static_cast<int64>(Dimensions.Y)
        * static_cast<int64>(Dimensions.Z);
    if (RegionCount <= 0 || RegionCount > Request.MaxRegions || RegionCount > MAX_int32)
    {
        OutResult.Error = FString::Printf(
            TEXT("Classification grid has %lld regions; MaxRegions is %d."),
            static_cast<long long>(RegionCount),
            Request.MaxRegions);
        return false;
    }

    // Build to local storage first. A failed request must not corrupt the previous cache.
    const FBox BuildBounds = Request.Bounds;
    const FVector RegionSize = Request.RegionSizeCm;
    TMap<FIntVector, FCachedClassificationCell> NewRegions;
    NewRegions.Reserve(static_cast<int32>(RegionCount));

    const auto CellBoundsFor = [&BuildBounds, &RegionSize](const FIntVector& Coordinate)
    {
        const FVector CellMin = BuildBounds.Min + FVector(
            Coordinate.X * RegionSize.X,
            Coordinate.Y * RegionSize.Y,
            Coordinate.Z * RegionSize.Z);
        const FVector CellMax = FVector(
            FMath::Min(CellMin.X + RegionSize.X, BuildBounds.Max.X),
            FMath::Min(CellMin.Y + RegionSize.Y, BuildBounds.Max.Y),
            FMath::Min(CellMin.Z + RegionSize.Z, BuildBounds.Max.Z));
        return FBox(CellMin, CellMax);
    };

    for (int32 X = 0; X < Dimensions.X; ++X)
    {
        for (int32 Y = 0; Y < Dimensions.Y; ++Y)
        {
            for (int32 Z = 0; Z < Dimensions.Z; ++Z)
            {
                const FIntVector Coordinate(X, Y, Z);
                FCachedClassificationCell Cell;
                Cell.Classification.Bounds = CellBoundsFor(Coordinate);
                Cell.Classification.Kind = EWorldRegionKind::Unknown;
                Cell.Classification.Source = EWorldClassificationSource::DefaultUnknown;
                Cell.Classification.Confidence = 0.0f;
                Cell.Classification.Reason = TEXT("No explicit classification marker or surface hit was found.");
                NewRegions.Add(Coordinate, MoveTemp(Cell));
            }
        }
    }

    const auto ApplyCandidate = [&NewRegions, &CellBoundsFor, &BuildBounds, &RegionSize, &Dimensions](
        const FClassificationCandidate& Candidate)
    {
        if (!Candidate.Bounds.IsValid || !BoxesIntersectInclusive(Candidate.Bounds, BuildBounds))
        {
            return;
        }

        const FBox ClippedBounds(
            FVector(
                FMath::Max(Candidate.Bounds.Min.X, BuildBounds.Min.X),
                FMath::Max(Candidate.Bounds.Min.Y, BuildBounds.Min.Y),
                FMath::Max(Candidate.Bounds.Min.Z, BuildBounds.Min.Z)),
            FVector(
                FMath::Min(Candidate.Bounds.Max.X, BuildBounds.Max.X),
                FMath::Min(Candidate.Bounds.Max.Y, BuildBounds.Max.Y),
                FMath::Min(Candidate.Bounds.Max.Z, BuildBounds.Max.Z)));

        const auto CoordFor = [&BuildBounds, &RegionSize, &Dimensions](const FVector& P)
        {
            return FIntVector(
                FMath::Clamp(FMath::FloorToInt((P.X - BuildBounds.Min.X) / RegionSize.X), 0, Dimensions.X - 1),
                FMath::Clamp(FMath::FloorToInt((P.Y - BuildBounds.Min.Y) / RegionSize.Y), 0, Dimensions.Y - 1),
                FMath::Clamp(FMath::FloorToInt((P.Z - BuildBounds.Min.Z) / RegionSize.Z), 0, Dimensions.Z - 1));
        };

        const FIntVector MinCell = CoordFor(ClippedBounds.Min);
        // The upper face is exclusive for grid traversal. Without ceil-minus-one,
        // an override ending exactly at a cell boundary leaks into the next cell.
        const FIntVector MaxCell(
            FMath::Clamp(FMath::CeilToInt((ClippedBounds.Max.X - BuildBounds.Min.X) / RegionSize.X) - 1, 0, Dimensions.X - 1),
            FMath::Clamp(FMath::CeilToInt((ClippedBounds.Max.Y - BuildBounds.Min.Y) / RegionSize.Y) - 1, 0, Dimensions.Y - 1),
            FMath::Clamp(FMath::CeilToInt((ClippedBounds.Max.Z - BuildBounds.Min.Z) / RegionSize.Z) - 1, 0, Dimensions.Z - 1));
        for (int32 X = MinCell.X; X <= MaxCell.X; ++X)
        {
            for (int32 Y = MinCell.Y; Y <= MaxCell.Y; ++Y)
            {
                for (int32 Z = MinCell.Z; Z <= MaxCell.Z; ++Z)
                {
                    const FIntVector Coordinate(X, Y, Z);
                    FCachedClassificationCell* Existing = NewRegions.Find(Coordinate);
                    if (!Existing)
                    {
                        continue;
                    }

                    if (Candidate.Priority < Existing->Priority)
                    {
                        continue;
                    }

                    if (Candidate.Priority == Existing->Priority && !Existing->StableKey.IsEmpty())
                    {
                        if (Existing->bPriorityConflict)
                        {
                            continue;
                        }

                        if (Candidate.Classification.Kind != Existing->Classification.Kind)
                        {
                            const FString FirstKey = Existing->StableKey;
                            Existing->Classification.Kind = EWorldRegionKind::Transition;
                            Existing->Classification.Confidence = 0.0f;
                            Existing->Classification.bExplicitDecision = true;
                            Existing->Classification.Reason = TEXT("Conflicting equal-priority region markers; conservative Transition result.");
                            Existing->Priority = Candidate.Priority;
                            Existing->StableKey = FString(TEXT("conflict|")) + FirstKey;
                            Existing->bPriorityConflict = true;
                            continue;
                        }

                        if (Candidate.StableKey >= Existing->StableKey)
                        {
                            continue;
                        }
                    }

                    const FBox CellBounds = CellBoundsFor(Coordinate);
                    FWorldRegionClassification Applied = Candidate.Classification;
                    Applied.Bounds = CellBounds;

                    // Never let a partial marker turn a coarser region into editable terrain.
                    if (!BoxContainsBox(Candidate.Bounds, CellBounds)
                        && Applied.Kind != EWorldRegionKind::Transition)
                    {
                        Applied.Kind = EWorldRegionKind::Transition;
                        Applied.Confidence = FMath::Min(Applied.Confidence, 0.5f);
                        Applied.Reason = FString::Printf(
                            TEXT("Partial region coverage; conservative Transition result. Source: %s"),
                            *Candidate.Classification.Reason);
                    }

                    Existing->Classification = MoveTemp(Applied);
                    Existing->Priority = Candidate.Priority;
                    Existing->StableKey = Candidate.StableKey;
                }
            }
        }
    };

    TArray<FClassificationCandidate> Candidates;
    Candidates.Reserve(ManualOverrides.Num() + 256);

    if (Request.bRespectManualOverrides)
    {
        for (const FWorldManualRegionOverride& Manual : ManualOverrides)
        {
            FClassificationCandidate Candidate;
            Candidate.Bounds = Manual.Bounds;
            Candidate.Priority = ManualOverridePriority;
            Candidate.StableKey = FString(TEXT("0|")) + Manual.Id.ToString();
            Candidate.bManualOverride = true;
            Candidate.Classification.Kind = Manual.Kind;
            Candidate.Classification.Source = EWorldClassificationSource::ManualOverride;
            Candidate.Classification.Confidence = 1.0f;
            Candidate.Classification.bExplicitDecision = true;
            Candidate.Classification.SourceTag = Manual.Id;
            Candidate.Classification.Reason = Manual.Reason;
            Candidates.Add(MoveTemp(Candidate));
        }
    }

    // Sort actors by stable object path so equal-priority tags never depend on iterator order.
    TArray<AActor*> Actors;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (IsValid(*It))
        {
            Actors.Add(*It);
        }
    }
    Actors.Sort([](const AActor& A, const AActor& B)
    {
        return A.GetPathName() < B.GetPathName();
    });

    for (AActor* Actor : Actors)
    {
        FVector ActorOrigin = FVector::ZeroVector;
        FVector ActorExtent = FVector::ZeroVector;
        Actor->GetActorBounds(false, ActorOrigin, ActorExtent, true);
        const FBox ActorBounds(ActorOrigin - ActorExtent, ActorOrigin + ActorExtent);

        TArray<FName> ActorTags = Actor->Tags;
        SortTags(ActorTags);
        for (const FName Tag : ActorTags)
        {
            EWorldRegionKind Kind;
            EWorldClassificationSource Source;
            if (!TryClassifyRegionTag(Tag, Kind, Source))
            {
                continue;
            }

            FClassificationCandidate Candidate;
            Candidate.Bounds = ActorBounds;
            Candidate.Priority = Source == EWorldClassificationSource::LandscapeLayer
                ? LandscapeLayerPriority : ExplicitRegionTagPriority;
            Candidate.StableKey = FString(TEXT("1|")) + Actor->GetPathName() + TEXT("|actor|") + Tag.ToString();
            Candidate.Classification.Kind = Kind;
            Candidate.Classification.Source = Source;
            Candidate.Classification.Confidence = Source == EWorldClassificationSource::LandscapeLayer ? 0.95f : 0.9f;
            Candidate.Classification.bExplicitDecision = true;
            Candidate.Classification.SourceTag = Tag;
            Candidate.Classification.Reason = FString::Printf(TEXT("Actor tag on %s"), *Actor->GetPathName());
            Candidates.Add(MoveTemp(Candidate));
        }

        TArray<UPrimitiveComponent*> Components;
        Actor->GetComponents<UPrimitiveComponent>(Components);
        Components.Sort([](const UPrimitiveComponent& A, const UPrimitiveComponent& B)
        {
            return A.GetPathName() < B.GetPathName();
        });

        for (UPrimitiveComponent* Component : Components)
        {
            if (!IsValid(Component))
            {
                continue;
            }

            const FBox ComponentBounds = Component->Bounds.GetBox();
            TArray<FName> ComponentTags = Component->ComponentTags;
            SortTags(ComponentTags);
            for (const FName Tag : ComponentTags)
            {
                EWorldRegionKind Kind;
                EWorldClassificationSource Source;
                if (!TryClassifyRegionTag(Tag, Kind, Source))
                {
                    continue;
                }

                FClassificationCandidate Candidate;
                Candidate.Bounds = ComponentBounds;
                Candidate.Priority = Source == EWorldClassificationSource::LandscapeLayer
                    ? LandscapeLayerPriority : ExplicitRegionTagPriority;
                Candidate.StableKey = FString(TEXT("1|")) + Actor->GetPathName()
                    + TEXT("|component|") + Component->GetPathName() + TEXT("|") + Tag.ToString();
                Candidate.Classification.Kind = Kind;
                Candidate.Classification.Source = Source;
                Candidate.Classification.Confidence = Source == EWorldClassificationSource::LandscapeLayer ? 0.95f : 0.9f;
                Candidate.Classification.bExplicitDecision = true;
                Candidate.Classification.SourceTag = Tag;
                Candidate.Classification.Reason = FString::Printf(
                    TEXT("Component tag on %s"), *Component->GetPathName());
                Candidates.Add(MoveTemp(Candidate));
            }
        }
    }

    Candidates.Sort([](const FClassificationCandidate& A, const FClassificationCandidate& B)
    {
        if (A.Priority != B.Priority)
        {
            return A.Priority > B.Priority;
        }
        return A.StableKey < B.StableKey;
    });
    for (const FClassificationCandidate& Candidate : Candidates)
    {
        ApplyCandidate(Candidate);
    }

    int32 GeometrySamples = 0;
    if (Request.bIncludeGeometrySampling)
    {
        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WorldClassificationSurfaceProbe), false);
        QueryParams.bFindInitialOverlaps = false;
        for (int32 X = 0; X < Dimensions.X && GeometrySamples < Request.MaxGeometrySamples; ++X)
        {
            for (int32 Y = 0; Y < Dimensions.Y && GeometrySamples < Request.MaxGeometrySamples; ++Y)
            {
                for (int32 Z = 0; Z < Dimensions.Z && GeometrySamples < Request.MaxGeometrySamples; ++Z)
                {
                    const FIntVector Coordinate(X, Y, Z);
                    FCachedClassificationCell* Cell = NewRegions.Find(Coordinate);
                    if (!Cell || Cell->Priority != INDEX_NONE)
                    {
                        continue;
                    }

                    const FBox CellBounds = CellBoundsFor(Coordinate);
                    const FVector Center = CellBounds.GetCenter();
                    const FVector TraceStart(Center.X, Center.Y, CellBounds.Max.Z + SurfaceProbeMarginCm);
                    const FVector TraceEnd(Center.X, Center.Y, CellBounds.Min.Z - SurfaceProbeMarginCm);
                    FHitResult Hit;
                    ++GeometrySamples;
                    if (!World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams)
                        || !Hit.bBlockingHit || !Hit.GetActor())
                    {
                        continue;
                    }

                    EWorldRegionKind HitKind = EWorldRegionKind::Unknown;
                    EWorldClassificationSource HitSource = EWorldClassificationSource::DefaultUnknown;
                    FName HitTag = NAME_None;
                    TArray<FName> HitTags = Hit.GetActor()->Tags;
                    if (Hit.GetComponent())
                    {
                        HitTags.Append(Hit.GetComponent()->ComponentTags);
                    }
                    SortTags(HitTags);
                    for (const FName Tag : HitTags)
                    {
                        if (TryClassifyRegionTag(Tag, HitKind, HitSource))
                        {
                            HitTag = Tag;
                            break;
                        }
                    }

                    if (HitKind == EWorldRegionKind::Unknown)
                    {
                        const UPrimitiveComponent* HitComponent = Hit.GetComponent();
                        if (Cast<ALandscapeProxy>(Hit.GetActor())
                            || (HitComponent && HitComponent->Mobility == EComponentMobility::Static))
                        {
                            HitKind = EWorldRegionKind::StaticGeometry;
                            HitSource = EWorldClassificationSource::GeometrySampling;
                        }
                    }

                    if (HitKind == EWorldRegionKind::Unknown)
                    {
                        continue;
                    }

                    Cell->Classification.Kind = HitKind;
                    Cell->Classification.Source = HitSource;
                    Cell->Classification.Confidence = HitSource == EWorldClassificationSource::LandscapeLayer ? 0.9f : 0.65f;
                    Cell->Classification.bExplicitDecision = HitSource == EWorldClassificationSource::LandscapeLayer
                        || HitSource == EWorldClassificationSource::ActorTag;
                    Cell->Classification.SourceTag = HitTag;
                    Cell->Classification.Reason = HitTag.IsNone()
                        ? FString(TEXT("Bounded center ray hit untagged static geometry; classified conservatively as non-editable."))
                        : FString::Printf(TEXT("Bounded center ray matched marker %s."), *HitTag.ToString());
                    Cell->Priority = GeometrySamplePriority;
                    Cell->StableKey = FString(TEXT("2|")) + Hit.GetActor()->GetPathName();
                }
            }
        }
    }

    CachedBuildBounds = BuildBounds;
    CachedRegionSizeCm = RegionSize;
    CachedGridDimensions = Dimensions;
    CachedRegions = MoveTemp(NewRegions);
    bHasBuiltClassification = true;
    ++ClassificationRevision;
    if (ClassificationRevision == 0)
    {
        ++ClassificationRevision;
    }

    OutResult.bSucceeded = true;
    OutResult.Regions.Reserve(static_cast<int32>(RegionCount));
    for (int32 X = 0; X < Dimensions.X; ++X)
    {
        for (int32 Y = 0; Y < Dimensions.Y; ++Y)
        {
            for (int32 Z = 0; Z < Dimensions.Z; ++Z)
            {
                const FCachedClassificationCell* Cell = CachedRegions.Find(FIntVector(X, Y, Z));
                if (!Cell)
                {
                    continue;
                }
                OutResult.Regions.Add(Cell->Classification);
                if (Cell->Classification.Kind == EWorldRegionKind::Unknown)
                {
                    ++OutResult.UnknownRegionCount;
                }
            }
        }
    }
    return true;
}

bool UWorldClassificationSubsystem::QueryClassification(
    const FVector& WorldPositionCm,
    FWorldRegionClassification& OutResult) const
{
    OutResult = FWorldRegionClassification{};
    FIntVector Coordinate;
    if (!GetCellCoordinate(WorldPositionCm, Coordinate))
    {
        return false;
    }

    const FCachedClassificationCell* Cell = CachedRegions.Find(Coordinate);
    if (!Cell)
    {
        return false;
    }

    OutResult = Cell->Classification;
    // An Unknown result inside a built map is still a successful lookup; callers
    // must inspect OutResult.Kind rather than interpreting Unknown as a lookup error.
    return true;
}

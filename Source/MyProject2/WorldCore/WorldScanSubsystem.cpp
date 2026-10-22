// Copyright Epic Games, Inc. All Rights Reserved.

#include "WorldScanSubsystem.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "LandscapeProxy.h"
#include "WorldCore/WorldClassificationSubsystem.h"
#include "WorldCore/WorldRuntimeProfileSubsystem.h"
#include "WorldScanSystem.h"

namespace
{
    bool HasActorOrComponentTag(
        const AActor* Actor,
        const UPrimitiveComponent* Component,
        const TCHAR* TagName)
    {
        const FName Tag(TagName);
        return (Actor && Actor->ActorHasTag(Tag))
            || (Component && Component->ComponentHasTag(Tag));
    }

    bool IsActorOrComponentUnstable(
        const AActor* Actor,
        const UPrimitiveComponent* Component)
    {
        if (HasActorOrComponentTag(Actor, Component, TEXT("World.State.Falling"))
            || HasActorOrComponentTag(Actor, Component, TEXT("World.State.Moving")))
        {
            return true;
        }

        if (Component && Component->IsSimulatingPhysics())
        {
            if (!Component->IsAnyRigidBodyAwake())
            {
                return false;
            }
            return Component->GetPhysicsLinearVelocity().SizeSquared() > FMath::Square(5.0f)
                || Component->GetPhysicsAngularVelocityInRadians().SizeSquared() > FMath::Square(0.05f);
        }

        return Actor && Actor->GetVelocity().SizeSquared() > FMath::Square(5.0f);
    }
}

bool UWorldScanSubsystem::IsWorldCompatible(UWorld* World) const
{
    return World != nullptr && World == GetWorld();
}

bool UWorldScanSubsystem::ResolveRegionKind(
    const AActor* Actor,
    const UPrimitiveComponent* Component,
    const FVector& WorldPositionCm,
    EWorldRegionKind& OutKind) const
{
    OutKind = EWorldRegionKind::Unknown;

    if (UWorld* World = GetWorld())
    {
        if (const UWorldClassificationSubsystem* Classification = World->GetSubsystem<UWorldClassificationSubsystem>())
        {
            FWorldRegionClassification Region;
            if (Classification->QueryClassification(WorldPositionCm, Region)
                && Region.Kind != EWorldRegionKind::Unknown)
            {
                OutKind = Region.Kind;
                return true;
            }
        }
    }

    // If the coarse map is unavailable/not built, use a conservative deterministic
    // marker policy: landscape-layer tags outrank general region tags, but conflicting
    // tags at the same priority resolve to Transition instead of accidentally granting digging.
    TArray<FName> Tags;
    if (Actor)
    {
        Tags.Append(Actor->Tags);
    }
    if (Component)
    {
        Tags.Append(Component->ComponentTags);
    }
    Tags.Sort([](const FName& A, const FName& B)
    {
        return A.ToString() < B.ToString();
    });

    int32 BestPriority = INDEX_NONE;
    bool bConflict = false;
    EWorldRegionKind BestKind = EWorldRegionKind::Unknown;
    for (const FName Tag : Tags)
    {
        EWorldRegionKind CandidateKind = EWorldRegionKind::Unknown;
        EWorldClassificationSource CandidateSource = EWorldClassificationSource::DefaultUnknown;
        if (!UWorldClassificationSubsystem::TryClassifyRegionTag(Tag, CandidateKind, CandidateSource))
        {
            continue;
        }

        const int32 CandidatePriority = CandidateSource == EWorldClassificationSource::LandscapeLayer ? 80 : 60;
        if (CandidatePriority > BestPriority)
        {
            BestPriority = CandidatePriority;
            BestKind = CandidateKind;
            bConflict = false;
        }
        else if (CandidatePriority == BestPriority && CandidateKind != BestKind)
        {
            bConflict = true;
        }
    }

    if (BestPriority != INDEX_NONE)
    {
        OutKind = bConflict ? EWorldRegionKind::Transition : BestKind;
        return true;
    }

    // Do not assume unmarked terrain is editable. It must have a buildable-world marker.
    if (Actor && Cast<ALandscapeProxy>(Actor))
    {
        OutKind = EWorldRegionKind::StaticGeometry;
        return true;
    }
    if (Component && Component->Mobility == EComponentMobility::Static)
    {
        OutKind = EWorldRegionKind::StaticGeometry;
        return true;
    }
    return false;
}

void UWorldScanSubsystem::ApplyRegionSafety(
    const AActor* Actor,
    const UPrimitiveComponent* Component,
    EWorldRegionKind RegionKind,
    FWorldScanHit& OutHit) const
{
    // The shared scan core can infer actor capabilities, but editable terrain is
    // the only source of excavation permission.
    OutHit.bCanDig = RegionKind == EWorldRegionKind::EditableVoxel
        && HasActorOrComponentTag(Actor, Component, TEXT("World.Capability.Dig"))
        && !HasActorOrComponentTag(Actor, Component, TEXT("World.Capability.NoDig"));

    // A classified terrain surface is a physical support candidate even if its
    // runtime mesh component is Movable. Stable placement/snap solvers still run later.
    if (RegionKind == EWorldRegionKind::EditableVoxel)
    {
        OutHit.bCanSupport = true;
        OutHit.bCanSnap = true;
    }
    else if (RegionKind == EWorldRegionKind::StaticGeometry)
    {
        OutHit.bCanSupport = true;
    }

    if (RegionKind == EWorldRegionKind::Water || RegionKind == EWorldRegionKind::Unknown)
    {
        OutHit.bCanSupport = false;
    }
    if (RegionKind == EWorldRegionKind::Transition)
    {
        OutHit.bCanDig = false;
        // A transition cell is not safe to use as the authority for support.
        OutHit.bCanSupport = false;
    }

    if (HasActorOrComponentTag(Actor, Component, TEXT("World.Capability.NoSupport")))
    {
        OutHit.bCanSupport = false;
    }
    if (HasActorOrComponentTag(Actor, Component, TEXT("World.Capability.NoSnap")))
    {
        OutHit.bCanSnap = false;
    }
    if (IsActorOrComponentUnstable(Actor, Component))
    {
        OutHit.bCanSupport = false;
    }
}

bool UWorldScanSubsystem::Trace(const FWorldScanQuery& Query, FWorldScanHit& OutHit) const
{
    OutHit = FWorldScanHit{};
    FString Error;
    if (!Query.Validate(Error) || !IsWorldCompatible(Query.World))
    {
        return false;
    }

    WorldScanSystem::FScanResult LegacyScan;
    if (!WorldScanSystem::Trace(
        Query.World,
        Query.StartCm,
        Query.EndCm,
        Query.ActorToIgnore,
        LegacyScan,
        Query.Channel,
        Query.bTraceComplex,
        &Query.ComponentsToIgnore,
        Query.bReturnPhysicalMaterial))
    {
        return false;
    }

    WorldScanSystem::FWorldAnalysis Analysis;
    if (!WorldScanSystem::AnalyzeHit(LegacyScan, Analysis))
    {
        return false;
    }

    OutHit.bHit = true;
    OutHit.Hit = LegacyScan.Hit;
    OutHit.ObjectClass = LegacyScan.Hit.GetActor()->GetClass()->GetFName();
    OutHit.bCanSupport = Analysis.HasCapability(WorldScanSystem::ECapability::CanSupport);
    OutHit.bCanSnap = Analysis.HasCapability(WorldScanSystem::ECapability::CanSnap);
    OutHit.bCanHarvest = Analysis.HasCapability(WorldScanSystem::ECapability::CanHarvest);
    OutHit.bCanCarry = Analysis.HasCapability(WorldScanSystem::ECapability::CanCarry);
    OutHit.bCanInteract = Analysis.HasCapability(WorldScanSystem::ECapability::CanInteract);

    ResolveRegionKind(
        LegacyScan.Hit.GetActor(),
        LegacyScan.Hit.GetComponent(),
        LegacyScan.Hit.ImpactPoint,
        OutHit.RegionKind);
    ApplyRegionSafety(
        LegacyScan.Hit.GetActor(),
        LegacyScan.Hit.GetComponent(),
        OutHit.RegionKind,
        OutHit);
    return true;
}

bool UWorldScanSubsystem::ShapeTrace(
    UWorld* World,
    const FWorldShapeTraceRequest& Request,
    const AActor* ActorToIgnore,
    FWorldShapeTraceResult& OutResult,
    ECollisionChannel Channel) const
{
    OutResult = FWorldShapeTraceResult{};
    FString Error;
    if (!IsWorldCompatible(World) || !Request.Validate(Error)
        || FVector::DistSquared(Request.StartCm, Request.EndCm) <= FMath::Square(KINDA_SMALL_NUMBER))
    {
        return false;
    }

    WorldScanSystem::FShapeTraceRequest LegacyRequest;
    switch (Request.Shape)
    {
    case EWorldShapeTraceKind::Sphere:
        LegacyRequest.Type = WorldScanSystem::EShapeTraceType::Sphere;
        break;
    case EWorldShapeTraceKind::Box:
        LegacyRequest.Type = WorldScanSystem::EShapeTraceType::Box;
        break;
    case EWorldShapeTraceKind::Capsule:
        LegacyRequest.Type = WorldScanSystem::EShapeTraceType::Capsule;
        break;
    default:
        return false;
    }
    LegacyRequest.RadiusCm = Request.RadiusCm;
    LegacyRequest.HalfExtentCm = Request.HalfExtentCm;
    LegacyRequest.HalfHeightCm = Request.HalfHeightCm;

    WorldScanSystem::FShapeScanResult LegacyResult;
    // The legacy helper's bool means "hit"; the provider's bool means "valid query ran".
    WorldScanSystem::ShapeTrace(
        World, Request.StartCm, Request.EndCm, LegacyRequest, ActorToIgnore,
        LegacyResult, Channel, false, Request.bMultiHit);
    OutResult.Hits = MoveTemp(LegacyResult.Hits);
    OutResult.bSucceeded = true;
    return true;
}

bool UWorldScanSubsystem::QueryOverlaps(
    UWorld* World,
    const FWorldOverlapRequest& Request,
    const AActor* ActorToIgnore,
    FWorldOverlapResult& OutResult,
    ECollisionChannel Channel) const
{
    OutResult = FWorldOverlapResult{};
    FString Error;
    if (!IsWorldCompatible(World) || !Request.Validate(Error))
    {
        return false;
    }

    WorldScanSystem::FOverlapScanResult LegacyResult;
    switch (Request.Shape)
    {
    case EWorldOverlapShape::Sphere:
        WorldScanSystem::OverlapSphere(
            World, Request.CenterCm, Request.RadiusCm, ActorToIgnore, LegacyResult, Channel);
        break;
    case EWorldOverlapShape::OrientedBox:
        WorldScanSystem::OverlapBox(
            World, Request.CenterCm, Request.HalfExtentCm, Request.Rotation,
            ActorToIgnore, LegacyResult, Channel);
        break;
    default:
        return false;
    }

    OutResult.Overlaps = MoveTemp(LegacyResult.Overlaps);
    OutResult.bSucceeded = true;
    return true;
}

bool UWorldScanSubsystem::SampleSurface(
    UWorld* World,
    const FWorldSurfaceSampleRequest& Request,
    const AActor* ActorToIgnore,
    FWorldSurfaceSampleResult& OutResult) const
{
    OutResult = FWorldSurfaceSampleResult{};
    FString Error;
    if (!IsWorldCompatible(World) || !Request.Validate(Error))
    {
        OutResult.Error = Error.IsEmpty()
            ? TEXT("Surface scan was passed a world different from this subsystem's world.")
            : Error;
        return false;
    }

    int32 ProfileMaxSamplesPerAxis = 5;
    if (const UWorldRuntimeProfileSubsystem* Profile = World->GetSubsystem<UWorldRuntimeProfileSubsystem>())
    {
        ProfileMaxSamplesPerAxis = Profile->GetActiveSettings().MaxSurfaceSamplesPerAxis;
    }
    if (Request.SamplesX > ProfileMaxSamplesPerAxis || Request.SamplesY > ProfileMaxSamplesPerAxis)
    {
        OutResult.Error = FString::Printf(
            TEXT("Surface sample grid (%d x %d) exceeds current profile cap (%d per axis)."),
            Request.SamplesX,
            Request.SamplesY,
            ProfileMaxSamplesPerAxis);
        return false;
    }

    WorldScanSystem::FGeometrySampleSettings LegacyRequest;
    LegacyRequest.HalfExtentCm = Request.HalfExtentCm;
    LegacyRequest.SamplesX = Request.SamplesX;
    LegacyRequest.SamplesY = Request.SamplesY;
    LegacyRequest.TraceUpCm = Request.TraceUpCm;
    LegacyRequest.TraceDownCm = Request.TraceDownCm;
    LegacyRequest.Channel = ECC_Visibility;
    LegacyRequest.bTraceComplex = false;

    WorldScanSystem::FGeometrySampleResult LegacyResult;
    if (!WorldScanSystem::SampleSurface(World, Request.CenterCm, LegacyRequest, ActorToIgnore, LegacyResult))
    {
        OutResult.Error = TEXT("No surface hit was returned by the requested sample grid.");
        return false;
    }

    OutResult.Points = MoveTemp(LegacyResult.SamplePoints);
    OutResult.Normals = MoveTemp(LegacyResult.SampleNormals);
    OutResult.MinHeightCm = LegacyResult.MinHeightCm;
    OutResult.MaxHeightCm = LegacyResult.MaxHeightCm;
    OutResult.MaxSlopeDegrees = LegacyResult.MaxSlopeDegrees;
    OutResult.bSucceeded = LegacyResult.bValid;
    return OutResult.bSucceeded;
}

bool UWorldScanSubsystem::AnalyzeActor(const AActor* Actor, FWorldActorSnapshot& OutSnapshot) const
{
    OutSnapshot = FWorldActorSnapshot{};
    if (!IsValid(Actor))
    {
        return false;
    }

    const UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
    if (!Primitive)
    {
        TArray<UPrimitiveComponent*> Components;
        Actor->GetComponents<UPrimitiveComponent>(Components);
        Components.Sort([](const UPrimitiveComponent& A, const UPrimitiveComponent& B)
        {
            return A.GetPathName() < B.GetPathName();
        });
        Primitive = Components.IsEmpty() ? nullptr : Components[0];
    }

    WorldScanSystem::FBoundsAnalysis Bounds;
    if (!WorldScanSystem::AnalyzeBounds(Actor, Primitive, Bounds))
    {
        return false;
    }

    WorldScanSystem::FPhysicsStateAnalysis Physics;
    WorldScanSystem::QueryPhysicsState(Actor, Primitive, Physics);

    OutSnapshot.bValid = true;
    OutSnapshot.Bounds = Bounds.Bounds;
    OutSnapshot.OriginCm = Bounds.Origin;
    OutSnapshot.DimensionsCm = Bounds.Dimensions;
    OutSnapshot.LinearVelocityCmPerSec = Physics.LinearVelocityCmPerSec;
    OutSnapshot.AngularVelocityRadiansPerSec = Physics.AngularVelocityRadiansPerSec;
    OutSnapshot.bSimulatingPhysics = Physics.bSimulatingPhysics;
    OutSnapshot.bRigidBodyAwake = Physics.bRigidBodyAwake;
    return true;
}

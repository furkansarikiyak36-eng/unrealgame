// Copyright Epic Games, Inc. All Rights Reserved.

#include "WorldScanSubsystem.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "LandscapeProxy.h"
#include "WorldCore/WorldClassificationSubsystem.h"
#include "WorldCore/WorldRuntimeProfileSubsystem.h"

namespace
{
    constexpr float MovingSpeedThresholdCmPerSec = 5.0f;

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
            return Component->GetPhysicsLinearVelocity().SizeSquared()
                > FMath::Square(MovingSpeedThresholdCmPerSec);
        }

        return Actor && Actor->GetVelocity().SizeSquared()
            > FMath::Square(MovingSpeedThresholdCmPerSec);
    }

    FCollisionShape MakeQueryShape(const FWorldShapeTraceRequest& Request)
    {
        switch (Request.Shape)
        {
        case EWorldShapeTraceKind::Box:
            return FCollisionShape::MakeBox(Request.HalfExtentCm);
        case EWorldShapeTraceKind::Capsule:
            return FCollisionShape::MakeCapsule(Request.RadiusCm, Request.HalfHeightCm);
        case EWorldShapeTraceKind::Sphere:
        default:
            return FCollisionShape::MakeSphere(Request.RadiusCm);
        }
    }

    FCollisionShape MakeOverlapShape(const FWorldOverlapRequest& Request)
    {
        switch (Request.Shape)
        {
        case EWorldOverlapShape::OrientedBox:
            return FCollisionShape::MakeBox(Request.HalfExtentCm);
        case EWorldOverlapShape::Sphere:
        default:
            return FCollisionShape::MakeSphere(Request.RadiusCm);
        }
    }

    bool IsFiniteFloat(float Value)
    {
        return FMath::IsFinite(Value);
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

    for (const FName Tag : Tags)
    {
        EWorldClassificationSource Source;
        if (UWorldClassificationSubsystem::TryClassifyRegionTag(Tag, OutKind, Source))
        {
            return true;
        }
    }

    // Conservative fallback: an unmarked static landscape/mesh is not editable.
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

void UWorldScanSubsystem::ResolveCapabilities(
    const AActor* Actor,
    const UPrimitiveComponent* Component,
    EWorldRegionKind RegionKind,
    FWorldScanHit& OutHit) const
{
    OutHit.bCanDig = RegionKind == EWorldRegionKind::EditableVoxel;

    switch (RegionKind)
    {
    case EWorldRegionKind::EditableVoxel:
    case EWorldRegionKind::StaticGeometry:
        OutHit.bCanSupport = true;
        break;
    case EWorldRegionKind::Transition:
        // Transition boundaries block digging but can be physical support if static.
        OutHit.bCanSupport = Component
            && Component->Mobility == EComponentMobility::Static
            && !(Component->IsSimulatingPhysics());
        break;
    case EWorldRegionKind::Water:
    case EWorldRegionKind::Unknown:
    default:
        OutHit.bCanSupport = false;
        break;
    }

    if (HasActorOrComponentTag(Actor, Component, TEXT("World.Object.Tree")))
    {
        OutHit.bCanHarvest = true;
        OutHit.bCanCarry = true;
    }
    if (HasActorOrComponentTag(Actor, Component, TEXT("World.Object.Log")))
    {
        OutHit.bCanSupport = true;
        OutHit.bCanSnap = true;
        OutHit.bCanHarvest = true;
        OutHit.bCanCarry = true;
    }
    if (HasActorOrComponentTag(Actor, Component, TEXT("World.Object.Rock")))
    {
        OutHit.bCanSupport = true;
        OutHit.bCanHarvest = true;
        OutHit.bCanCarry = true;
    }
    if (HasActorOrComponentTag(Actor, Component, TEXT("World.Object.Stone")))
    {
        OutHit.bCanSupport = true;
        OutHit.bCanSnap = true;
        OutHit.bCanHarvest = true;
        OutHit.bCanCarry = true;
    }
    if (HasActorOrComponentTag(Actor, Component, TEXT("World.Object.Ground")))
    {
        OutHit.bCanSupport = true;
    }
    if (HasActorOrComponentTag(Actor, Component, TEXT("World.Object.Structure")))
    {
        OutHit.bCanSupport = true;
        OutHit.bCanSnap = true;
    }
    if (HasActorOrComponentTag(Actor, Component, TEXT("World.Object.RawResource")))
    {
        OutHit.bCanHarvest = true;
        OutHit.bCanInteract = true;
    }

    // Capability overrides are deliberate opt-outs for destructive/unsafe defaults,
    // or explicit opt-ins for interactions with otherwise unknown tagged actors.
    if (HasActorOrComponentTag(Actor, Component, TEXT("World.Capability.Harvest")))
    {
        OutHit.bCanHarvest = true;
    }
    if (HasActorOrComponentTag(Actor, Component, TEXT("World.Capability.Carry")))
    {
        OutHit.bCanCarry = true;
    }
    if (HasActorOrComponentTag(Actor, Component, TEXT("World.Capability.Interact")))
    {
        OutHit.bCanInteract = true;
    }
    if (HasActorOrComponentTag(Actor, Component, TEXT("World.Capability.Snap")))
    {
        OutHit.bCanSnap = true;
    }
    if (HasActorOrComponentTag(Actor, Component, TEXT("World.Capability.NoSnap")))
    {
        OutHit.bCanSnap = false;
    }
    if (HasActorOrComponentTag(Actor, Component, TEXT("World.Capability.NoSupport")))
    {
        OutHit.bCanSupport = false;
    }
    if (HasActorOrComponentTag(Actor, Component, TEXT("World.Capability.NoDig")))
    {
        OutHit.bCanDig = false;
    }

    // A classification override must never allow digging in an explicitly static region.
    if (RegionKind != EWorldRegionKind::EditableVoxel)
    {
        OutHit.bCanDig = false;
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

    FCollisionQueryParams Params(SCENE_QUERY_STAT(WorldScanContractTrace), Query.bTraceComplex);
    Params.bReturnPhysicalMaterial = Query.bReturnPhysicalMaterial;
    Params.bFindInitialOverlaps = false;
    if (Query.ActorToIgnore)
    {
        Params.AddIgnoredActor(Query.ActorToIgnore);
    }
    for (const UPrimitiveComponent* Component : Query.ComponentsToIgnore)
    {
        if (IsValid(Component))
        {
            Params.AddIgnoredComponent(Component);
        }
    }

    FHitResult Hit;
    const bool bHit = Query.World->LineTraceSingleByChannel(
        Hit, Query.StartCm, Query.EndCm, Query.Channel, Params);
    if (!bHit || !Hit.bBlockingHit || !Hit.GetActor())
    {
        return false;
    }

    OutHit.bHit = true;
    OutHit.Hit = Hit;
    OutHit.ObjectClass = Hit.GetActor()->GetClass()->GetFName();

    ResolveRegionKind(Hit.GetActor(), Hit.GetComponent(), Hit.ImpactPoint, OutHit.RegionKind);
    ResolveCapabilities(Hit.GetActor(), Hit.GetComponent(), OutHit.RegionKind, OutHit);
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
    if (!IsWorldCompatible(World) || !Request.Validate(Error))
    {
        return false;
    }

    FCollisionQueryParams Params(SCENE_QUERY_STAT(WorldScanContractShapeTrace), false);
    Params.bFindInitialOverlaps = false;
    if (ActorToIgnore)
    {
        Params.AddIgnoredActor(ActorToIgnore);
    }

    const FCollisionShape Shape = MakeQueryShape(Request);
    if (Request.bMultiHit)
    {
        World->SweepMultiByChannel(
            OutResult.Hits, Request.StartCm, Request.EndCm, FQuat::Identity, Channel, Shape, Params);
    }
    else
    {
        FHitResult Hit;
        if (World->SweepSingleByChannel(
            Hit, Request.StartCm, Request.EndCm, FQuat::Identity, Channel, Shape, Params))
        {
            OutResult.Hits.Add(MoveTemp(Hit));
        }
    }

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

    FCollisionQueryParams Params(SCENE_QUERY_STAT(WorldScanContractOverlap), false);
    Params.bFindInitialOverlaps = true;
    if (ActorToIgnore)
    {
        Params.AddIgnoredActor(ActorToIgnore);
    }

    const FCollisionShape Shape = MakeOverlapShape(Request);
    World->OverlapMultiByChannel(
        OutResult.Overlaps, Request.CenterCm, Request.Rotation, Channel, Shape, Params);

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

    FCollisionQueryParams Params(SCENE_QUERY_STAT(WorldScanContractSurfaceSamples), false);
    Params.bFindInitialOverlaps = false;
    if (ActorToIgnore)
    {
        Params.AddIgnoredActor(ActorToIgnore);
    }

    float MinHeight = TNumericLimits<float>::Max();
    float MaxHeight = TNumericLimits<float>::Lowest();
    float MaxSlope = 0.0f;

    for (int32 Y = 0; Y < Request.SamplesY; ++Y)
    {
        const float YAlpha = Request.SamplesY == 1 ? 0.5f : static_cast<float>(Y) / (Request.SamplesY - 1);
        const float YOffset = FMath::Lerp(-Request.HalfExtentCm.Y, Request.HalfExtentCm.Y, YAlpha);
        for (int32 X = 0; X < Request.SamplesX; ++X)
        {
            const float XAlpha = Request.SamplesX == 1 ? 0.5f : static_cast<float>(X) / (Request.SamplesX - 1);
            const float XOffset = FMath::Lerp(-Request.HalfExtentCm.X, Request.HalfExtentCm.X, XAlpha);
            const FVector SampleCenter = Request.CenterCm + FVector(XOffset, YOffset, 0.0f);
            const FVector Start(SampleCenter.X, SampleCenter.Y, Request.CenterCm.Z + Request.TraceUpCm);
            const FVector End(SampleCenter.X, SampleCenter.Y, Request.CenterCm.Z - Request.TraceDownCm);

            FHitResult Hit;
            if (!World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params)
                || !Hit.bBlockingHit)
            {
                continue;
            }

            const FVector Normal = Hit.ImpactNormal.GetSafeNormal();
            const float Slope = FMath::RadiansToDegrees(
                FMath::Acos(FMath::Clamp(FVector::DotProduct(Normal, FVector::UpVector), -1.0f, 1.0f)));
            OutResult.Points.Add(Hit.ImpactPoint);
            OutResult.Normals.Add(Normal);
            MinHeight = FMath::Min(MinHeight, Hit.ImpactPoint.Z);
            MaxHeight = FMath::Max(MaxHeight, Hit.ImpactPoint.Z);
            MaxSlope = FMath::Max(MaxSlope, Slope);
        }
    }

    OutResult.bSucceeded = !OutResult.Points.IsEmpty();
    if (!OutResult.bSucceeded)
    {
        OutResult.Error = TEXT("No surface hit was returned by the requested sample grid.");
        return false;
    }

    OutResult.MinHeightCm = MinHeight;
    OutResult.MaxHeightCm = MaxHeight;
    OutResult.MaxSlopeDegrees = MaxSlope;
    return true;
}

bool UWorldScanSubsystem::AnalyzeActor(const AActor* Actor, FWorldActorSnapshot& OutSnapshot) const
{
    OutSnapshot = FWorldActorSnapshot{};
    if (!IsValid(Actor))
    {
        return false;
    }

    FVector Origin = FVector::ZeroVector;
    FVector Extent = FVector::ZeroVector;
    Actor->GetActorBounds(false, Origin, Extent, true);
    const FBox Bounds(Origin - Extent, Origin + Extent);
    if (!Bounds.IsValid
        || !WorldSystemContractValidation::IsFiniteVector(Origin)
        || !WorldSystemContractValidation::IsFiniteVector(Extent))
    {
        return false;
    }

    OutSnapshot.bValid = true;
    OutSnapshot.Bounds = Bounds;
    OutSnapshot.OriginCm = Origin;
    OutSnapshot.DimensionsCm = Extent * 2.0f;
    OutSnapshot.LinearVelocityCmPerSec = Actor->GetVelocity();

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

    if (Primitive)
    {
        OutSnapshot.bSimulatingPhysics = Primitive->IsSimulatingPhysics();
        OutSnapshot.bRigidBodyAwake = Primitive->IsAnyRigidBodyAwake();
        if (OutSnapshot.bSimulatingPhysics)
        {
            OutSnapshot.LinearVelocityCmPerSec = Primitive->GetPhysicsLinearVelocity();
            OutSnapshot.AngularVelocityRadiansPerSec = Primitive->GetPhysicsAngularVelocityInRadians();
        }
    }

    return true;
}

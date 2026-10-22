// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldCore/WorldSystemContracts.h"
#include "WorldScanSubsystem.generated.h"

/**
 * Demand-driven world query implementation for the LOW_MID vertical slice.
 * No Tick polling and no automatic multi-query fan-out: a caller requests each query.
 */
UCLASS()
class MYPROJECT2_API UWorldScanSubsystem final
    : public UWorldSubsystem
    , public IWorldScanProvider
{
    GENERATED_BODY()

public:
    virtual bool Trace(const FWorldScanQuery& Query, FWorldScanHit& OutHit) const override;

    virtual bool ShapeTrace(
        UWorld* World,
        const FWorldShapeTraceRequest& Request,
        const AActor* ActorToIgnore,
        FWorldShapeTraceResult& OutResult,
        ECollisionChannel Channel = ECC_Visibility) const override;

    virtual bool QueryOverlaps(
        UWorld* World,
        const FWorldOverlapRequest& Request,
        const AActor* ActorToIgnore,
        FWorldOverlapResult& OutResult,
        ECollisionChannel Channel = ECC_Visibility) const override;

    virtual bool SampleSurface(
        UWorld* World,
        const FWorldSurfaceSampleRequest& Request,
        const AActor* ActorToIgnore,
        FWorldSurfaceSampleResult& OutResult) const override;

    virtual bool AnalyzeActor(const AActor* Actor, FWorldActorSnapshot& OutSnapshot) const override;

private:
    bool ResolveRegionKind(
        const AActor* Actor,
        const UPrimitiveComponent* Component,
        const FVector& WorldPositionCm,
        EWorldRegionKind& OutKind) const;

    void ApplyRegionSafety(
        const AActor* Actor,
        const UPrimitiveComponent* Component,
        EWorldRegionKind RegionKind,
        FWorldScanHit& OutHit) const;

    bool IsWorldCompatible(UWorld* World) const;
};

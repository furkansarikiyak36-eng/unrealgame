// Copyright Epic Games, Inc. All Rights Reserved.

#include "WorldScanSystem.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

bool WorldScanSystem::ExecuteScanPlan(
	UWorld* World, const FWorldScanRequest& Request, const AActor* ActorToIgnore,
	const AActor* TargetActorOverride, const UPrimitiveComponent* TargetComponentOverride,
	FWorldScanBundle& OutBundle)
{
	OutBundle = FWorldScanBundle{};
	if (!World)
	{
		return false;
	}

	if (Request.bRunRaycast)
	{
		OutBundle.bRaycastAttempted = true;
		Trace(World, Request.TraceStart, Request.TraceEnd, ActorToIgnore,
			OutBundle.Raycast, Request.TraceChannel, Request.bTraceComplex);
		if (OutBundle.Raycast.bHit)
		{
			AnalyzeHit(OutBundle.Raycast, OutBundle.TargetAnalysis);
		}
	}

	if (Request.bRunShapeTrace)
	{
		OutBundle.bShapeTraceAttempted = true;
		ShapeTrace(World, Request.ShapeTraceStart, Request.ShapeTraceEnd, Request.ShapeRequest,
			ActorToIgnore, OutBundle.ShapeTrace, Request.TraceChannel, Request.bTraceComplex, false);
	}

	if (Request.bRunOverlapSphere)
	{
		OutBundle.bOverlapSphereAttempted = true;
		OverlapSphere(World, Request.QueryCenter, Request.OverlapRadiusCm, ActorToIgnore,
			OutBundle.OverlapSphere, Request.OverlapChannel);
	}

	if (Request.bRunOverlapBox)
	{
		OutBundle.bOverlapBoxAttempted = true;
		OverlapBox(World, Request.QueryCenter, Request.OverlapHalfExtentCm, Request.OverlapRotation,
			ActorToIgnore, OutBundle.OverlapBox, Request.OverlapChannel);
	}

	if (Request.bRunSurfaceSampling)
	{
		OutBundle.bSurfaceSamplingAttempted = true;
		SampleSurface(World, Request.QueryCenter, Request.GeometrySettings, ActorToIgnore, OutBundle.Surface);
	}

	const AActor* TargetActor = TargetActorOverride;
	const UPrimitiveComponent* TargetComponent = TargetComponentOverride;
	if (!TargetActor && OutBundle.Raycast.bHit)
	{
		TargetActor = OutBundle.Raycast.Hit.GetActor();
	}
	if (!TargetComponent && OutBundle.Raycast.bHit)
	{
		TargetComponent = OutBundle.Raycast.Hit.GetComponent();
	}

	if (Request.bRunBoundsAnalysis)
	{
		OutBundle.bBoundsAnalysisAttempted = true;
		AnalyzeBounds(TargetActor, TargetComponent, OutBundle.Bounds);
	}
	if (Request.bRunPhysicsQuery)
	{
		OutBundle.bPhysicsQueryAttempted = true;
		QueryPhysicsState(TargetActor, TargetComponent, OutBundle.Physics);
	}

	// True means the requested plan was dispatched against a valid world, not that a hit was guaranteed.
	return true;
}

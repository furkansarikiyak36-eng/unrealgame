// Copyright Epic Games, Inc. All Rights Reserved.

#include "WorldScanSystem.h"

#include "CollisionShape.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace
{
	FCollisionShape MakeScanShape(const WorldScanSystem::FShapeTraceRequest& Request)
	{
		using WorldScanSystem::EShapeTraceType;
		switch (Request.Type)
		{
		case EShapeTraceType::Box:
			return FCollisionShape::MakeBox(FVector(
				FMath::Max(0.1f, Request.HalfExtentCm.X),
				FMath::Max(0.1f, Request.HalfExtentCm.Y),
				FMath::Max(0.1f, Request.HalfExtentCm.Z)));
		case EShapeTraceType::Capsule:
			return FCollisionShape::MakeCapsule(
				FMath::Max(0.1f, Request.RadiusCm),
				FMath::Max(Request.RadiusCm, Request.HalfHeightCm));
		case EShapeTraceType::Sphere:
		default:
			return FCollisionShape::MakeSphere(FMath::Max(0.1f, Request.RadiusCm));
		}
	}

	FCollisionQueryParams MakeQueryParams(const AActor* ActorToIgnore, bool bTraceComplex, const TCHAR* StatName)
	{
		FCollisionQueryParams Params(FName(StatName), bTraceComplex);
		Params.bFindInitialOverlaps = true;
		if (ActorToIgnore)
		{
			Params.AddIgnoredActor(ActorToIgnore);
		}
		return Params;
	}
}

bool WorldScanSystem::ShapeTrace(
	UWorld* World, const FVector& Start, const FVector& End,
	const FShapeTraceRequest& Request, const AActor* ActorToIgnore,
	FShapeScanResult& OutResult, ECollisionChannel Channel, bool bTraceComplex, bool bMulti)
{
	OutResult = FShapeScanResult{};
	OutResult.TraceStart = Start;
	OutResult.TraceEnd = End;
	OutResult.TraceLengthCm = FVector::Distance(Start, End);
	if (!World || OutResult.TraceLengthCm <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FCollisionShape Shape = MakeScanShape(Request);
	FCollisionQueryParams Params = MakeQueryParams(ActorToIgnore, bTraceComplex, TEXT("WorldScanSystemShapeTrace"));
	bool bHit = false;
	if (bMulti)
	{
		bHit = World->SweepMultiByChannel(OutResult.Hits, Start, End, FQuat::Identity, Channel, Shape, Params);
	}
	else
	{
		FHitResult Hit;
		bHit = World->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, Channel, Shape, Params);
		if (bHit)
		{
			OutResult.Hits.Add(MoveTemp(Hit));
		}
	}
	OutResult.bHit = bHit;
	return bHit;
}

bool WorldScanSystem::OverlapSphere(
	UWorld* World, const FVector& Center, float RadiusCm, const AActor* ActorToIgnore,
	FOverlapScanResult& OutResult, ECollisionChannel Channel)
{
	OutResult = FOverlapScanResult{};
	OutResult.Center = Center;
	if (!World || RadiusCm <= 0.f)
	{
		return false;
	}

	const FCollisionShape Shape = FCollisionShape::MakeSphere(RadiusCm);
	FCollisionQueryParams Params = MakeQueryParams(ActorToIgnore, false, TEXT("WorldScanSystemOverlapSphere"));
	OutResult.bAnyOverlap = World->OverlapMultiByChannel(
		OutResult.Overlaps, Center, FQuat::Identity, Channel, Shape, Params);
	return OutResult.bAnyOverlap;
}

bool WorldScanSystem::OverlapBox(
	UWorld* World, const FVector& Center, const FVector& HalfExtentCm, const FQuat& Rotation,
	const AActor* ActorToIgnore, FOverlapScanResult& OutResult, ECollisionChannel Channel)
{
	OutResult = FOverlapScanResult{};
	OutResult.Center = Center;
	if (!World || HalfExtentCm.X <= 0.f || HalfExtentCm.Y <= 0.f || HalfExtentCm.Z <= 0.f)
	{
		return false;
	}

	const FCollisionShape Shape = FCollisionShape::MakeBox(HalfExtentCm);
	FCollisionQueryParams Params = MakeQueryParams(ActorToIgnore, false, TEXT("WorldScanSystemOverlapBox"));
	OutResult.bAnyOverlap = World->OverlapMultiByChannel(
		OutResult.Overlaps, Center, Rotation, Channel, Shape, Params);
	return OutResult.bAnyOverlap;
}

bool WorldScanSystem::SampleSurface(
	UWorld* World, const FVector& Center, const FGeometrySampleSettings& Settings,
	const AActor* ActorToIgnore, FGeometrySampleResult& OutResult)
{
	OutResult = FGeometrySampleResult{};
	if (!World || Settings.SamplesX < 1 || Settings.SamplesY < 1 ||
		Settings.TraceUpCm <= 0.f || Settings.TraceDownCm <= 0.f ||
		Settings.HalfExtentCm.X < 0.f || Settings.HalfExtentCm.Y < 0.f)
	{
		return false;
	}

	const int32 SamplesX = FMath::Clamp(Settings.SamplesX, 1, 32);
	const int32 SamplesY = FMath::Clamp(Settings.SamplesY, 1, 32);
	OutResult.RequestedSamples = SamplesX * SamplesY;
	float MinZ = TNumericLimits<float>::Max();
	float MaxZ = TNumericLimits<float>::Lowest();
	FVector NormalSum = FVector::ZeroVector;

	for (int32 Y = 0; Y < SamplesY; ++Y)
	{
		const float AlphaY = SamplesY == 1 ? 0.5f : float(Y) / float(SamplesY - 1);
		const float OffsetY = FMath::Lerp(-Settings.HalfExtentCm.Y, Settings.HalfExtentCm.Y, AlphaY);
		for (int32 X = 0; X < SamplesX; ++X)
		{
			const float AlphaX = SamplesX == 1 ? 0.5f : float(X) / float(SamplesX - 1);
			const float OffsetX = FMath::Lerp(-Settings.HalfExtentCm.X, Settings.HalfExtentCm.X, AlphaX);
			const FVector SampleCenter = Center + FVector(OffsetX, OffsetY, 0.f);
			FScanResult Scan;
			const FVector Start = SampleCenter + FVector(0.f, 0.f, Settings.TraceUpCm);
			const FVector End = SampleCenter - FVector(0.f, 0.f, Settings.TraceDownCm);
			if (!Trace(World, Start, End, ActorToIgnore, Scan, Settings.Channel, Settings.bTraceComplex))
			{
				continue;
			}

			const FVector Point = Scan.Hit.ImpactPoint;
			const FVector Normal = Scan.Hit.ImpactNormal.GetSafeNormal();
			OutResult.SamplePoints.Add(Point);
			OutResult.SampleNormals.Add(Normal);
			NormalSum += Normal;
			MinZ = FMath::Min(MinZ, Point.Z);
			MaxZ = FMath::Max(MaxZ, Point.Z);
			const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(
				FVector::DotProduct(Normal, FVector::UpVector), -1.f, 1.f)));
			OutResult.MaxSlopeDegrees = FMath::Max(OutResult.MaxSlopeDegrees, Angle);
		}
	}

	OutResult.SuccessfulSamples = OutResult.SamplePoints.Num();
	if (OutResult.SuccessfulSamples == 0)
	{
		return false;
	}

	OutResult.MinHeightCm = MinZ;
	OutResult.MaxHeightCm = MaxZ;
	OutResult.HeightRangeCm = MaxZ - MinZ;
	OutResult.AverageNormal = NormalSum.GetSafeNormal();
	OutResult.bValid = true;
	return true;
}

bool WorldScanSystem::AnalyzeBounds(
	const AActor* Actor, const UPrimitiveComponent* PreferredComponent, FBoundsAnalysis& OutAnalysis)
{
	OutAnalysis = FBoundsAnalysis{};
	if (!Actor && !PreferredComponent)
	{
		return false;
	}

	if (PreferredComponent)
	{
		const FBoxSphereBounds& ComponentBounds = PreferredComponent->Bounds;
		OutAnalysis.Origin = ComponentBounds.Origin;
		OutAnalysis.Extent = ComponentBounds.BoxExtent;
		OutAnalysis.SphereRadiusCm = ComponentBounds.SphereRadius;
		OutAnalysis.Rotation = PreferredComponent->GetComponentQuat();
	}
	else
	{
		FVector Origin = FVector::ZeroVector;
		FVector Extent = FVector::ZeroVector;
		Actor->GetActorBounds(true, Origin, Extent, true);
		OutAnalysis.Origin = Origin;
		OutAnalysis.Extent = Extent;
		OutAnalysis.SphereRadiusCm = Extent.Size();
		OutAnalysis.Rotation = Actor->GetActorQuat();
	}
	OutAnalysis.Dimensions = OutAnalysis.Extent * 2.f;
	OutAnalysis.Bounds = FBox(OutAnalysis.Origin - OutAnalysis.Extent, OutAnalysis.Origin + OutAnalysis.Extent);
	OutAnalysis.bValid = OutAnalysis.Extent.GetMin() >= 0.f;
	return OutAnalysis.bValid;
}

bool WorldScanSystem::QueryPhysicsState(
	const AActor* Actor, const UPrimitiveComponent* PreferredComponent, FPhysicsStateAnalysis& OutAnalysis)
{
	OutAnalysis = FPhysicsStateAnalysis{};
	if (!Actor && !PreferredComponent)
	{
		return false;
	}

	const UPrimitiveComponent* Component = PreferredComponent;
	if (!Component && Actor)
	{
		Component = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
	}
	if (Component)
	{
		OutAnalysis.bSimulatingPhysics = Component->IsSimulatingPhysics();
		OutAnalysis.bRigidBodyAwake = Component->IsSimulatingPhysics() && Component->IsAnyRigidBodyAwake();
		if (Component->IsSimulatingPhysics())
		{
			OutAnalysis.LinearVelocityCmPerSec = Component->GetPhysicsLinearVelocity();
			OutAnalysis.AngularVelocityRadiansPerSec = Component->GetPhysicsAngularVelocityInRadians();
		}
	}
	if (OutAnalysis.LinearVelocityCmPerSec.IsNearlyZero() && Actor)
	{
		OutAnalysis.LinearVelocityCmPerSec = Actor->GetVelocity();
	}

	constexpr float MotionSpeedCmPerSec = 5.f;
	constexpr float FallingSpeedCmPerSec = 20.f;
	if (Component && Component->IsSimulatingPhysics() && !OutAnalysis.bRigidBodyAwake)
	{
		OutAnalysis.State = EPhysicalState::Sleeping;
	}
	else if (OutAnalysis.LinearVelocityCmPerSec.Z < -FallingSpeedCmPerSec)
	{
		OutAnalysis.State = EPhysicalState::Falling;
	}
	else if (OutAnalysis.LinearVelocityCmPerSec.SizeSquared() > FMath::Square(MotionSpeedCmPerSec) ||
		OutAnalysis.AngularVelocityRadiansPerSec.SizeSquared() > FMath::Square(0.05f))
	{
		OutAnalysis.State = EPhysicalState::Moving;
	}
	else if (Component && Component->Mobility == EComponentMobility::Static)
	{
		OutAnalysis.State = EPhysicalState::Static;
	}
	else
	{
		OutAnalysis.State = EPhysicalState::Unknown;
	}

	OutAnalysis.bValid = true;
	return true;
}

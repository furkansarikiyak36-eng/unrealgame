#include "Phase2_Transport/CarvableLogJackingComponent.h"

#include "CarvableLog.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "StructureLink.h"
#include "WorldScanSystem.h"

#include "Math/RotationMatrix.h"

namespace
{
	bool IsFiniteVector(const FVector& V)
	{
		return FMath::IsFinite(V.X) && FMath::IsFinite(V.Y) && FMath::IsFinite(V.Z);
	}
}

UCarvableLogJackingComponent::UCarvableLogJackingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UCarvableLogJackingComponent::LiftOneEnd(ACarvableLog* Log, float RaiseHeightCm, bool bRaiseEndB)
{
	if (!IsValid(Log) || !GetWorld() || Log->IsCarried()
		|| !FMath::IsFinite(RaiseHeightCm) || RaiseHeightCm <= 0.f
		|| !FMath::IsFinite(MaxLiftHeightCm) || RaiseHeightCm > MaxLiftHeightCm
		|| !FMath::IsFinite(MaxPitchDegrees) || MaxPitchDegrees <= 0.f || MaxPitchDegrees >= 90.f)
	{
		return false;
	}

	FVector Center, Axis;
	float Radius = 0.f, HalfLength = 0.f;
	if (!ACarvableLog::GetLogAxis(Log, Center, Axis, Radius, HalfLength)
		|| !IsFiniteVector(Center) || !IsFiniteVector(Axis)
		|| FMath::Abs(Axis.Z) > 0.25f || HalfLength <= KINDA_SMALL_NUMBER)
	{
		// This helper raises one end of a lying log; standing posts and already-steep pieces use other placement rules.
		return false;
	}

	const float Length = HalfLength * 2.f;
	const float MaxVerticalReach = Length * FMath::Sin(FMath::DegreesToRadians(MaxPitchDegrees));
	if (RaiseHeightCm > MaxVerticalReach)
	{
		return false;
	}

	const float Angle = FMath::Asin(FMath::Clamp(RaiseHeightCm / Length, 0.f, 1.f));
	const FVector FlatAxis = FVector(Axis.X, Axis.Y, 0.f).GetSafeNormal();
	if (FlatAxis.IsNearlyZero())
	{
		return false;
	}

	const FVector FixedEndpoint = Center + FlatAxis * (bRaiseEndB ? -HalfLength : HalfLength);
	const float Sign = bRaiseEndB ? 1.f : -1.f;
	const FVector NewAxis = (FlatAxis * FMath::Cos(Angle) + FVector::UpVector * (Sign * FMath::Sin(Angle))).GetSafeNormal();
	const FQuat NewRotation = FRotationMatrix::MakeFromXZ(NewAxis, FVector::UpVector).ToQuat();
	const FVector NewCenter = FixedEndpoint + NewAxis * (bRaiseEndB ? HalfLength : -HalfLength);
	const FTransform OldTransform = Log->GetActorTransform();
	const FBox LocalBox = Log->GetLocalBox();
	const FVector LocalCenter = LocalBox.GetCenter() * OldTransform.GetScale3D();
	const FVector NewLocation = NewCenter - NewRotation.RotateVector(LocalCenter);

	// A log lying loose on the ground (the usual case) is taken into the structure here: it is held where it is put.
	// No sweep: the fixed end stays on the ground it already touches, which a sweep would report as a blocking hit.
	Log->PlaceAt(FTransform(NewRotation, NewLocation, OldTransform.GetScale3D()), /*bAnchorIt*/ true);

	StructureLink::MarkChanged();
	return true;
}

bool UCarvableLogJackingComponent::GetEndpointGeometry(
	const ACarvableLog* Log,
	bool bRaiseEndB,
	FVector& OutEndpointCenter,
	FVector& OutEndpointAxis,
	float& OutRadius) const
{
	float HalfLength = 0.f;
	if (!ACarvableLog::GetLogAxis(Log, OutEndpointCenter, OutEndpointAxis, OutRadius, HalfLength)
		|| HalfLength <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	OutEndpointAxis = OutEndpointAxis.GetSafeNormal();
	OutEndpointCenter += OutEndpointAxis * (bRaiseEndB ? HalfLength : -HalfLength);
	return IsFiniteVector(OutEndpointCenter) && IsFiniteVector(OutEndpointAxis) && OutRadius > 0.f;
}

bool UCarvableLogJackingComponent::PlaceSupportUnder(
	ACarvableLog* ElevatedLog,
	ACarvableLog* SupportPiece,
	bool bRaiseEndB)
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(ElevatedLog) || !IsValid(SupportPiece)
		|| ElevatedLog == SupportPiece || ElevatedLog->IsCarried() || !ElevatedLog->IsAnchored()
		|| SupportPiece->IsAnchored() || SupportPiece->GetMotion() == ELogMotion::Installed
		|| SupportPiece->GetLength() < ACarvableLog::ModuleLength - 2.f
		|| FMath::Abs(SupportPiece->GetLength() / ACarvableLog::ModuleLength
			- FMath::RoundToFloat(SupportPiece->GetLength() / ACarvableLog::ModuleLength)) > 0.05f)
	{
		return false;
	}

	FVector Endpoint, Axis;
	float Radius = 0.f;
	if (!GetEndpointGeometry(ElevatedLog, bRaiseEndB, Endpoint, Axis, Radius))
	{
		return false;
	}

	const float VerticalCrossSectionRadius = Radius * FMath::Sqrt(FMath::Max(0.f, 1.f - FMath::Square(Axis.Z)));
	const FVector TraceStart(Endpoint.X, Endpoint.Y, Endpoint.Z + GroundTraceUpCm);
	const FVector TraceEnd(Endpoint.X, Endpoint.Y, Endpoint.Z - GroundTraceDownCm);
	WorldScanSystem::FScanResult GroundScan;
	if (!WorldScanSystem::Trace(World, TraceStart, TraceEnd, ElevatedLog, GroundScan, ECC_WorldStatic, false)
		|| !GroundScan.bHit || GroundScan.Hit.ImpactNormal.Z < 0.6f)
	{
		return false;
	}

	const float TargetTopZ = Endpoint.Z - VerticalCrossSectionRadius;
	const float RequiredHeight = TargetTopZ - GroundScan.Hit.ImpactPoint.Z;
	if (!FMath::IsFinite(RequiredHeight) || RequiredHeight <= 0.f
		|| FMath::Abs(RequiredHeight - SupportPiece->GetLength()) > SupportHeightToleranceCm)
	{
		// The player must raise the beam to a compatible module height; never stretch a support to fake a fit.
		return false;
	}

	const FTransform OriginalTransform = SupportPiece->GetActorTransform();
	USceneComponent* OriginalParent = SupportPiece->GetRootComponent()
		? SupportPiece->GetRootComponent()->GetAttachParent() : nullptr;
	const FTransform OriginalRelative = OriginalParent
		? OriginalTransform.GetRelativeTransform(OriginalParent->GetComponentTransform())
		: FTransform::Identity;
	const bool bWasCarried = SupportPiece->IsCarried();
	const FTransform StandTransform = SupportPiece->GetStandTransform(
		GroundScan.Hit.ImpactPoint, ElevatedLog->GetActorRotation().Yaw);
	SupportPiece->PlaceAt(StandTransform, /*bAnchorIt*/ true);
	StructureLink::MarkChanged();

	if (!StructureLink::WouldBeSupported(World, ElevatedLog, ElevatedLog->GetActorTransform()))
	{
		// Roll back the physical piece and its ownership if the structure graph does not accept the support.
		if (bWasCarried && OriginalParent)
		{
			SupportPiece->BeginCarry(OriginalParent, OriginalRelative);
		}
		else
		{
			SupportPiece->PlaceAt(OriginalTransform, /*bAnchorIt*/ false);
		}
		StructureLink::MarkChanged();
		return false;
	}

	StructureLink::Settle(World);
	StructureLink::MarkChanged();
	return true;
}

bool UCarvableLogJackingComponent::IsFullySupported(const ACarvableLog* Log) const
{
	return Log && GetWorld()
		&& StructureLink::WouldBeSupported(GetWorld(), Log, Log->GetActorTransform());
}

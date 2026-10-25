// Copyright Epic Games, Inc. All Rights Reserved.

#include "WorldSnapTwoAnchorSolver.h"

#include "Math/RotationMatrix.h"

namespace
{
	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X) && FMath::IsFinite(Value.Y) && FMath::IsFinite(Value.Z);
	}

	bool IsValidAnchor(const WorldSnap::FAnchor& Anchor)
	{
		return Anchor.Kind != WorldSnap::EAnchorKind::Unknown
			&& IsFiniteVector(Anchor.Position)
			&& IsFiniteVector(Anchor.Normal)
			&& !Anchor.Normal.IsNearlyZero()
			&& FMath::IsFinite(Anchor.MaxSupportedLoad)
			&& Anchor.MaxSupportedLoad >= 0.f;
	}
}

bool FWorldSnapTwoAnchorSolver::Solve(
	const WorldSnap::FTwoAnchorQuery& Query,
	WorldSnap::FTwoAnchorResult& OutResult)
{
	OutResult = WorldSnap::FTwoAnchorResult{};

	if (!FMath::IsFinite(Query.PieceLengthCm)
		|| !FMath::IsFinite(Query.LocalMinX)
		|| !FMath::IsFinite(Query.LocalMaxX)
		|| !FMath::IsFinite(Query.LengthToleranceCm)
		|| !FMath::IsFinite(Query.MaxEndPenetrationCm)
		|| !FMath::IsFinite(Query.MaxGapCm)
		|| !FMath::IsFinite(Query.RequiredLoad)
		|| !FMath::IsFinite(Query.MinPitchDegrees)
		|| !FMath::IsFinite(Query.MaxPitchDegrees)
		|| !IsFiniteVector(Query.UpHint)
		|| Query.PieceLengthCm <= KINDA_SMALL_NUMBER
		|| Query.LocalMaxX <= Query.LocalMinX
		|| Query.LengthToleranceCm < 0.f
		|| Query.MaxEndPenetrationCm < 0.f
		|| Query.MaxGapCm < 0.f
		|| Query.RequiredLoad < 0.f
		|| Query.MinPitchDegrees < -90.f
		|| Query.MaxPitchDegrees > 90.f
		|| Query.MinPitchDegrees > Query.MaxPitchDegrees
		|| FMath::Abs((Query.LocalMaxX - Query.LocalMinX) - Query.PieceLengthCm) > Query.LengthToleranceCm)
	{
		OutResult.Status = WorldSnap::ETwoAnchorStatus::InvalidQuery;
		return false;
	}

	if (!IsValidAnchor(Query.StartAnchor) || !IsValidAnchor(Query.EndAnchor))
	{
		OutResult.Status = WorldSnap::ETwoAnchorStatus::InvalidAnchor;
		return false;
	}

	if (!Query.StartAnchor.bAvailable || !Query.EndAnchor.bAvailable)
	{
		OutResult.Status = WorldSnap::ETwoAnchorStatus::AnchorUnavailable;
		return false;
	}

	if (Query.bRejectOccupied && (Query.StartAnchor.bOccupied || Query.EndAnchor.bOccupied))
	{
		OutResult.Status = WorldSnap::ETwoAnchorStatus::OccupiedAnchor;
		return false;
	}

	if (!Query.StartAnchor.bTargetCompatible || !Query.EndAnchor.bTargetCompatible)
	{
		OutResult.Status = WorldSnap::ETwoAnchorStatus::IncompatibleAnchor;
		return false;
	}

	if (Query.bRequireLoadBearing && (!Query.StartAnchor.bLoadBearing || !Query.EndAnchor.bLoadBearing))
	{
		OutResult.Status = WorldSnap::ETwoAnchorStatus::NonLoadBearing;
		return false;
	}

	if (Query.bRequireLoadBearing &&
		(!Query.StartAnchor.bSupportValid || !Query.EndAnchor.bSupportValid ||
		 Query.StartAnchor.MaxSupportedLoad < Query.RequiredLoad ||
		 Query.EndAnchor.MaxSupportedLoad < Query.RequiredLoad))
	{
		OutResult.Status = WorldSnap::ETwoAnchorStatus::InvalidSupport;
		return false;
	}

	if (Query.bRequireDistinctAnchors
		&& !Query.StartAnchor.Id.IsNone()
		&& Query.StartAnchor.Id == Query.EndAnchor.Id)
	{
		OutResult.Status = WorldSnap::ETwoAnchorStatus::SameAnchor;
		return false;
	}

	const FVector Delta = Query.EndAnchor.Position - Query.StartAnchor.Position;
	const float Distance = Delta.Size();
	if (!FMath::IsFinite(Distance) || Distance <= KINDA_SMALL_NUMBER)
	{
		OutResult.Status = WorldSnap::ETwoAnchorStatus::LengthMismatch;
		return false;
	}

	const float SignedLengthError = Distance - Query.PieceLengthCm;
	const float LengthError = FMath::Abs(SignedLengthError);
	OutResult.AnchorDistanceCm = Distance;
	OutResult.LengthErrorCm = LengthError;
	OutResult.SignedLengthErrorCm = SignedLengthError;
	const float AllowedGap = Query.LengthToleranceCm + Query.MaxGapCm;
	const float AllowedCompression = Query.LengthToleranceCm + 2.f * Query.MaxEndPenetrationCm;
	if (SignedLengthError > AllowedGap || SignedLengthError < -AllowedCompression)
	{
		OutResult.Status = WorldSnap::ETwoAnchorStatus::LengthMismatch;
		return false;
	}

	const FVector Direction = Delta / Distance;
	OutResult.YawDegrees = FMath::RadiansToDegrees(FMath::Atan2(Direction.Y, Direction.X));
	const float PitchDegrees = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(Direction.Z, -1.f, 1.f)));
	OutResult.PitchDegrees = PitchDegrees;
	if (PitchDegrees < Query.MinPitchDegrees || PitchDegrees > Query.MaxPitchDegrees)
	{
		OutResult.Status = WorldSnap::ETwoAnchorStatus::PitchOutOfRange;
		return false;
	}

	FVector Up = Query.UpHint.GetSafeNormal();
	if (Up.IsNearlyZero() || FMath::Abs(FVector::DotProduct(Up, Direction)) > 0.999f)
	{
		Up = FVector::RightVector;
		if (FMath::Abs(FVector::DotProduct(Up, Direction)) > 0.999f)
		{
			Up = FVector::ForwardVector;
		}
	}

	const FQuat Rotation = FRotationMatrix::MakeFromXZ(Direction, Up).ToQuat();
	const FVector LocalStart(Query.LocalMinX, 0.f, 0.f);
	const FVector Origin = Query.StartAnchor.Position - Rotation.RotateVector(LocalStart);
	OutResult.Transform = FTransform(Rotation, Origin);
	OutResult.SolvedStart = OutResult.Transform.TransformPosition(LocalStart);
	OutResult.SolvedEnd = OutResult.Transform.TransformPosition(FVector(Query.LocalMaxX, 0.f, 0.f));

	const float EndpointTolerance = Query.LengthToleranceCm + Query.MaxGapCm
		+ 2.f * Query.MaxEndPenetrationCm + 0.01f;
	if (!OutResult.SolvedStart.Equals(Query.StartAnchor.Position, EndpointTolerance)
		|| !OutResult.SolvedEnd.Equals(Query.EndAnchor.Position, EndpointTolerance))
	{
		OutResult = WorldSnap::FTwoAnchorResult{};
		OutResult.Status = WorldSnap::ETwoAnchorStatus::LengthMismatch;
		return false;
	}

	OutResult.bValid = true;
	OutResult.Status = WorldSnap::ETwoAnchorStatus::Resolved;
	return true;
}

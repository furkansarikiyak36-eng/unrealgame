// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace WorldSnap
{
	enum class EAnchorKind : uint8
	{
		Unknown,
		Ground,
		LogEnd,
		LogSide,
		LogTop,
		PostTop,
		BeamEnd,
		PlatformEdge
	};

	enum class ETwoAnchorStatus : uint8
	{
		NotRun,
		Resolved,
		InvalidQuery,
		InvalidAnchor,
		AnchorUnavailable,
		NonLoadBearing,
		SameAnchor,
		LengthMismatch,
		PitchOutOfRange,
		OccupiedAnchor,
		IncompatibleAnchor,
		InvalidSupport
	};

	/** Endpoint target. Position is the desired endpoint-centre position, not the raw surface hit. */
	struct FAnchor
	{
		FName Id = NAME_None;
		EAnchorKind Kind = EAnchorKind::Unknown;
		FVector Position = FVector::ZeroVector;
		FVector Normal = FVector::UpVector;
		bool bAvailable = true;
		bool bLoadBearing = true;
		bool bOccupied = false;
		bool bTargetCompatible = true;
		bool bSupportValid = true;
		float MaxSupportedLoad = TNumericLimits<float>::Max();
	};

	/** Geometry contract for a rigid piece whose local long axis is X. */
	struct FTwoAnchorQuery
	{
		FAnchor StartAnchor;
		FAnchor EndAnchor;
		float PieceLengthCm = 0.f;
		float LocalMinX = 0.f;
		float LocalMaxX = 0.f;
		float LengthToleranceCm = 2.f;
		/** Extra compression allowed across both ends, e.g. logs slightly seated in sockets. */
		float MaxEndPenetrationCm = 0.f;
		/** Extra free span allowed beyond LengthToleranceCm. */
		float MaxGapCm = 0.f;
		float RequiredLoad = 0.f;
		float MinPitchDegrees = -80.f;
		float MaxPitchDegrees = 80.f;
		FVector UpHint = FVector::UpVector;
		bool bRequireLoadBearing = true;
		bool bRejectOccupied = true;
		bool bRequireDistinctAnchors = true;
	};

	struct FTwoAnchorResult
	{
		bool bValid = false;
		ETwoAnchorStatus Status = ETwoAnchorStatus::NotRun;
		FTransform Transform = FTransform::Identity;
		FVector SolvedStart = FVector::ZeroVector;
		FVector SolvedEnd = FVector::ZeroVector;
		float AnchorDistanceCm = 0.f;
		float LengthErrorCm = 0.f;
		float SignedLengthErrorCm = 0.f;
		float YawDegrees = 0.f;
		float PitchDegrees = 0.f;
	};
}

/** Deterministic transform solver for a rigid piece constrained by both endpoints. */
class WORLDSNAP_API FWorldSnapTwoAnchorSolver
{
public:
	static bool Solve(const WorldSnap::FTwoAnchorQuery& Query, WorldSnap::FTwoAnchorResult& OutResult);
};

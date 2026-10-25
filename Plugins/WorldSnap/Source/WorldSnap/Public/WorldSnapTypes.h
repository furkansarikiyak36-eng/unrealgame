// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Generic snap data. This module intentionally does not depend on WorldScanSystem
 * or any game-specific actor class; adapters translate scan observations into this contract.
 */
namespace WorldSnap
{
	enum class EResolveStatus : uint8
	{
		NotRun,
		Resolved,
		NoCandidates,
		InvalidQuery,
		NoCompatiblePoint
	};

	struct FPoint
	{
		FName Id = NAME_None;
		FName Group = NAME_None;
		FVector Position = FVector::ZeroVector;
		FVector Normal = FVector::UpVector;
		FVector Axis = FVector::ForwardVector;

		/** Optional caller-owned index used to map the chosen point back to its source representation. */
		int32 SourceIndex = INDEX_NONE;

		/** Caller reports whether this candidate can currently receive a piece. */
		bool bAvailable = true;
		/** Occupied sockets are not snap targets even when geometrically closest. */
		bool bOccupied = false;
		/** Whether the candidate is permitted to bear structural load. */
		bool bLoadBearing = true;
		/** Optional semantic target label, e.g. Ground, PostTop, LogEnd. */
		FName SemanticType = NAME_None;
	};

	struct FQuery
	{
		FVector HitPoint = FVector::ZeroVector;
		FVector HitNormal = FVector::UpVector;

		/** A non-positive value disables the radial limit. Distances are in Unreal centimeters. */
		float MaxDistanceCm = 125.f;

		/** If set, only candidates in RequiredGroup are eligible. NAME_None means no group filter. */
		FName RequiredGroup = NAME_None;

		bool bRequireNormalCompatibility = true;
		float NormalAlignmentThreshold = 0.25f;

		/** Optional axis-aware selection, useful for repeated module points along a log. */
		bool bUseAxisConstraint = false;
		float MaxAlongAxisErrorCm = 45.f;
		float AlongAxisWeight = 1.f;
		float AcrossAxisWeight = 0.02f;
		float NormalPenalty = 10000.f;

		bool bRejectUnavailable = true;
		bool bRejectOccupied = true;
		bool bRequireLoadBearing = false;
	};

	struct FResolveResult
	{
		bool bValid = false;
		EResolveStatus Status = EResolveStatus::NotRun;
		int32 CandidateIndex = INDEX_NONE;
		FPoint Point;
		float DistanceCm = 0.f;
		float NormalDot = 1.f;
		float AlongAxisErrorCm = 0.f;
		float Score = TNumericLimits<float>::Max();
	};
}

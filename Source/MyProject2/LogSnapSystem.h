// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class ACarvableLog;

/** Deterministic snap-point generation and selection. Raycasts acquire a target; this module selects a predefined point. */
namespace LogSnapSystem
{
	enum class EFace : uint8 { Top, Bottom, Front, Back, EndA, EndB };

	struct FPoint
	{
		FName Id = NAME_None;
		EFace Face = EFace::Top;
		FVector Position = FVector::ZeroVector;
		FVector Normal = FVector::UpVector;
		FVector Axis = FVector::ForwardVector;
		float AlongAxis = 0.f;
		int32 ModuleIndex = INDEX_NONE;
		bool bEnd = false;
	};

	struct FQuery
	{
		FVector HitPoint = FVector::ZeroVector;
		FVector HitNormal = FVector::UpVector;
		float MaxAlongAxisError = 45.f;
		float NormalAlignmentThreshold = 0.25f;
	};

	MYPROJECT2_API bool BuildPoints(const ACarvableLog* Log, TArray<FPoint>& OutPoints);
	MYPROJECT2_API bool Resolve(const TArray<FPoint>& Points, const FQuery& Query, FPoint& OutPoint);
	MYPROJECT2_API float FoldAxisYawDelta(float DeltaDegrees);
	MYPROJECT2_API float NearestSector(float DeltaDegrees);
}

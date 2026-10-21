// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LogSnapSystem.h"
#include "WorldScanSystem.h"
#include "WorldSnapResolver.h"

class ACarvableLog;

/**
 * Adapter between the world-scan result contract and the reusable WorldSnap plugin.
 * Game/actor-specific point generation stays here; the plugin remains actor-agnostic.
 */
namespace WorldSnapScanBridge
{
	struct FLogSnapSettings
	{
		float MaxDistanceCm = 125.f;
		float MaxAlongAxisErrorCm = 45.f;
		float NormalAlignmentThreshold = 0.25f;
		FName RequiredFaceGroup = NAME_None;
	};

	struct FResolvedLogSnap
	{
		bool bValid = false;
		FHitResult SourceHit;
		TWeakObjectPtr<ACarvableLog> TargetLog;
		WorldScanSystem::FWorldAnalysis TargetAnalysis;
		LogSnapSystem::FPoint LogPoint;
		WorldSnap::FResolveResult ResolverResult;
	};

	/** Resolves a log snap only from an executed scanner raycast bundle; it does not commit placement. */
	MYPROJECT2_API bool ResolveLogSnap(const WorldScanSystem::FWorldScanBundle& Scan,
		const FLogSnapSettings& Settings, FResolvedLogSnap& OutResult);
}

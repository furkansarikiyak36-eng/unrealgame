// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WorldSnapTypes.h"

/** Deterministic, data-only snap candidate filtering and scoring. */
class WORLDSNAP_API FWorldSnapResolver
{
public:
	/**
	 * Select the best compatible candidate.
	 * Equal scores preserve input order, so callers must generate candidates in stable order.
	 * This function chooses a point only; gameplay placement/support rules remain outside the plugin.
	 */
	static bool Resolve(const TArray<WorldSnap::FPoint>& Candidates,
		const WorldSnap::FQuery& Query, WorldSnap::FResolveResult& OutResult);
};

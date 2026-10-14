// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AStoneActor;
class AStickActor;
class UWorld;

/**
 * Physical stones and sticks lying in the world, as a broken rock or a felled tree leaves them (Sons of the Forest: the
 * resource is a thing you pick up, not a number). A small stone goes into the pack; a building stone is carried in both
 * hands and placed; a raw stick goes into the pack.
 */
namespace LooseResources
{
	/** A small stone (pocketable, for fire rings) or a building stone (heavy, carried and placed). */
	MYPROJECT2_API AStoneActor* SpawnStone(UWorld* World, const FVector& Location, bool bBuildingStone);
	/** A raw stick, about half a metre long. */
	MYPROJECT2_API AStickActor* SpawnStick(UWorld* World, const FVector& Location, const FRotator& Rotation);
	/** A stone light enough for one hand goes into the pack rather than into the hands. */
	MYPROJECT2_API bool IsPocketStone(const AStoneActor* Stone);
}

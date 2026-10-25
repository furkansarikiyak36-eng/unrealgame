// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AStoneActor;
class UStaticMesh;
enum class EStoneShape : uint8;
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
	/**
	 * A carryable stone made from a placed stone mesh (a Megascans rock, a wall stone, a cobblestone): same mesh and size,
	 * physical, picked up with [E] and placed face to face with the stone placement. Mass follows the size (3-75 kg:
	 * carried in both hands, never pocketed).
	 */
	MYPROJECT2_API AStoneActor* SpawnStoneFromMesh(UWorld* World, UStaticMesh* Mesh, const FTransform& Transform, EStoneShape Shape);
	/** A raw stick, about half a metre long. */
	MYPROJECT2_API AStickActor* SpawnStick(UWorld* World, const FVector& Location, const FRotator& Rotation);
	/** A stone light enough for one hand goes into the pack rather than into the hands. */
	MYPROJECT2_API bool IsPocketStone(const AStoneActor* Stone);
}

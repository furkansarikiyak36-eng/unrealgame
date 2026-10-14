// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SimCore/SimulationTypes.h"

class ACropFieldActor;
class AAnimalActor;
class AForestCharacter;
class UAnimalDefinition;
class UPlantDefinition;
class UWorld;

/**
 * Farming on top of the simulation's crops and animals (PhaseT): a field is sown with seeds from the pack and grows with
 * the days, its grain goes into the pack; animals give eggs and wool, and meat and hide when slaughtered. Food fills
 * hunger, warm clothes made of hide keep the cold off.
 */
namespace FarmKit
{
	/** Seeds sown per field, and the most grain a ripe field gives. */
	constexpr int32 SeedsPerField = 4;
	constexpr int32 MaxGrainPerField = 12;
	/** Hides for one piece of hide clothing, how much one piece insulates, and the most clothes can. */
	constexpr int32 HidesPerGarment = 3;
	constexpr float InsulationPerGarment = 0.4f;
	constexpr float MaxInsulation = 0.8f;

	/** A quick greybox grain: ripe in about six days of the simulation's weather. */
	MYPROJECT2_API UPlantDefinition* WildGrain(UObject* Outer);
	MYPROJECT2_API UAnimalDefinition* AnimalOf(UObject* Outer, EAnimalSpecies Species);

	/** A bare field, two metres square, on the ground at Ground. */
	MYPROJECT2_API ACropFieldActor* MakeField(UWorld* World, const FVector& Ground);
	/** A patch of wild grain, already ripe: where the first seeds come from. */
	MYPROJECT2_API ACropFieldActor* MakeWildPatch(UWorld* World, const FVector& Ground);
	MYPROJECT2_API bool IsRipe(const ACropFieldActor* Field);
	MYPROJECT2_API bool IsSown(const ACropFieldActor* Field);
	/** Sows SeedsPerField seeds from the player's pack. */
	MYPROJECT2_API bool Sow(ACropFieldActor* Field, AForestCharacter* Player);
	/** A ripe field's grain into the pack, with some seed back; the field is bare again. Returns the grain. */
	MYPROJECT2_API int32 Harvest(ACropFieldActor* Field, AForestCharacter* Player);

	/** A chicken or a sheep standing at Location, grown up. */
	MYPROJECT2_API AAnimalActor* SpawnAnimal(UWorld* World, const FVector& Location, EAnimalSpecies Species);
	/** Eggs, milk and wool the animal has made, into the pack. */
	MYPROJECT2_API int32 CollectProducts(AAnimalActor* Animal, AForestCharacter* Player);
	/** Killed with the axe: meat, hide (and wool from a sheep) into the pack; the animal is gone. */
	MYPROJECT2_API bool Butcher(AAnimalActor* Animal, AForestCharacter* Player);

	/** Eats the most filling thing in the pack (meat cooked by a fire nearby fills more). Returns what was eaten. */
	MYPROJECT2_API FString Eat(AForestCharacter* Player);
	/** Makes one piece of hide clothing if there are hides and room for more warmth. */
	MYPROJECT2_API bool MakeHideGarment(AForestCharacter* Player);
}

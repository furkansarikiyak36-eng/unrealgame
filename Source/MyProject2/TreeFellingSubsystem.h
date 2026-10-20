// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TreeFellingSubsystem.generated.h"

class AHarvestableActor;

/**
 * Makes level trees fellable in a game world.
 *
 * On begin play every SkeletalMeshActor (Megaplant) and StaticMeshActor (e.g. KiteDemo) tagged "Fellable" becomes an
 * AHarvestableActor tree in place (same mesh, a trunk capsule takes the hits); the source actor is hidden. An axe hit
 * is only heard; when the tree gives way a crack is heard, a stump stays, the tree topples and, when it lands, raw
 * UnrealExport logs (600 / 300 cm by tree height) lie along the fall line. Logs split 600 -> 300 -> 150 -> 75.
 *
 * Rocks tagged "Breakable" become stones of the stone chain (AHarvestableActor::InitFromStaticRock).
 *
 * Pawns that are not the project's own character (it has its own axe) swing with the left mouse button: a short sweep
 * in front of the pawn hits the nearest tree, log or rock (axe or pickaxe, as the target needs). The right mouse button
 * strikes a chisel on the stone in front (carving, AHarvestableActor::CarveHit).
 */
UCLASS()
class MYPROJECT2_API UTreeFellingSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static constexpr const TCHAR* FellableTag = TEXT("Fellable");
	/** Same damage as the character's axe: a tree under 15 m takes 6 swings, a taller one 8. */
	static constexpr float ChopDamage = 20.f;
	static constexpr float ChopReachCm = 220.f;
	static constexpr float ChopCooldownSeconds = 0.6f;

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UTreeFellingSubsystem, STATGROUP_Tickables); }

	/** Converts the tagged skeletal trees of the world; returns how many. Safe to call twice. */
	UFUNCTION(BlueprintCallable, Category = "Felling")
	int32 ConvertFellableTrees();

	/** One axe swing from Chopper towards its facing; true when something was hit. Also used by tests. */
	UFUNCTION(BlueprintCallable, Category = "Felling")
	bool ChopFrom(AActor* Chopper);

	/** One chisel strike from Chopper on the stone in front of it; true when a carvable stone was hit. Right mouse. */
	UFUNCTION(BlueprintCallable, Category = "Felling")
	bool CarveFrom(AActor* Chopper);

	static constexpr const TCHAR* BreakableTag = TEXT("Breakable");

	/** Trees converted in this world (alive or felled). */
	UFUNCTION(BlueprintCallable, Category = "Felling")
	int32 GetConvertedCount() const { return Converted; }

private:
	int32 Converted = 0;
	float Cooldown = 0.f;
};

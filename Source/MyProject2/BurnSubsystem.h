// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BurnSubsystem.generated.h"

class ACarvableLog;

/**
 * Drives the fire of a world: every burning piece of wood burns down, heats the wood beside it (the closer, the faster; touching
 * pieces catch in about the time a lighter takes) and lights a fire pit it lies in. Warming pieces cool off when the heat stops.
 */
UCLASS()
class MYPROJECT2_API UBurnSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UBurnSubsystem* Get(UWorld* World);

	/** Wood within this gap (cm, surface to surface) of a burning piece is heated; the heat falls off linearly to nothing here. */
	static constexpr float SpreadReachCm = 50.f;
	/** Wood this close (gap, cm) to burning wood catches as quickly as a lighter would make it. */
	static constexpr float TouchGapCm = 10.f;

	void Light(ACarvableLog* Piece);
	void Warm(ACarvableLog* Piece);
	int32 GetBurningCount() const;

	/** Burns the world for Seconds (in steps of a quarter second). The tick does this; the mechanics test calls it to skip time. */
	void Advance(float Seconds);

	virtual void Tick(float DeltaSeconds) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UBurnSubsystem, STATGROUP_Tickables); }

private:
	void Step(float Seconds);

	TArray<TWeakObjectPtr<ACarvableLog>> Burning;
	TArray<TWeakObjectPtr<ACarvableLog>> Warming;
};

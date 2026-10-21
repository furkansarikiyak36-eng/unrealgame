// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StickStructure.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;

/**
 * One sharpened stick driven into the ground (Sons of the Forest's physical stick building: nothing is placed as a
 * finished prefab, every stick is put in by hand). What sticks make follows from how they stand:
 * - a stake hurts whoever walks into its point;
 * - stakes closer than a body's width together block the way, a palisade, and raiders have to break through;
 * - a stake set alight with the lighter is a torch: light and some warmth until it burns down.
 * It has a strength like wood (0 sound, 1 broken).
 */
UCLASS()
class MYPROJECT2_API AStickStructure : public AActor
{
	GENERATED_BODY()

public:
	AStickStructure();

	static constexpr float HeightCm = 140.f;
	static constexpr float RadiusCm = 3.5f;
	/** Enemies this close (cm, to the stake) are hurt this much per second. */
	static constexpr float SpikeReachCm = 45.f;
	static constexpr float SpikeDps = 20.f;
	/** A torch burns this long, warms this far, this much. */
	static constexpr float TorchSeconds = 360.f;
	static constexpr float TorchReachCm = 300.f;
	static constexpr float TorchWarmthC = 12.f;
	/** Seconds of the lighter's flame it takes to set a stake alight. */
	static constexpr float IgniteSeconds = 2.f;

	/** A stake driven into the ground at Ground, standing up. */
	static AStickStructure* Plant(UWorld* World, const FVector& Ground, float Yaw);

	float GetDamage() const { return Damage; }
	/** Wears it down; it breaks (is destroyed) at 1. */
	void AddDamage(float Amount);

	/** The lighter's flame: after IgniteSeconds the stake burns as a torch. True once it burns. */
	bool ApplyHeat(float Seconds);
	bool IsLitTorch() const { return bLit; }
	float GetBurnSecondsLeft() const { return BurnLeft; }

	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Shaft;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Point;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Flame;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPointLightComponent> Light;

	float Damage = 0.f;
	float Heat = 0.f;
	bool bLit = false;
	float BurnLeft = 0.f;
};

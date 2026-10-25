// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Campfire.generated.h"

class ACarvableLog;
class UPointLightComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * A campfire. Any piece of wood is fuel: it burns as long as the wood it holds (a whole log 10 minutes, a half 5, a
 * quarter 2.5), so cutting a log up into firewood is just cutting it up. The fire holds at most 15 minutes of fuel.
 * Wood standing in its heat is scorched (see ACarvableLog::AddDamage) and joints of burnt pieces let go.
 */
UCLASS()
class MYPROJECT2_API ACampfire : public AActor
{
	GENERATED_BODY()

public:
	ACampfire();

	/** How long a whole log (390 x 70 cm) burns, and the most a fire holds. */
	static constexpr float SecondsPerWholeLog = 600.f;
	static constexpr float MaxFuelSeconds = 900.f;
	/** Wood within this reach (from the fire, to the surface of a log) is scorched, this much of its damage per second. */
	static constexpr float HeatReachCm = 160.f;
	static constexpr float HeatDamagePerSecond = 0.02f;

	/** Seconds Piece would burn. */
	static float FuelSecondsOf(const ACarvableLog* Piece);

	/** A fire pit with a ring of stones on the ground at Ground, not lit until it has fuel. */
	static ACampfire* Build(UWorld* World, const FVector& Ground);
	/** Creates only the fire logic; the caller's hand-placed stones remain as the visible hearth ring. */
	static ACampfire* BuildWithExternalStoneRing(UWorld* World, const FVector& Ground);

	/**
	 * Throws Piece on the fire: it is used up (a burning piece lights the fire). False if the fire is already full or the piece is part of a
	 * structure. The caller takes the piece off the shoulder first.
	 */
	bool AddFuel(ACarvableLog* Piece);

	/** Fuel alone does not make a fire: it has to be lit (a lighter, or burning wood). */
	bool IsBurning() const { return bLit && FuelSeconds > 0.f; }
	/** Lights the fire if it has fuel. */
	bool Ignite();
	/** Heat from a lighter: the fire catches after a couple of seconds. */
	bool ApplyHeat(float Seconds);
	float GetFuelSeconds() const { return FuelSeconds; }

	/** Burns for Seconds: fuel goes down, the light follows, wood in reach is scorched. Called every tick as well. */
	void Burn(float Seconds);

	virtual void Tick(float DeltaSeconds) override;

private:
	void SetLit(bool bLit);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPointLightComponent> Light;

	UPROPERTY(VisibleAnywhere)
	TArray<TObjectPtr<UStaticMeshComponent>> Stones;

	UPROPERTY(VisibleAnywhere)
	TArray<TObjectPtr<UStaticMeshComponent>> Flames;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Embers;

	float FuelSeconds = 0.f;
	float HeatSeconds = 0.f;
	bool bLit = false;
	float FlickerTime = 0.f;
};

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldFeatures.generated.h"

class UStaticMeshComponent;

/**
 * A river: wading through it is slow and the water is cold (Death Stranding: the world is hard to cross until you build
 * across it). Logs laid over it as a bridge keep the player dry. Its water can be drunk.
 */
UCLASS()
class MYPROJECT2_API ARiverStrip : public AActor
{
	GENERATED_BODY()

public:
	ARiverStrip();

	/** A strip of water HalfSize (x, y) around Center, its surface at the ground's height. */
	static ARiverStrip* Spawn(UWorld* World, const FVector& Center, const FVector2D& HalfSize);

	/** Standing in the water: inside the strip with the feet at most a little above its surface. */
	bool Contains(const FVector& Feet) const;
	static bool IsInWater(const UWorld* World, const FVector& Feet);

	/** How much colder wet clothes make it, and how slow wading is. */
	static constexpr float WetColdC = 8.f;
	static constexpr float WadeSpeedFactor = 0.45f;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Water;

	FVector2D Half = FVector2D(100.f, 100.f);
};

/**
 * A rope strung between two points: ride along it to the other end (up a cliff, over a river). Each rope is a ropeway
 * in the shared route graph (URegionLogisticsSubsystem), so the world gets easier to cross the more of them there are.
 */
UCLASS()
class MYPROJECT2_API ARopeLine : public AActor
{
	GENERATED_BODY()

public:
	ARopeLine();

	static constexpr float MinLengthCm = 200.f;
	static constexpr float MaxLengthCm = 3000.f;
	static constexpr float ReachCm = 200.f;
	static constexpr float RideSpeed = 600.f;

	/** A rope from A to B; null if it would be too short or too long. */
	static ARopeLine* Span(UWorld* World, const FVector& A, const FVector& B);

	FVector GetA() const { return EndA; }
	FVector GetB() const { return EndB; }
	FName GetLinkKey() const { return LinkKey; }
	FName GetNodeKey(bool bEndB) const;
	/** The end within reach of Where (true: B), if there is one. */
	bool EndNear(const FVector& Where, bool& bOutEndB) const;

	static int32 CountAll(const UWorld* World);

	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Line;

	FVector EndA = FVector::ZeroVector;
	FVector EndB = FVector::ZeroVector;
	FName LinkKey;
};

/**
 * A remnant of the people who were here before: a note on a stake, read with E (Subnautica: the story is told by what is
 * left in the world, never by a cutscene).
 */
UCLASS()
class MYPROJECT2_API ARemnantNote : public AActor
{
	GENERATED_BODY()

public:
	ARemnantNote();

	static ARemnantNote* Place(UWorld* World, const FVector& Ground, const FString& Title, const FString& Text);

	FString Title;
	FString Text;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Stake;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Paper;
};

/** A block of rock rising out of the ground: too high to climb without a rope. */
UCLASS()
class MYPROJECT2_API ACliffBlock : public AActor
{
	GENERATED_BODY()

public:
	ACliffBlock();

	static ACliffBlock* Spawn(UWorld* World, const FVector& GroundCenter, const FVector& Size);
	float GetTopZ() const { return TopZ; }

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Rock;

	float TopZ = 0.f;
};

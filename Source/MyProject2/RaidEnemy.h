// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RaidEnemy.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;
class UStructureDamageResolver;

/**
 * A raider. It walks to the player, or to the nearest installed wood when it has no one in reach, hits what is in its way
 * (a wall is damaged like by a fire: past 60% the joints let go) and is hurt by pointed stakes it walks into and by
 * logs that strike it. A greybox figure: a red post.
 */
UCLASS()
class MYPROJECT2_API ARaidEnemy : public AActor
{
	GENERATED_BODY()

public:
	ARaidEnemy();

	float Health = 60.f;
	float MoveSpeed = 250.f;
	/** Damage per second to wood (0..1 of a piece's strength) and to the player. */
	float WoodDps = 0.05f;
	float PlayerDps = 8.f;
	/** Wood the enemy can hit from this far (cm) from its body. */
	static constexpr float AttackReachCm = 110.f;
	/** A pointed post this close hurts the enemy this much per second. */
	static constexpr float StakeReachCm = 55.f;
	static constexpr float StakeDps = 25.f;

	/** What the enemy goes for instead of looking for a target. */
	void SetTarget(AActor* Target) { TargetOverride = Target; }

	/** Thinks and moves for Dt seconds. The tick does this; the mechanics test calls it to skip time. */
	void StepAI(float Dt);

	/** Hurts the enemy; destroyed at zero. */
	void TakeHit(float Amount);
	bool IsAlive() const { return Health > 0.f; }

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCapsuleComponent> Capsule;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Body;

	TWeakObjectPtr<AActor> TargetOverride;
	UPROPERTY(Transient)
	TObjectPtr<UStructureDamageResolver> DamageResolver;

	TArray<FVector> CachedNavPath;
	int32 CachedNavWaypoint = 0;
	TWeakObjectPtr<AActor> CachedNavTarget;
	FVector CachedNavGoal = FVector::ZeroVector;
	double NextPathRefreshTime = 0.0;
};

/** Starts and stops a raid with the plan this game ships with (a night with one wave of raiders). */
struct MYPROJECT2_API RaidKit
{
	/** Builds the plan in code and starts it for the player in the world. False if it could not start. */
	static bool StartDefaultRaid(UWorld* World);
	/** Ends the raid and removes every raider. */
	static void StopRaid(UWorld* World);
	static int32 CountEnemies(UWorld* World);
};

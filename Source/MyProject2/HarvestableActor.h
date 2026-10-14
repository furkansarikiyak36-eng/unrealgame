// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ForestTypes.h"
#include "HarvestableActor.generated.h"

class UStaticMeshComponent;

/**
 * A tree, rock or big log lying on the ground that the player can hit.
 * Trees topple away from the player and leave a stage 1 log; cutting a big log
 * splits it in two (stage 1 -> two stage 2 logs -> four carryable logs); rocks
 * break apart into stones. The fall is animated instead of simulated so a 100k+
 * triangle tree never needs complex physics.
 */
UCLASS()
class MYPROJECT2_API AHarvestableActor : public AActor
{
	GENERATED_BODY()

public:
	AHarvestableActor();

	/** Copies mesh and materials from a level placed actor and sizes health and drops from it. */
	void InitFrom(const UStaticMeshComponent* Source, EHarvestType InType);

	/** Lays a big log of the given stage (1 or 2) on the ground, centred at Center along Direction. */
	void InitLog(int32 InStage, const FVector& Center, const FVector& Direction);

	/**
	 * Whole logs (six modules, 390 cm) a tree of this height gives: 85% of the height is usable trunk, at most four.
	 * A trunk or a half trunk carries the number it still holds until it is cut up.
	 */
	static int32 LogsFromHeight(float TreeHeightCm);

	/** A broken rock: one building stone and Count - 1 small stones, lying around Center. */
	static void DropRockResources(UWorld* World, const FVector& Center, int32 Count);
	/** A felled tree's branches: raw sticks along the fall line from Base. */
	static void DropBranchSticks(UWorld* World, const FVector& Base, const FVector& Direction, float Length);
	static constexpr int32 SticksPerTree = 3;
	int32 GetLogCount() const { return LogCount; }
	float GetTreeHeight() const { return Height; }
	void SetLogCount(int32 Count) { LogCount = Count; }

	/** Returns true when this hit felled the tree, broke the rock or split the log. */
	bool ApplyHit(float Damage, const FHitResult& Hit, const FVector& HitDirection);

	virtual void Tick(float DeltaSeconds) override;

	bool IsAlive() const { return Health > 0.f; }
	bool IsTree() const { return HarvestType == EHarvestType::Tree; }
	bool IsTrunk() const { return HarvestType == EHarvestType::Trunk; }
	/** Trees and big logs take the axe, rocks the pickaxe. */
	bool IsWood() const { return HarvestType != EHarvestType::Rock; }
	float GetHealthPercent() const { return MaxHealth > 0.f ? Health / MaxHealth : 0.f; }
	/** Inventory item a big log becomes when picked up. */
	EForestItem GetLogItem() const { return LogStage == 1 ? EForestItem::TrunkLog : EForestItem::BigLog; }
	FString GetDisplayName() const;

	/** Dragging a big log: Grab picks the end nearest Holder as the lifted end. */
	void BeginDrag(const FVector& Holder);
	/** Lifted end follows Front, the other end trails on the ground. */
	void DragTo(const FVector& Front);
	/** Lays the log back on the ground where it is. */
	void EndDrag();
	bool IsDragged() const { return bDragged; }

	/** Shouldering a big log: attached to the player's view, no collision. */
	void BeginShoulder(USceneComponent* AttachTo, const FTransform& Relative, bool bCastShadow = false);
	/** Off the shoulder, laid on the ground centred at Center along Direction. */
	void PlaceOnGround(const FVector& Center, const FVector& Direction);

private:
	void StartFall(const FVector& Direction);
	void FinishFall();
	void BreakRock();
	void SplitLog();
	void SpawnDebris(const FVector& Location, const FVector& Normal, int32 Num);
	void AddHitMark(const FHitResult& Hit);

	/** Places a log so its centre sits on the ground below Center. */
	static FTransform GroundedLogTransform(UWorld* World, const UStaticMesh* LogMesh, const FVector& Scale, const FVector& Center, const FVector& Direction, const AActor* Ignore);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, Category = "Harvest")
	EHarvestType HarvestType = EHarvestType::Tree;

	/** For big logs: 1 = trunk straight from the tree, 2 = its halves. */
	UPROPERTY(VisibleInstanceOnly, Category = "Harvest")
	int32 LogStage = 0;

	UPROPERTY(EditAnywhere, Category = "Harvest")
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, Category = "Harvest")
	int32 DropCount = 3;

	UPROPERTY(EditAnywhere, Category = "Harvest")
	float FallDuration = 2.4f;

	/** Cap on axe/pickaxe decals per tree, log or rock. */
	UPROPERTY(EditAnywhere, Category = "Harvest")
	int32 MaxHitMarks = 12;

	int32 HitMarkCount = 0;

	float Health = 100.f;
	float Height = 1000.f;
	/** Whole logs still inside this trunk (or half trunk). */
	int32 LogCount = 4;

	bool bFalling = false;
	float FallTime = 0.f;
	FVector FallDirection = FVector::ForwardVector;
	FVector FallAxis = FVector::RightVector;
	FQuat StartRotation = FQuat::Identity;
	FVector BaseLocation = FVector::ZeroVector;
	float ShakeTime = 0.f;

	bool bDragged = false;
	FVector DragBack = FVector::ZeroVector;
	float DragLength = 0.f;
	float DragRadius = 0.f;
};

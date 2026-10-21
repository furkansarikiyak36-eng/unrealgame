// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ForestTypes.h"
#include "HarvestableActor.generated.h"

class UStaticMeshComponent;
class USkeletalMeshComponent;
class UCapsuleComponent;

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

	/**
	 * A skeletal tree (Megaplant): the skeletal mesh is shown as is, a capsule round the trunk takes the hits,
	 * and the whole actor tips over from its base like a static tree.
	 */
	void InitFromSkeletal(const USkeletalMeshComponent* Source);

	/**
	 * A level-placed static tree (e.g. KiteDemo) tagged "Fellable": the mesh is shown without collision, a capsule round
	 * the trunk takes the hits. Same simple felling as the skeletal trees (see bSimpleTree).
	 */
	void InitFromStaticTree(const UStaticMeshComponent* Source);

	/**
	 * A raw UnrealExport log (SM_RawLog_<length>, along X, pivot at the centre) lying on the ground. Trees leave these;
	 * hitting one splits it into two of half the length: 600 -> 300 -> 150 -> 75. 75 cm is the end of the chain.
	 * Lengths are whole 75 cm modules (ACarvableLog::ModuleLength): 600 = 8M, 300 = 4M, 150 = 2M, 75 = 1M.
	 */
	void InitRawLog(int32 LengthCm, const FVector& Center, const FVector& Direction);
	/** SM_RawLog_<length>; for a length without its own mesh, the 150 cm one (InitRawLog then scales it). */
	static UStaticMesh* LoadRawLog(int32 LengthCm);
	static constexpr int32 ShortestRawLogCm = 75;
	/** The longest raw log that goes on the shoulder; longer ones are dragged by rope. */
	static constexpr int32 LongestShoulderLogCm = 300;
	int32 GetRawLogLength() const { return RawLogLength; }

	/**
	 * Picking up a raw log: it becomes the building piece every assembly uses (ACarvableLog, same mesh and place,
	 * cut, notched, laid, stood and carried by its volume). This actor is destroyed. Null if it is not a raw log.
	 */
	class ACarvableLog* ConvertToCarvable();

	/**
	 * A level-placed rock tagged "Breakable" (Megascans). Its size gives its stone tier: 3 large (100 cm or more),
	 * 2 medium (40 cm or more), 1 building stone. The pickaxe breaks large into three medium stones (50-75 cm) and
	 * medium into three building stones (30-40 cm, the smallest piece: wall and paving size), which scatter; building
	 * stones stay whole. See CarveHit for carving.
	 */
	void InitFromStaticRock(const UStaticMeshComponent* Source);

	/** A loose stone of Tier (1 small, 2 medium) thrown from where it is with Impulse; its physics stops after 4 s. */
	void InitLooseStone(int32 Tier, UStaticMesh* StoneMesh, float Scale, const FVector& Impulse);

	/**
	 * One chisel strike. After CarveHitsNeeded strikes a medium stone becomes a flat wall stone, a building stone a
	 * squared cobblestone (grey or brown) of its size, at least 30 cm. Returns true when it changed.
	 */
	bool CarveHit();
	bool IsCarvable() const { return HarvestType == EHarvestType::Rock && (StoneTier == 1 || StoneTier == 2); }
	static constexpr int32 CarveHitsNeeded = 3;

	/** Raw logs a felled tree leaves, longest first: 18 m or taller -> 600 + 300, 10 m or taller -> 600, else 300. */
	static TArray<int32> RawLogsForHeight(float TreeHeightCm);

	/** Plays /Game/Audio/Felling/<Name> at Location with distance falloff (silent if the asset is missing). */
	static void PlayFellingSound(UWorld* World, const TCHAR* Name, const FVector& Location, float Volume = 1.f);

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
	/** Trunk capsule for trees whose own mesh has no collision; sets TrunkRadius. */
	void SetupTrunkCollider();
	/** Breakable rocks: the pieces of the next tier fly apart from the rock's centre. */
	void BreakStone();

	/** Places a log so its centre sits on the ground below Center. */
	static FTransform GroundedLogTransform(UWorld* World, const UStaticMesh* LogMesh, const FVector& Scale, const FVector& Center, const FVector& Direction, const AActor* Ignore);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Skeletal trees only: the tree itself, no collision. */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USkeletalMeshComponent> TreeSkin;

	/** Skeletal trees only: trunk collision for the axe and the player. */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCapsuleComponent> TrunkCollider;

	UPROPERTY(EditAnywhere, Category = "Harvest")
	EHarvestType HarvestType = EHarvestType::Tree;

	/** For big logs: 1 = trunk straight from the tree, 2 = its halves. */
	UPROPERTY(VisibleInstanceOnly, Category = "Harvest")
	int32 LogStage = 0;

	/** Raw UnrealExport logs only: length in cm (600, 300, 150); 0 for the greybox logs. */
	UPROPERTY(VisibleInstanceOnly, Category = "Harvest")
	int32 RawLogLength = 0;

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

	/**
	 * Skeletal and level-placed static trees: an axe hit only plays a sound; when it gives way a crack is heard, the
	 * tree leans for a moment and falls and, when it lands, raw logs lie along the fall line (RawLogsForHeight).
	 */
	bool bSimpleTree = false;
	/** Trunk radius in world cm. */
	float TrunkRadius = 30.f;
	/** Simple trees: seconds the tree creaks and leans before it goes over. */
	float BreakDuration = 0.7f;
	/** Point the tree rotates around while falling: its foot. */
	FVector FallPivot = FVector::ZeroVector;

	/** Breakable rocks and their pieces: 3 large, 2 medium, 1 small, -1 carved; 0 = not part of the stone chain. */
	int32 StoneTier = 0;
	int32 CarveHits = 0;
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

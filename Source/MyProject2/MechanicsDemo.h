// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CarvableLog.h"
#include "MechanicsDemo.generated.h"

class AHarvestableActor;
class APickupItem;
class ACharacter;
class UArrowComponent;

/** One line of the guided demo checklist, as the HUD shows it. */
struct FDemoGoalLine
{
	FString Text;
	bool bDone = false;
	bool bCurrent = false;
};

/**
 * The wood chain: fell a tree, split the trunk into two big logs, split those into four carryable logs,
 * then carve a raw log with every carving tool.
 *
 * Guided (default): the player does every step; the HUD shows a checklist that ticks off what the player
 * has done, with a hint for the next step. Nothing is hit, moved or turned for the player.
 * Auto play (tests only, -MechanicsDemo / -MechanicsDemoQuit): the demo does each step itself and reports
 * PASS / FAIL to the screen and the log (prefix [MechanicsDemo]); -MechanicsDemoQuit exits when done.
 *
 * Start: placed in a level (Demo_WoodChain) it starts on play, and the raw log to carve lies where it stands,
 * along its arrow. Elsewhere: admin mode + L, or the console command MechanicsDemo.
 */
UCLASS()
class MYPROJECT2_API AMechanicsDemo : public AActor
{
	GENERATED_BODY()

public:
	AMechanicsDemo();

	/** Starts (or restarts) the demo in this world; bAuto plays it by itself, for tests. */
	static void Start(UWorld* World, bool bAuto, bool bQuitWhenDone = false);

	/** Guided mode is running: the HUD draws the checklist. */
	bool IsGuiding() const { return !bAutoPlay && bGuideActive; }
	/** Checklist lines and the hint for the current step. */
	void GetGuide(TArray<FDemoGoalLine>& OutLines, FString& OutHint) const;

	virtual void Tick(float DeltaSeconds) override;

	/** Placed in a level: start on play without a key or console command. */
	UPROPERTY(EditAnywhere, Category = "Demo")
	bool bAutoStart = true;

	/** Play every step by itself (automated test). Off: guided, the player does everything. */
	UPROPERTY(EditAnywhere, Category = "Demo")
	bool bAutoPlay = false;

	/** Seconds after play begins before the first step, so the player can look around. */
	UPROPERTY(EditAnywhere, Category = "Demo")
	float StartDelay = 2.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	enum class EStep : uint8
	{
		PickTree,
		Chop,
		WaitFall,
		SplitTrunk,
		WaitBigLogs,
		SplitBigLogs,
		WaitLogs,
		PlaceAcross,
		CheckAcross,
		CarryAndStand,
		Frame,
		Shaping,
		Script,
		SpawnCarvable,
		Carve,
		Done,
	};

	/** Resets every counter and runs the chain from the first step. */
	void Begin(float Delay);
	void Report(bool bPassed, const FString& Text);
	void Info(const FString& Text);
	void Finish();
	void Wait(float Seconds) { WaitTime = Seconds; }
	void LookAt(const FVector& Target, float Distance);
	/** One axe hit at the side of Target facing the player; true when the hit felled or split it. */
	bool Hit(AHarvestableActor* Target);
	void FindBigLogs();
	bool RunCarveStep(int32 Index);
	/** Checks the dashed cut guide for a cut before it is made: lines exist, lie on the bark, span this along the log. */
	void CheckGuide(const ACarvableLog* Log, ECarveTool Tool, float X, float Angle, float Width, float Depth, const TCHAR* Name);
	/** The raw log to carve: at the placed spot along the arrow, or in front of the player across the view. */
	ACarvableLog* SpawnCarvable(ACharacter* Player);

	/** Guided mode: watch the world for what the player has done. */
	enum EGoal : int32
	{
		GoalFell,
		GoalSplitTrunk,
		GoalSplitBigLogs,
		GoalNotch,
		GoalNotchRound,
		GoalNotchFlat,
		GoalCrossCut,
		GoalSplit,
		GoalPoint,
		GoalChamfer,
		GoalTrim,
		GoalDovetail,
		GoalPlaceAcross,
		GoalStand,
		GoalBrace,
		GoalPlatform,
		GoalDoor,
		GoalStakeWall,
		GoalFire,
		GoalRepair,
		GoalCount,
	};
	void TickGuided(float DeltaSeconds);
	/** Ticks off goals from what appeared in the world. Also runs under auto play, so tests cover the guide. */
	void TrackGoals(float DeltaSeconds);
	void CompleteGoal(int32 Goal);
	void HandleLogCarved(ACarvableLog* Log, ECarveTool Tool);
	void HandleLogPlacedAcross(ACarvableLog* Upper);
	void HandleLogStoodUp(ACarvableLog* Post);
	void HandleLogRepaired(ACarvableLog* Piece);
	FDelegateHandle RepairedHandle;
	FDelegateHandle PlacedHandle;
	FDelegateHandle StoodHandle;
	/** The tree's whole logs that came down in this run, not yet used by a step. */
	TArray<ACarvableLog*> FreshLogs() const;
	/** Auto play: the corner laid in PlaceAcross, checked for holding still in CheckAcross. */
	TWeakObjectPtr<ACarvableLog> CornerUpper;
	TWeakObjectPtr<ACarvableLog> CornerLower;
	FVector CornerUpperStart = FVector::ZeroVector;

	/** Auto play: the structure test (corner, posts and beam, taking a post out), stage by stage. */
	bool RunFrameStage(class AForestCharacter* Character);
	/** Auto play: end shaping (stake, chamfer, trim), dovetail and repeated splits, measured by the wood they remove. */
	void RunShaping(class AForestCharacter* Character);

	/**
	 * Scripted tests of the later mechanics (braces, platform, cabin...). Each step does its work and returns how many
	 * seconds to wait before the next one. Pieces are kept by name between steps.
	 */
	TArray<TFunction<float()>> Script;
	int32 ScriptIndex = 0;
	TMap<FName, TWeakObjectPtr<ACarvableLog>> Named;
	void BuildScript(class AForestCharacter* Character);
	void AddBraceScript(class AForestCharacter* Character);
	void AddPlatformScript(class AForestCharacter* Character);
	void AddCabinScript(class AForestCharacter* Character);
	void AddRecognitionScript(class AForestCharacter* Character);
	void AddFireScript(class AForestCharacter* Character);
	void AddDamageScript(class AForestCharacter* Character);
	void AddBurnScript(class AForestCharacter* Character);
	void AddPhaseScript(class AForestCharacter* Character);
	int32 ExpectedTreeLogs = 4;
	FVector PushStart = FVector::ZeroVector;
	FVector HaulStart = FVector::ZeroVector;
	FVector LoadStart = FVector::ZeroVector;
	void AddCollapseScript(class AForestCharacter* Character);
	ACarvableLog* Piece(FName Name) const;
	int32 FrameStage = 0;
	bool bRepeatFrameStage = false;
	TArray<TWeakObjectPtr<ACarvableLog>> FrameLogs;
	TWeakObjectPtr<ACarvableLog> FramePostA;
	TWeakObjectPtr<ACarvableLog> FramePostB;
	TWeakObjectPtr<ACarvableLog> FrameBeam;
	float FrameBeamZ = 0.f;

	bool bGuideActive = false;
	bool bGuideReady = false;
	/** ExistingBefore is filled: from here on new trunks, big logs and logs count toward the goals. */
	bool bTracking = false;
	bool GoalDone[GoalCount] = {};
	float PollTime = 0.f;
	TSet<TWeakObjectPtr<AActor>> SeenTrunks;
	TSet<TWeakObjectPtr<AActor>> SeenBigLogs;
	TSet<TWeakObjectPtr<AActor>> SeenLogs;
	FDelegateHandle CarvedHandle;

	EStep Step = EStep::PickTree;
	float WaitTime = 0.5f;
	float StepTime = 0.f;
	int32 Hits = 0;
	int32 Passed = 0;
	int32 Failed = 0;
	int32 CarveIndex = 0;
	bool bQuit = false;
	FVector TreeBase = FVector::ZeroVector;
	FVector ChopDirection = FVector::ForwardVector;

	TWeakObjectPtr<AHarvestableActor> Tree;
	TWeakObjectPtr<AHarvestableActor> Trunk;
	TArray<TWeakObjectPtr<AHarvestableActor>> BigLogs;
	TSet<TWeakObjectPtr<AActor>> ExistingBefore;
	TWeakObjectPtr<ACarvableLog> Carvable;

	/** Carving spot and log direction when placed in a level. */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UArrowComponent> CarveSpot;

	/** Loaded with the level (not spawned by a key or command): carve at CarveSpot, stay for another run. */
	bool bPlaced = false;
};

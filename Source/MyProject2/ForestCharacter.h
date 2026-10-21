// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "ForestTypes.h"
#include "CarvableLog.h"
#include "ForestCharacter.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
class UInputAction;
class UInputMappingContext;
class UMaterialInstanceDynamic;
class AHarvestableActor;
class APickupItem;
class AStructureManager;
class ALogWallBlueprint;
class ALogConstructionActor;
class AStoneMasonryActor;
class ACampfire;
class AHaulCart;
class AFireStone;
class AStickStructure;
class ARopeLine;
class ARemnantNote;
class ARawMaterialNode;
class UInstancedStaticMeshComponent;
class USurvivalComponent;
class UChunkManager;
class UCableComponent;
class ULineBatchComponent;
class UStoneCarryComponent;
class UStonePlacementComponent;
class UInventoryComponent;
class AStoneActor;

/**
 * First person survival character: chop trees, break rocks, pick up drops,
 * craft simple materials and place stone blocks. Input is built in code so the
 * prototype needs no input assets.
 */
UCLASS()
class MYPROJECT2_API AForestCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AForestCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	int32 GetItemCount(EForestItem Item) const;
	/** Canonical stack inventory shared with storage, crafting and persistence. */
	UInventoryComponent* GetInventoryComponent() const { return CoreInventory; }
	/** Drop stackable inventory resources into the world; stock is consumed only after spawn succeeds. */
	UFUNCTION(BlueprintCallable, Category = "Inventory") AActor* DropInventoryItem(EForestItem Item, int32 Count = 1);

	/** Backpack (GDD 6.3): 24 slots, stacks of 20. */
	static constexpr int32 InventorySlots = 24;
	static constexpr int32 StackSize = 20;
	/** The backpack as slots, in display order: one entry per stack. */
	void GetInventorySlots(TArray<TPair<EForestItem, int32>>& OutSlots) const;
	int32 GetUsedSlots() const;
	bool IsInventoryOpen() const { return bInventoryOpen; }
	bool IsAdminMode() const { return bAdminMode; }
	EForestTool GetCurrentTool() const { return CurrentTool; }
	const USurvivalComponent* GetSurvival() const { return Survival; }
	/** For the mechanics test: the survival values, writable. */
	USurvivalComponent* GetSurvivalForTest() { return Survival; }
	/** The log in the player's hands (None when empty; TrunkLog while a trunk is dragged by rope). */
	EForestItem GetCarriedItem() const { return CarriedItem; }
	int32 GetCarriedCount() const { return CarriedItem == EForestItem::Log ? CarriedLogs.Num() : (CarriedItem != EForestItem::None ? 1 : 0); }
	AHarvestableActor* GetFocusedHarvestable() const { return FocusedHarvestable.Get(); }
	APickupItem* GetFocusedPickup() const { return FocusedPickup.Get(); }
	ARawMaterialNode* GetFocusedRawNode() const { return FocusedRawNode.Get(); }
	bool IsInBuildMode() const { return bBuildMode; }
	EForestItem GetSelectedBuildItem() const { return SelectedBuildItem; }
	bool IsPlacementValid() const { return bPlacementValid; }
	/** The log wall planner is selected (shown as the first build option). */
	bool IsWallTool() const { return SelectedBuildItem == EForestItem::None; }
	bool IsWallStartSet() const { return bWallStartSet; }
	ALogWallBlueprint* GetFocusedBlueprint() const { return FocusedBlueprint.Get(); }
	ALogConstructionActor* GetFocusedLogActor() const { return FocusedLogActor.Get(); }
	AStoneMasonryActor* GetFocusedStoneActor() const { return FocusedStoneActor.Get(); }
	ACarvableLog* GetFocusedCarvable() const { return FocusedCarvable.Get(); }
	ECarveTool GetCarveTool() const { return CarveTool; }
	float GetCarveAngle() const { return CarveAngle; }
	int32 GetCarveHits() const { return CarveHits; }
	float GetCarveWidth() const { return CarveWidth; }
	float GetCarveDepth() const { return CarveDepth; }
	/** Round / flat notch depth as a share of the log's thickness: 1/6, 1/3, 1/2. */
	static float GetCarveDepthFraction(int32 Index);
	int32 GetCarveDepthIndex() const { return CarveDepthIndex; }
	/** The log in sight still has a place for the selected tool. */
	bool IsCarveSpotValid() const { return bCarveSpotValid; }
	/** E would lay the carried log across the log in sight. */
	bool CanPlaceAcross() const { return CarryPlace == ECarryPlace::Across; }

	/** What a left click does with the carried piece where the player is looking. */
	enum class ECarryPlace : uint8 { None, Across, Lay, Stand, Beam, Brace, Floor, Fuel, Repair, Slope, Load, Stack, Diagonal };
	ECarryPlace GetCarryPlace() const { return CarryPlace; }
	/** Diagonal placement: its angle to the log below, +45 or -45. */
	float GetDiagonalAngle() const { return DiagonalAngle; }
	/** The piece would be held up where it is about to go (the structure graph says so); false: it would fall. */
	bool IsPlaceSupported() const { return bPlaceSupported; }
	/** R while carrying: lay pieces down or stand them up as posts. */
	bool IsPlacingStanding() const { return bPlaceStanding; }
	/** R again: a piece leaning at an angle, a ramp or rafter; the wheel changes the angle. */
	bool IsPlacingSlope() const { return bPlaceSlope; }
	float GetSlopeAngle() const { return SlopeAngle; }
	/** F held: the lighter heats what is in sight (a piece of wood or a fire pit). */
	bool IsLighterHeld() const { return bLighterHeld; }
	/** Stones it takes to ring a fire pit. */
	static constexpr int32 FirePitStones = 7;
	/** An enemy hit on the player. */
	void TakeEnemyHit(float Amount);
	/** Shoulder load, 1 = full (one whole log). */
	float GetCarryLoad() const;

	/** For PIE test scripts (Python): E and the left click as the player presses them, and what is carried. */
	UFUNCTION(BlueprintCallable, Category = "Forest|Test") void TestPressInteract();
	UFUNCTION(BlueprintCallable, Category = "Forest|Test") void TestPressPrimary();
	UFUNCTION(BlueprintCallable, Category = "Forest|Test") float TestCarryLoad() const { return GetCarryLoad(); }
	UFUNCTION(BlueprintCallable, Category = "Forest|Test") int32 TestCarriedCount() const { return GetCarriedCount(); }
	UFUNCTION(BlueprintCallable, Category = "Forest|Test") FString TestCarriedItem() const;
	UFUNCTION(BlueprintCallable, Category = "Forest|Test") FString TestLastMessage() const;
	/** An axe hit on Log at a point of its surface given in its local space (point and outward normal), as PerformHit does. */
	UFUNCTION(BlueprintCallable, Category = "Forest|Test") void TestShapeHit(ACarvableLog* Log, FVector LocalPoint, FVector LocalNormal, bool bChannel);
	/** One shovel stroke at a ground point (as if aimed there): bFill dumps soil from the sack. Returns the message. */
	UFUNCTION(BlueprintCallable, Category = "Forest|Test") FString TestShovel(FVector Point, FVector Normal, bool bFill);
	UFUNCTION(BlueprintPure, Category = "Forest|Test") float TestLooseSoil() const { return LooseSoilM3; }
	/**
	 * Lays a raw log of LengthCm across the view 3 m ahead, then shoulders and places Courses more on it facing it (a wall),
	 * and finally one more while looking along the wall (a corner). Returns what each placement chose and where it went.
	 */
	UFUNCTION(BlueprintCallable, Category = "Forest|Test") FString TestBuildWall(int32 LengthCm, int32 Courses);
	/**
	 * Snap sectors on a log: lays a raw log, then looks at it from view offsets -90..90 (step 5) and reports which
	 * placement each picks and its angle to the log; then builds +45, four courses on it and a square-across log on top
	 * and reports each angle to the first log.
	 */
	UFUNCTION(BlueprintCallable, Category = "Forest|Test") FString TestSnapSectors(int32 LengthCm);

	/** How the player holds a piece when it goes down: lying, standing up as a post, or leaning at an angle. */
	enum class EShowcaseHold : uint8 { Lay, Stand, Slope };
	/**
	 * Puts Piece down exactly as the player would: shoulders it, faces ViewYaw, aims at Aim on Target (or the ground),
	 * picks the placement the game would pick and places it. Used by ALogShowcase to build its stations with the real
	 * mechanics. Returns what was chosen (None when nothing fits; the piece is then dropped back where it was).
	 */
	ECarryPlace PlaceForShowcase(ACarvableLog* Piece, AActor* Target, const FVector& Aim, const FVector& Normal, float ViewYaw,
		EShowcaseHold Hold, float SlopeDeg = 45.f);
	/** Name of a placement for reports. */
	static const TCHAR* CarryPlaceName(ECarryPlace Place);
	static bool IsProfileTool(ECarveTool Tool)
	{
		return Tool == ECarveTool::NotchRound || Tool == ECarveTool::NotchFlat || Tool == ECarveTool::Dovetail;
	}
	/** Tools that work an end of the log. */
	static bool IsEndTool(ECarveTool Tool) { return Tool == ECarveTool::Point || Tool == ECarveTool::Chamfer || Tool == ECarveTool::Trim; }
	static const TCHAR* GetCarveToolName(ECarveTool Tool);
	const FString& GetMessage() const { return Message; }
	float GetMessageAge() const;

protected:
	virtual void BeginPlay() override;

private:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartSprint();
	void StopSprint();
	void PrimaryAction();
	void PerformHit();
	void EquipTool(EForestTool Tool);
	void UpdateTool(float DeltaSeconds);
	void Interact();
	void CraftBeam();
	void CraftFirewood();
	void CraftStoneBlock();
	void CraftHalfLog();
	void CraftShortLog();
	void CraftHollowLog();
	void CraftPlank();
	void ToggleBuildMode();
	void ToggleTrowel();
	/** [B] Shovel on/off. Left click digs soil into the sack, right click dumps it. Rock cannot be dug. */
	void ToggleShovel();
	/** Digs (or with bFill fills) a 45 cm ball of soil at Point; Normal points out of the ground. Returns the message. */
	FString ShovelStroke(const FVector& Point, const FVector& Normal, bool bFill);
	void RotateBuild();
	void CycleBuildItem(const FInputActionValue& Value);
	void CancelBuild();

	/** Development helpers: G gives building materials, H builds a finished test room in front of the player. */
	void DebugGiveMaterials();
	void DebugBuildRoom();
	/** Development helper (J): fells the nearest tree or splits the nearest big log in front. */
	void DebugHarvestNearest();
	/** Development helpers: N empties hunger and thirst, M deals 35 damage. */
	void DebugStarve();
	void DebugHurt();
	/** Admin + Y: 30% damage to the carvable log in sight, as an attacker or a fire would do. */
	void DebugDamageStructure();
	/** P: admin mode. The debug keys only work in it, and [8] spawns a sled for free. */
	void ToggleAdminMode();
	bool RequireAdmin();

	void HandleDeath();

	/**
	 * Physical carrying: up to three logs on the shoulder, one big log on the shoulder,
	 * a trunk dragged along the ground by a rope tied round one end.
	 */
	/** Onto the shoulder if there is room for its weight; false (with a message) if not. */
	bool StartCarry(ACarvableLog* Piece);
	/** Re-stacks the carried pieces on the shoulder, heaviest at the bottom. */
	void ArrangeShoulder();
	void ShoulderBigLog(AHarvestableActor* BigLog);
	void StartRopeDrag(AHarvestableActor* Trunk);
	void StopRopeDrag();
	float RopeEndZ() const;
	/** E with full hands: one log off the shoulder, the big log down, or the rope untied. */
	void DropOne();
	/** Everything down (death). */
	void DropCarried();
	/** Uses up carried logs (crafting, building, wall plans). */
	bool ConsumeCarried(EForestItem Item, int32 Count);
	void UpdateCarry(float DeltaSeconds);
	float CarrySpeed() const;
	bool IsHandsBusy() const;
	void RefreshToolVisibility();

	UInputAction* MakeAction(const TCHAR* Name, EInputActionValueType Type);
	bool TraceFromView(float Distance, FHitResult& OutHit, bool bIgnorePlans = false) const;
	void UpdateFocus();
	void UpdateBuildPreview();
	bool ComputeBuildTransform(FTransform& OutTransform, const UInstancedStaticMeshComponent** OutSnappedTo = nullptr) const;
	bool IsPlacementFree(const FTransform& Transform, const UInstancedStaticMeshComponent* SnappedTo = nullptr) const;
	void PlaceSelected();
	void SelectBuildItem(EForestItem Item);
	void SelectNextAvailable(int32 Direction);
	void SetGhostValid(bool bValid);

	bool AimWallPoint(FVector& OutPoint) const;
	bool FindWallEndpoint(const FVector& Near, FVector& OutPoint, float& OutPhase) const;
	void UpdateWallPreview();
	void WallClick();

	/** In-game carving of carvable logs: aim snaps to the grid, a ghost shows the cut, two axe hits on the same spot apply it. */
	void UpdateCarveAim(const FHitResult& Hit, ACarvableLog* Log);
	void UpdateCarvePreview();
	void ApplyCarveHit(ACarvableLog* Log);
	/** Raw UnrealExport logs: the axe hit is a cross cut, a split, hewing or a channel by where it lands (RawLogShapes). */
	void ApplyShapeHit(ACarvableLog* Log, const FHitResult& Hit);
	/** R on a raw log: hits open a channel (gutter, then trough) instead of cutting / splitting / hewing. */
	bool bChannelMode = false;
	TWeakObjectPtr<ACarvableLog> ShapeTarget;
	uint8 ShapeAction = 0;
	int32 ShapeHits = 0;
	/** K (admin): three carvable raw logs (A 500, B 375, C 250 cm) in front of the player. */
	void DebugSpawnCarvableLogs();
	/** Admin + L: starts (or restarts) the guided wood chain demo, see AMechanicsDemo. */
	void DebugMechanicsDemo();

public:
	/**
	 * For the mechanics test: one axe hit on a carvable log with Tool, as if aimed at WorldPoint on the face with
	 * WorldNormal, through the same aim and two-hit logic the player uses. True when this hit made the cut.
	 */
	bool TestCarveHit(ACarvableLog* Log, ECarveTool Tool, const FVector& WorldPoint, const FVector& WorldNormal);

	/** For the mechanics test: E on a piece (true if it went onto the shoulder). */
	bool TestPickUp(ACarvableLog* Piece);
	/** For the mechanics test: a left click with the carried piece aimed at Hit (true if it was placed). */
	bool TestPlace(bool bStanding, const FHitResult& Hit);
	/** For the mechanics test: the lean mode chosen (angle in degrees) and the carried piece placed where the ground is aimed at. */
	bool TestPlaceSlope(const FHitResult& Hit, float AngleDeg);
	/** For the mechanics test: [C] with the stones given first when bGiveStones. Returns the pit, or null. */
	ACampfire* TestBuildFirePit(const FHitResult& Hit, bool bGiveStones);
	/** For the mechanics test: [C] once, with a stone given first. Returns the fire pit if this stone completed the ring. */
	ACampfire* TestPlaceStone(const FHitResult& Hit, bool bGiveStone);
	/** For the mechanics test: [X] on Target. */
	bool TestPush(AActor* Target, const FVector& Velocity);
	/** For the mechanics test: [V] on a piece. */
	void TestToggleTemporary(ACarvableLog* Piece);
	/** For the mechanics test: [E] on a cart (takes the last piece off it onto the shoulder). */
	ACarvableLog* TestUnload(AHaulCart* Cart);
	/** For the mechanics test: [E] on a loose stone or stick (a small stone and a stick go into the pack, a building stone into the hands). */
	bool TestPickUpResource(AActor* Resource);
	/** [I] Kazık çubuk: one stick from the pack driven into the ground at Hit. Null without a stick. */
	AStickStructure* PlantStakeAt(const FHitResult& Hit);
	bool IsCarryingStone() const;
	/** For the mechanics test: puts a carried stone down, empties the pack of sticks, gives sticks. */
	void TestDropStone();
	void TestUseUpSticks();
	void TestGiveSticks(int32 Count);
	/** For the mechanics test and the farm code: items into and out of the pack. */
	int32 TestGiveItem(EForestItem Item, int32 Count) { return AddItem(Item, Count); }
	bool TestRemoveItem(EForestItem Item, int32 Count) { return RemoveItem(Item, Count); }
	/** What [I] can make: the three stick builds, a field, hide clothing and rope. */
	/** What [I] makes: a stake driven in by hand, a field, hide clothing, rope, a rope line. */
	enum ECraftEntry : uint8 { CraftStake, CraftField, CraftGarment, CraftRope, CraftRopeLine, CraftEntryCount };
	static FString CraftEntryName(uint8 Entry);
	/** For the mechanics test: [I] entry Entry made (on Hit when it is placed on the ground). */
	bool TestCraft(uint8 Entry, const FHitResult& Hit);
	/** [8]: eats the most filling food in the pack. */
	void EatFood();
	/** For the mechanics test: E on a note, drinking where the player stands, riding a rope from its near end. */
	bool TestReadNote(ARemnantNote* Note);
	bool TestDrink();
	/** [E] on a raw material source: one gathering into the pack. Returns how many items went in. */
	int32 TestGather(ARawMaterialNode* Node);
	bool TestRideRope(ARopeLine* Line);
	/** The rope line being strung: its first end is set. */
	bool HasRopeAnchor() const { return bRopeAnchorSet; }
	bool IsRiding() const { return RideRope.IsValid(); }
	bool IsStickMode() const { return bStickMode; }
	/** [F1]: the full list of controls. */
	bool IsHelpOpen() const { return bHelpOpen; }
	void ToggleHelp() { bHelpOpen = !bHelpOpen; }
	FString GetStickModeLabel() const;
	/** For the mechanics test: the lighter held on Target for Seconds. */
	void TestLight(AActor* Target, float Seconds);

	/** Console: MechanicsDemo. Same as admin + L, without needing admin mode. */
	UFUNCTION(Exec)
	void MechanicsDemo();

private:

	/** Adds as many as fit in the backpack; returns how many went in. */
	int32 AddItem(EForestItem Item, int32 Count);
	/** Would the full stack fit without partially consuming the world pickup? */
	bool CanAddItem(EForestItem Item, int32 Count) const;
	/** Adds definitions for this project's stackable items to the canonical inventory. */
	void InitializeCoreInventoryDefinitions();
	void ToggleInventory();
	bool RemoveItem(EForestItem Item, int32 Count);
	void Craft(EForestItem Input, int32 InputCount, EForestItem Output, int32 OutputCount);
	void ShowMessage(const FString& Text);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USurvivalComponent> Survival;

	/** Keeps the world chunks round the player loaded so water, soil and seasons are simulated where the player is. */
	UPROPERTY(VisibleAnywhere, Category = "World")
	TObjectPtr<UChunkManager> WorldChunks;

	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	TObjectPtr<UInventoryComponent> CoreInventory;

	/** Physical stone handling uses the same actor from pickup through placement. */
	UPROPERTY(VisibleAnywhere, Category = "Stone")
	TObjectPtr<UStoneCarryComponent> StoneCarry;

	UPROPERTY(VisibleAnywhere, Category = "Stone")
	TObjectPtr<UStonePlacementComponent> StonePlacement;

	/** Heavy actions cost stamina (GDD 6.1). */
	UPROPERTY(EditAnywhere, Category = "Forest")
	float SwingStaminaCost = 8.f;

	bool bWantsSprint = false;


	EForestItem CarriedItem = EForestItem::None;
	/** Pieces on the shoulder, bottom first; how many fit depends on their wood (ACarvableLog::GetShoulderCapacity). */
	TArray<TWeakObjectPtr<ACarvableLog>> CarriedLogs;
	TWeakObjectPtr<AHarvestableActor> ShoulderedLog;
	TWeakObjectPtr<AHarvestableActor> RopedLog;
	FVector RopeFront = FVector::ZeroVector;
	static constexpr float RopeLength = 200.f;

	UPROPERTY()
	TObjectPtr<UCableComponent> Rope;
	FTransform RespawnTransform;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> BuildPreview;

	/** Hand position in front of the camera; swinging rotates this. */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> ToolPivot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ToolMesh;

	/** Ghost of the whole log wall while choosing its end point. */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> WallPreview;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInputAction>> Actions;

	UPROPERTY(Transient)
	TObjectPtr<AStructureManager> Structures;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GhostMaterial;

	UPROPERTY(EditAnywhere, Category = "Forest")
	float ReachDistance = 300.f;

	UPROPERTY(EditAnywhere, Category = "Forest")
	float BuildDistance = 600.f;

	UPROPERTY(EditAnywhere, Category = "Forest")
	float HitDamage = 20.f;

	UPROPERTY(EditAnywhere, Category = "Forest")
	float SwingCooldown = 0.55f;

	UPROPERTY(EditAnywhere, Category = "Forest")
	float WalkSpeed = 380.f;

	UPROPERTY(EditAnywhere, Category = "Forest")
	float SprintSpeed = 650.f;

	// Item counts now have one owner: CoreInventory. Physical logs remain ACarvableLog actors.
	bool bInventoryOpen = false;
	bool bAdminMode = false;
	TWeakObjectPtr<AHarvestableActor> FocusedHarvestable;
	TWeakObjectPtr<APickupItem> FocusedPickup;
	TWeakObjectPtr<ARawMaterialNode> FocusedRawNode;
	TWeakObjectPtr<ALogWallBlueprint> FocusedBlueprint;
	TWeakObjectPtr<ALogConstructionActor> FocusedLogActor;
	TWeakObjectPtr<AStoneMasonryActor> FocusedStoneActor;

	EForestTool CurrentTool = EForestTool::Axe;
	float SwingTime = -1.f;
	float DiagonalAngle = 45.f;
	/** Loose soil in the sack, m3 (dug soil swells by 25 %). */
	float LooseSoilM3 = 0.f;
	static constexpr float MaxLooseSoilM3 = 1.0f;
	static constexpr float ShovelRadiusCm = 45.f;
	bool bPendingHit = false;

	bool bWallStartSet = false;
	FVector WallStart = FVector::ZeroVector;
	float WallPhase = 0.f;

	FString Message;
	double MessageTime = -100.0;
	double LastSwingTime = -100.0;
	float BaseFOV = 90.f;
	float FOVPunch = 0.f;
	bool bBuildMode = false;
	bool bPlacementValid = false;
	bool bGhostValid = true;
	float BuildYawOffset = 0.f;
	EForestItem SelectedBuildItem = EForestItem::None;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> CarvePreview;

	/**
	 * Build and carve previews are drawn as lines (Sons of the Forest blueprint style). The preview meshes above
	 * still carry the transform and validity but are hidden in game; this redraws their outlines every frame.
	 */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<ULineBatchComponent> PreviewLines;
	void DrawPreviewLines();
	/** Red dashed line where the axe will cut: round a tree at the aimed height, round a lying log where it splits, on a carvable log along the cut. */
	void DrawCutGuide();
	/** Where the view trace hit the focused target this frame. */
	FVector FocusHitPoint = FVector::ZeroVector;

	/**
	 * Carrying pieces: where a left click puts the top one. Across a lying log (saddle-notched, see
	 * ACarvableLog::PlaceAcross), else on the ground lying down or standing up as a post (R chooses).
	 */
	ECarryPlace CarryPlace = ECarryPlace::None;
	bool bPlaceStanding = false;
	bool bPlaceSlope = false;
	float SlopeAngle = 45.f;
	/** [F] held. */
	bool bLighterHeld = false;
	/** [I]: stick building on/off; Q / the wheel picks what, left click builds it where the ground is aimed at. */
	bool bStickMode = false;
	bool bHelpOpen = false;
	/** [I] rope line: the first end, set by the first click. */
	bool bRopeAnchorSet = false;
	FVector RopeAnchor = FVector::ZeroVector;
	/** Riding a rope: from one end to the other. */
	TWeakObjectPtr<ARopeLine> RideRope;
	FVector RideFrom = FVector::ZeroVector;
	FVector RideTo = FVector::ZeroVector;
	float RideT = 0.f;
	void UpdateRide(float DeltaSeconds);
	/** Water where the player stands, or a step ahead. */
	bool IsWaterAtHand() const;
	uint8 StickBuildIndex = 0;
	void ToggleStickMode();
	bool StickBuildSpot(FHitResult& OutHit) const;
	void LighterPressed();
	void LighterReleased();
	/** Heats the target in sight for Seconds. */
	void HeatTarget(AActor* Target, float Seconds);
	/** [C]: lays a stone where the ground is aimed at; seven close together make a fire pit. */
	void PlaceFireStone();
	/** [V]: marks the piece in sight as scaffolding (temporary) or not. */
	void ToggleTemporary();
	/** [X]: shoves the loose piece, cart or sled in sight. */
	void PushAimed();
	/** [O]: cuts a door-wide gap (two modules) out of the installed piece in sight. */
	void OpenAimed();
	/** Admin [Z]: starts a raid. Admin [U]: sets a cart, a sled and a rack down in front. */
	void DebugRaid();
	void DebugHaulers();
	/** Admin [9]: the clock three hours on. */
	void DebugSkipTime();
	bool bPlaceSupported = true;
	TWeakObjectPtr<AActor> PlaceTarget;
	/** Re-checks the structure after a piece went in or came out; says how many pieces fell. */
	void SettleStructure();
	FCrossPlacement CrossPlacement;
	TWeakObjectPtr<AActor> CrossLower;
	FTransform PlaceTransform;
	void UpdateCarryPlacement();
	void UpdateCarryPlacementAt(const FHitResult& Hit, bool bHit);
	ACarvableLog* GetTopCarried() const;
	bool PlaceCarried();

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CarveCylinder;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CarveCube;

	TWeakObjectPtr<ACarvableLog> FocusedCarvable;
	ECarveTool CarveTool = ECarveTool::Notch;
	/** Notch: crossing angle (90 = square, 0 = lengthwise groove). Split: angle of the plane around the log axis. */
	float CarveAngle = 90.f;
	float CarveX = 0.f;
	float CarveSide = 0.f;
	/**
	 * Joints are sized by the log in sight: as long as it is thick (a crossing log of the same size sits in them),
	 * and 1/6, 1/3 or 1/2 of its thickness deep (wheel). Set by UpdateCarveAim.
	 */
	float CarveWidth = 70.f;
	float CarveDepth = 17.5f;
	int32 CarveDepthIndex = 0;
	/** Where the cut would go is one of the log's snap points (false: none left for this tool). */
	bool bCarveSpotValid = false;
	/** Hits land on the same cut only while tool, spot and angle stay the same. */
	int32 CarveHits = 0;
	FVector4 CarveKey = FVector4(0.f, 0.f, 0.f, -1.f);
	TWeakObjectPtr<ACarvableLog> CarveTarget;
};

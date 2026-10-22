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
class UInstancedStaticMeshComponent;
class USurvivalComponent;
class USurvivalExcavationAdapter;
class UUndergroundExposureComponent;
class UCableComponent;
class ULineBatchComponent;
struct FWorldPlacementQuery;
struct FWorldPlacementResult;

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
	USurvivalComponent* GetSurvivalMutable() const { return Survival; }
	UUndergroundExposureComponent* GetExposureMutable() const { return UndergroundExposure; }
	const FTransform& GetRespawnTransformForSave() const { return RespawnTransform; }
	void SetRespawnTransformForSave(const FTransform& Transform) { RespawnTransform = Transform; }
	void ExportInventorySaveData(TArray<uint8>& OutItemIds, TArray<int32>& OutCounts) const;
	static bool ValidateInventorySaveData(const TArray<uint8>& ItemIds, const TArray<int32>& Counts, FString& OutError);
	bool ImportInventorySaveData(const TArray<uint8>& ItemIds, const TArray<int32>& Counts, FString& OutError);
	bool CanSaveWorldState(FString& OutError) const;
	/** The log in the player's hands (None when empty; TrunkLog while a trunk is dragged by rope). */
	EForestItem GetCarriedItem() const { return CarriedItem; }
	int32 GetCarriedCount() const { return CarriedItem == EForestItem::Log ? CarriedLogs.Num() : (CarriedItem != EForestItem::None ? 1 : 0); }
	AHarvestableActor* GetFocusedHarvestable() const { return FocusedHarvestable.Get(); }
	APickupItem* GetFocusedPickup() const { return FocusedPickup.Get(); }
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
	enum class ECarryPlace : uint8 { None, Across, Lay, Stand, Beam, Brace, Floor, Fuel, Repair, Slope, Load };
	ECarryPlace GetCarryPlace() const { return CarryPlace; }
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
	bool ComputeBuildTransform(
		FTransform& OutTransform,
		const UInstancedStaticMeshComponent** OutSnappedTo = nullptr,
		FWorldPlacementQuery* OutVoxelQuery = nullptr,
		FWorldPlacementResult* OutVoxelPlacement = nullptr) const;
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
	/** For the mechanics test: the lighter held on Target for Seconds. */
	void TestLight(AActor* Target, float Seconds);

	/** Console: MechanicsDemo. Same as admin + L, without needing admin mode. */
	UFUNCTION(Exec)
	void MechanicsDemo();

private:

	/** Adds as many as fit in the backpack; returns how many went in. */
	int32 AddItem(EForestItem Item, int32 Count);
	/** Would Count of Item fit, after Freed of FreedItem left the backpack? */
	bool CanAddItem(EForestItem Item, int32 Count, EForestItem FreedItem = EForestItem::None, int32 Freed = 0) const;
	void ToggleInventory();
	bool RemoveItem(EForestItem Item, int32 Count);
	void Craft(EForestItem Input, int32 InputCount, EForestItem Output, int32 OutputCount);
	void ShowMessage(const FString& Text);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USurvivalComponent> Survival;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Survival", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USurvivalExcavationAdapter> SurvivalExcavation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Survival", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UUndergroundExposureComponent> UndergroundExposure;

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

	TMap<EForestItem, int32> Inventory;
	bool bInventoryOpen = false;
	bool bAdminMode = false;
	TWeakObjectPtr<AHarvestableActor> FocusedHarvestable;
	TWeakObjectPtr<APickupItem> FocusedPickup;
	TWeakObjectPtr<ALogWallBlueprint> FocusedBlueprint;
	TWeakObjectPtr<ALogConstructionActor> FocusedLogActor;
	TWeakObjectPtr<AStoneMasonryActor> FocusedStoneActor;

	EForestTool CurrentTool = EForestTool::Axe;
	float SwingTime = -1.f;
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

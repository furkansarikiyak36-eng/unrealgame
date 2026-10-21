// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CarvableLog.generated.h"

class UDynamicMesh;
class UDynamicMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UStaticMesh;
class UStaticMeshComponent;
class UPointLightComponent;
class APickupItem;
class UMaterialDefinition;

UENUM()
enum class ECarveTool : uint8
{
	Notch,
	CrossCut,
	Split,
	/** Crosswise notch with a round side profile (a C cut into the log). */
	NotchRound,
	/** Crosswise notch with straight walls and a flat floor. */
	NotchFlat,
	/** An end worked into a point, a stake. */
	Point,
	/** The edge of an end knocked off at 45 degrees. */
	Chamfer,
	/** A pointed or chamfered end cut back square. */
	Trim,
	/** A flat notch that narrows towards the surface, so what sits in it cannot be pulled out. */
	Dovetail,
	/** A lengthwise channel along the top or bottom of the whole log (a trough, a gutter). */
	Groove,
	Count UMETA(Hidden),
};

/** What an end of a log looks like. */
enum class ELogEnd : uint8
{
	Flat,
	Pointed,
	Chamfered,
};

class ACarvableLog;
class ARaidEnemy;
/** Where a piece is: lying loose in the world, on a shoulder or cart, or part of a structure. */
enum class ELogMotion : uint8 { Loose, Carried, Installed };
/** Fired after every successful cut on any carvable log (the log that was cut, the tool that cut it). */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnLogCarved, ACarvableLog*, ECarveTool);
/** Fired when a log has been laid across another and seated in its saddle notch (the upper log). */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLogPlacedAcross, ACarvableLog*);
/** Fired when a piece has been stood upright as a post. */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLogStoodUp, ACarvableLog*);
/** Fired when a damaged piece of a structure has been replaced (the new piece). */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLogRepaired, ACarvableLog*);

/** A joint cut into a piece (a cup, a socket...), kept so the same cut can be made in a replacement. */
struct FJointCut
{
	ECarveTool Tool = ECarveTool::Notch;
	float LocalX = 0.f;
	float SideAngleDeg = 0.f;
	float AngleDeg = 0.f;
	float Width = 0.f;
	float Depth = 0.f;
	float MateRadius = 0.f;
};

/** Where a log laid across another one goes, and the saddle notch that seats it (see ACarvableLog::ComputeCrossPlacement). */
struct FCrossPlacement
{
	/** World transform of the upper log (its local X runs square across the lower log). */
	FTransform Upper;
	/** Upper log's local X of the notch, right above the lower log's axis. */
	float NotchX = 0.f;
	float LowerRadius = 0.f;
	float UpperRadius = 0.f;
};

/**
 * A raw log that can be carved while playing: smooth cups and grooves (notches), cross cuts and
 * lengthwise splits, cut from the scanned mesh with Geometry Script. Local X is the log axis.
 * Carved faces use material slot 1 (cut wood, rings computed in local space), so they need no UVs.
 */
/** Wood catches fire slowly, burns for as long as it holds wood, and is charred afterwards. */
enum class EBurnState : uint8 { Intact, Igniting, Burning, Charred };

UCLASS()
class MYPROJECT2_API ACarvableLog : public AActor
{
	GENERATED_BODY()

public:
	ACarvableLog();

	/** Spawns a carvable copy of a raw log mesh (e.g. /Game/Building/Raw/SM_RawLog_C). */
	static ACarvableLog* SpawnFromStaticMesh(UWorld* World, UStaticMesh* Mesh, const FTransform& Transform);

	/** A whole log as a tree gives it (the greybox log, six modules long), falling into place. */
	static ACarvableLog* SpawnLog(UWorld* World, const FTransform& Transform);

	/** Puts a log lying on the ground on the bench: replaces the pickup with a carvable log of the same mesh, in place. */
	static ACarvableLog* SpawnFromPickup(APickupItem* Pickup);

	/** Axis of a log lying in the world: a carvable log or a carryable log item. False for anything else. */
	static bool GetLogAxis(const AActor* Log, FVector& OutCentre, FVector& OutAxis, float& OutRadius, float& OutHalfLength);

	/** Pick the complementary +45/-45 degree course in a shared reference frame, alternating above an existing diagonal log. */
	static float GetComplementarySnapYaw(float ReferenceYaw, float LowerYaw, float DesiredYaw);

	/** Place a horizontal log above Lower at the shared-frame complementary 45 degree angle with a seated notch. */
	static bool ComputeComplementarySnapPlacement(const ACarvableLog* Lower, const FVector& Aim, const ACarvableLog* Upper,
		float ReferenceYaw, float DesiredYaw, FCrossPlacement& OutPlacement);

	/**
	 * Laying a log (UpperMesh) square across Lower, aimed at Aim by a player standing at Viewer, like the corner of a
	 * log wall: it crosses near one end (an overhang past the lower log), runs away from the viewer and sits in a
	 * saddle notch cut half the lower log deep into its underside. False if Lower is not a lying log.
	 */
	static bool ComputeCrossPlacement(const AActor* Lower, const FVector& Aim, const FVector& Viewer, const ACarvableLog* Upper,
		FCrossPlacement& Out);

	/** An anchored, horizontal log lying across the top of Post (a beam on a post), if there is one. */
	static ACarvableLog* FindBeamOnPost(const ACarvableLog* Post);

	/**
	 * A brace for Piece: from the side of Post up to the underside of the beam on its top at 45 degrees, running towards
	 * the middle of the beam. It needs a piece between two and four modules long; how far down the post it starts follows
	 * from the piece's length. False if there is no beam on the post or the piece does not fit.
	 */
	static bool ComputeBracePlacement(const ACarvableLog* Post, const ACarvableLog* Piece, FTransform& OutTransform);

	/**
	 * A split piece laid across a lying log as a floor board: flat face up, running away from Viewer over the log, its
	 * near end a little past it, resting on top. False if Piece is not split or Lower is not a lying log.
	 */
	static bool ComputeFloorPlacement(const AActor* Lower, const FVector& Aim, const FVector& Viewer, const ACarvableLog* Piece,
		FTransform& OutTransform);

	/** Rim of that saddle notch on the upper log, in its local space (for the dashed guide before it is placed). */
	static void GetCrossNotchGuide(const FCrossPlacement& Placement, const ACarvableLog* Upper,
		TArray<TArray<FVector>>& OutLines, TArray<bool>& OutClosed);

	/**
	 * Lays Upper across Lower as computed: moves it there (off the shoulder), cuts its saddle notch and seats it. Both
	 * logs are anchored where they are (a lying log item is turned into a carvable log first). Returns Upper.
	 */
	static ACarvableLog* PlaceAcross(AActor* Lower, const FCrossPlacement& Placement, ACarvableLog* Upper);

	/**
	 * 0 sound .. 1 burnt through or broken (wood goes darker as it takes damage). From the Wood structure rules: above 0.6 the
	 * joints of a piece let go (it no longer holds or is held through them), above 0.85 it cannot hold anything at all.
	 */
	float GetDamage() const { return Damage; }
	void AddDamage(float Amount);
	void SetDamage(float Value);

	/** Apply one bounded world-weather interval to this existing wood condition state. */
	void SimulateMaterialEnvironment(float ElapsedDays, float AmbientTemperatureC, bool bRainExposed, float RainfallMm);
	UFUNCTION(BlueprintPure, Category = "Material|Condition") float GetMoistureContent() const { return MoistureContent; }
	UFUNCTION(BlueprintPure, Category = "Material|Condition") float GetMaterialAgeDays() const { return MaterialAgeDays; }

	// --- Fire. Any piece of wood burns. It takes heat to catch (thin pieces quickly, thick ones slowly), burns for as long as
	// the wood it holds (see ACampfire::FuelSecondsOf) taking damage all the while, and ends up charred. Burning pieces
	// heat the wood around them and light fires (UBurnSubsystem drives all of it). ---

	EBurnState GetBurnState() const { return BurnState; }
	bool IsBurning() const { return BurnState == EBurnState::Burning; }
	bool IsCharred() const { return BurnState == EBurnState::Charred; }
	/** Seconds of full heat it takes this piece to catch: its radius times 0.4, between 2 and 20. */
	float GetIgniteSeconds() const;
	/** Heat from a lighter, a flame or a fire. True once the piece has caught (this call or before). */
	bool ApplyHeat(float Seconds);
	/** Catches fire at once. False if it is charred, or too many pieces already burn (MaxBurning). */
	bool Ignite();
	/** Seconds of burning left, and the share of its fuel used so far (0..1). */
	float GetBurnSecondsLeft() const { return BurnLeft; }
	float GetHeatShare() const;
	/** Called by UBurnSubsystem: burns for Seconds (fuel, damage, flames). */
	void StepBurn(float Seconds);
	/** Called by UBurnSubsystem: heat fades when nothing heats the piece. Returns true while it is still warming. */
	bool CoolStep(float Seconds);
	/** Most pieces burning at once; more would be too much to simulate. */
	static constexpr int32 MaxBurning = 12;

	/** Scaffolding: part of the building site only until the structure is done. */
	bool IsTemporary() const { return bTemporary; }
	void SetTemporary(bool bValue) { bTemporary = bValue; }

	/** A damaged piece in a structure can be swapped for another one of the same size (see ReplaceDamaged). */
	static bool CanReplace(const ACarvableLog* Old, const ACarvableLog* Replacement);
	/**
	 * Replacement takes Old's place, anchored, with the same joints cut into it. Old is released from the structure, damage and
	 * all, for the caller to pick up.
	 */
	static bool ReplaceDamaged(ACarvableLog* Old, ACarvableLog* Replacement);

	/** Part of a structure now: stays exactly where it is, no physics, also after further cuts. */
	void Anchor();
	/** Off the structure to be carried: no longer anchored, physics stays off (the shoulder takes it). */
	void Release() { bAnchored = false; }
	/** No longer held up: anchoring is dropped and physics takes over, so it tips or falls where it is. */
	void LetFall();
	/** A beam that cannot carry its own span: it lets go and snaps in two at LocalX, both halves left to physics. */
	ACarvableLog* SnapAt(float LocalX);
	/** Joints cut into this log, as local X ranges. */
	const TArray<FVector2f>& GetCutSpans() const { return CutSpans; }
	bool IsAnchored() const { return bAnchored; }

	/** Loose (free to roll and slide), Carried (shoulder, cart) or Installed (part of a structure). */
	ELogMotion GetMotion() const { return bCarried ? ELogMotion::Carried : (bAnchored ? ELogMotion::Installed : ELogMotion::Loose); }
	/** A shove: a loose piece takes Velocity (cm/s) and rolls or slides until it rests. False for carried or installed pieces. */
	bool Push(const FVector& Velocity);
	/** A hard landing or hit hurts wood. ImpulseSize is the normal impulse of the hit (kg cm/s); small knocks do nothing. */
	void AddImpactDamage(float ImpulseSize);
	static constexpr float ImpactThreshold = 1200000.f;
	/** A moving piece that strikes an enemy hurts it, by the impulse of the blow. */
	static void HitEnemy(ARaidEnemy* Enemy, float ImpulseSize);

	/**
	 * A piece leaning at AngleDeg (10..80) with its lower end on the ground at LowEnd, rising in the direction of Forward
	 * (flattened): a ramp, a rafter, a stair stringer. False if Forward points straight up or down.
	 */
	static bool ComputeSlopePlacement(const FVector& LowEnd, const FVector& Forward, float AngleDeg, const ACarvableLog* Piece, FTransform& Out);

	/**
	 * Cuts a gap Modules wide (an even number, at least 2) out of this piece, on the module lines nearest CentreX, leaving a
	 * piece on each side: a door or window opening in a wall course. At least one module must stay on each side. Pieces of an
	 * installed log stay installed. Returns the piece to the right of the gap, or null if there is no room.
	 */
	ACarvableLog* CutOpening(float CentreX, int32 Modules);

	/** Cuts a channel along the whole log, opening towards SideAngleDeg (0 = top). */
	bool CutGroove(float SideAngleDeg);

	/** Any cut on any log; the mechanics demo ticks off its carving goals with it. */
	static FOnLogCarved OnLogCarved;
	static FOnLogPlacedAcross OnLogPlacedAcross;
	static FOnLogStoodUp OnLogStoodUp;
	static FOnLogRepaired OnLogRepaired;

	/** Wood in this piece (cm3); what it weighs on the shoulder. */
	float GetVolume() const { return Volume; }
	/** How much wood fits on the shoulder: one whole log (390 x 70 cm), or two halves, or six one-module pieces. */
	static float GetShoulderCapacity();
	/** Split lengthwise at some point (a half, a quarter): not a round log any more. */
	bool IsSplit() const { return bSplitPiece; }
	/** A whole round log, full length: what a wall plan or a recipe takes as "a log". */
	/** A whole carryable log: four modules (the 300 cm raw log), not split. */
	bool IsFullLog() const { return !bSplitPiece && GetLength() >= 4.f * ModuleLength - 5.f; }
	/** The piece's extent in its own frame. */
	FBox GetLocalBox() const { return FBox(FVector(MinX, -Radius, -Radius), FVector(MaxX, Radius, Radius)); }

	/** On the player's shoulder: no physics, no collision, attached to AttachTo at Relative. */
	void BeginCarry(USceneComponent* AttachTo, const FTransform& Relative);
	bool IsCarried() const { return bCarried; }
	/** Off the shoulder to Transform: anchored (a post) or left to settle where it lands. */
	void PlaceAt(const FTransform& Transform, bool bAnchorIt);

	/** Lying on the ground at Ground, its length along Yaw. */
	FTransform GetLayTransform(const FVector& Ground, float Yaw) const;
	/** Standing upright as a post with its lower end on Ground. */
	FTransform GetStandTransform(const FVector& Ground, float Yaw) const;

	float GetLength() const { return MaxX - MinX; }
	/** Triangles in the current mesh; grows with every cut, used by the mechanics demo to check a cut landed. */
	int32 GetTriangleCount() const;
	float GetRadius() const { return Radius; }
	float GetCenterX() const { return (MinX + MaxX) * 0.5f; }

	float GetDiameter() const { return Radius * 2.f; }

	/**
	 * The modular grid every cut snaps to: lines ModuleLength apart, counted from one end of the log as it was felled.
	 * Pieces keep the same lines (they share the log's local frame), so pieces cut on the grid always fit together.
	 * A 390 cm log is exactly six modules.
	 */
	static constexpr float ModuleLength = 75.f;

	/** Local X of the grid lines that lie inside the log (not its ends). */
	void GetModuleLines(TArray<float>& OutX) const;
	float GetMinX() const { return MinX; }
	float GetMaxX() const { return MaxX; }

	/**
	 * Where an axe can make this cut on this log, all on the module grid. Joints (notch, round, flat) go at the
	 * corners, one module in from each end, and on the middle line of a long log for a cross wall; a flat notch also
	 * right at each end (an end lap, open at the end). A cross cut goes on any grid line that leaves both pieces at
	 * least one module long. Places already cut are taken.
	 * Width is the joint's length along the log. Split has no points (it runs the whole log).
	 */
	void GetSnapPoints(ECarveTool Tool, float Width, TArray<float>& OutX) const;

	/** The snap point nearest LocalX; false when the log has none left for this tool. */
	bool FindSnapPoint(ECarveTool Tool, float LocalX, float Width, float& OutX) const;

	/** No earlier joint overlaps [X0, X1]. */
	bool IsSpanFree(float X0, float X1) const;

	/**
	 * Cuts a cup for a crossing log of MateRadius. SideAngleDeg turns around the log axis
	 * (0 = top, 180 = bottom); CrossAngleDeg is the crossing angle (90 = square, 0 = lengthwise groove).
	 */
	bool Notch(float LocalX, float SideAngleDeg, float CrossAngleDeg, float MateRadius, float Depth);

	/**
	 * Cuts a crosswise notch Width long (along the log) and Depth deep, opening toward SideAngleDeg
	 * (0 = top, 180 = bottom). Round leaves a circular side profile, flat a rectangular one.
	 * Depth is clamped to the radius.
	 */
	bool CutProfile(float LocalX, float SideAngleDeg, float Width, float Depth, bool bRound);

	/** A dovetail socket: Width wide at the floor, 70% of that at the surface, Depth deep, through the whole log. */
	bool CutDovetail(float LocalX, float SideAngleDeg, float Width, float Depth);

	/** A stake point is one module long, ending in a flat tip of this radius; a chamfer takes this much off the edge. */
	static constexpr float PointLength = 65.f;
	static constexpr float PointTipRadius = 4.f;
	static constexpr float ChamferSize = 8.f;

	/**
	 * Works one end: Point (a stake), Chamfer (bevelled edge) or Trim (cut a pointed or chamfered end back square).
	 * Pointed and chamfered ends are only made on flat ends with wood to spare; Trim only on shaped ends.
	 */
	bool ShapeEnd(bool bMaxEnd, ECarveTool Shape);

	ELogEnd GetEnd(bool bMaxEnd) const { return bMaxEnd ? EndMax : EndMin; }
	/** 0 whole, 1 half, 2 quarter: how many times the log has been split lengthwise. */
	int32 GetSplitCount() const { return SplitCount; }
	/** Direction (in the YZ plane, as Split's angle) the flat face of a split piece looks out of the wood. */
	float GetFlatAngle() const { return FlatAngle; }
	float GetGridOriginX() const { return bGridSet ? GridOriginX : MinX; }

	/** Frame of a profile notch for previews: axis direction out of the log, across direction, and the cutter centre. */
	void GetProfileFrame(float LocalX, float SideAngleDeg, float Width, float Depth, bool bRound,
		FVector& OutCentre, FVector& OutOut, FVector& OutAcross) const;

	/**
	 * Where a cut would meet the log's surface, as local-space polylines lying just above the bark:
	 * the rim of a notch, the ring of a cross cut, the two lines of a split. Used for the dashed cut guide.
	 * Angle is the notch crossing angle or the split angle; Width / Depth are for the round and flat notches.
	 */
	void GetCutGuide(ECarveTool Tool, float LocalX, float SideAngleDeg, float AngleDeg, float Width, float Depth,
		TArray<TArray<FVector>>& OutLines, TArray<bool>& OutClosed) const;

	/** Saws across at LocalX; the part beyond becomes a new log. */
	ACarvableLog* CrossCut(float LocalX);

	/** Splits along the axis on the plane at AngleDeg around X (0 = vertical); the other half becomes a new log. */
	ACarvableLog* Split(float AngleDeg);

	/** Where the cup's mating log axis runs (local space), for previews. */
	void GetNotchFrame(float LocalX, float SideAngleDeg, float CrossAngleDeg, float MateRadius, float Depth,
		FVector& OutCentre, FVector& OutDirection) const;

	/** Placed in a level: the raw mesh this log starts from (loaded when play begins). */
	UPROPERTY(EditAnywhere, Category = "Carving")
	TObjectPtr<UStaticMesh> SourceMesh;

	/** Start without physics (stays where it was placed until hit apart). */
	UPROPERTY(EditAnywhere, Category = "Carving")
	bool bStartAsleep = true;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved,
		FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

	/**
	 * Lets the log fall into place (after spawning or a cut), then freezes it once it rests, so it does not keep
	 * rolling on the ground under the axe: aim and the two-hit rule need it to hold still.
	 */
	void Resettle();
	void LoadSource();

private:
	void SetMaterials(UMaterialInterface* Bark);
	void FinishEdit();
	void SubtractTool(UDynamicMesh* Target, UDynamicMesh* Tool);
	void SubtractHalfSpace(UDynamicMesh* Target, const FVector& Normal, float Offset);
	ACarvableLog* SpawnPiece(UDynamicMesh* Mesh) const;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UDynamicMeshComponent> MeshComponent;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BarkMaterial;

	float MinX = 0.f;
	float MaxX = 0.f;
	/** Radius of the original log; stays the reference for cup depths after carving. */
	float Radius = 0.f;

	float Damage = 0.f;

	/** Optional project-tuned material profile; absent profiles use the timber defaults in CarvableLog.cpp. */
	UPROPERTY(EditDefaultsOnly, Category = "Material|Profile")
	TObjectPtr<UMaterialDefinition> MaterialDefinition = nullptr;

	/** Moisture is a material condition, while Damage remains the single shared structural integrity value. */
	UPROPERTY(VisibleInstanceOnly, SaveGame, Category = "Material|Condition")
	float MoistureContent = 0.0f;

	UPROPERTY(VisibleInstanceOnly, SaveGame, Category = "Material|Condition")
	float MaterialAgeDays = 0.0f;
	bool bTemporary = false;
	TArray<FJointCut> JointCuts;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DamageMaterial;
	void UpdateDamageLook();

	EBurnState BurnState = EBurnState::Intact;
	float HeatSeconds = 0.f;
	/** A piece just made (spawned, cut) lands without impact damage for this many seconds. */
	float ImpactGrace = 2.f;
	bool bHeatedSinceStep = false;
	float BurnLeft = 0.f;
	float BurnTotal = 0.f;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> FlameParts;
	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> FlameLight;
	void ShowFlames(bool bShow, float Strength);
	void EndBurn();

	bool bAnchored = false;
	bool bCarried = false;
	ELogEnd EndMin = ELogEnd::Flat;
	ELogEnd EndMax = ELogEnd::Flat;
	int32 SplitCount = 0;
	float FlatAngle = 0.f;
	bool bSplitPiece = false;
	float Volume = 0.f;

	/** Local X where the module grid starts; set once, when the log is made, and handed on to its pieces. */
	float GridOriginX = 0.f;
	bool bGridSet = false;

	/** Seconds since Resettle, and how long it has been nearly still. */
	float SettleTime = 0.f;
	float StillTime = 0.f;

	/** Joints already cut, as local X ranges: no second joint and no cross cut inside them. */
	TArray<FVector2f> CutSpans;
};

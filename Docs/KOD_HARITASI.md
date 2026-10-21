# Kod haritası

Otomatik üretildi (21 October 2026 17:59) — `python Scripts/update_docs.py`. Elle düzenlenmez.

Her sınıf için genel (public) fonksiyonlar ve üstlerindeki açıklama. Kaynak `Source/MyProject2` (`Wood/` alt klasörü web Claude paketi).
Açıklaması olmayanlar sayfanın sonunda: bunlar için başlık dosyasına yorum yazılmalı.

## BuildingNodeActor.h

**ABuildingNodeActor** (class) — Base building node actor for all structural elements (Logs, Stone Masonry, Trusses, Bucks). Integrates with UStructuralIntegritySubsystem to propagate gravity & lateral …

- `virtual float ApplyDamage(float Amount, const FVector& Direction)` — Deals structural damage. Returns remaining structural health.
- `virtual void TriggerChaosCollapse(const FVector& Impulse = FVector::ZeroVector)` — Triggers dynamic Chaos Physics collapse of this building node
- `void AddSupportingNode(ABuildingNodeActor* Child)` — Adds a child node that rests on top of this node
- `void RemoveSupportingNode(ABuildingNodeActor* Child)` — Removes a child node
- `void AddSupportedByNode(ABuildingNodeActor* Parent)` — Sets parent nodes that physically support this node from below
- `float GetSelfWeight() const` — --- Structural Getters & Setters ---
- Ayrıca (açıklamasız): `ABuildingNodeActor`, `GetCurrentLoadKN`, `SetCurrentLoadKN`, `GetCompressiveCapacityKN`, `IsGrounded`, `SetGrounded`, `IsCollapsed`, `GetSupportingNodes`, `GetSupportedByNodes`

## BurnSubsystem.h

**UBurnSubsystem** (class) — Drives the fire of a world: every burning piece of wood burns down, heats the wood beside it (the closer, the faster; touching pieces catch in about the time a lighter …

- `void Advance(float Seconds)` — Burns the world for Seconds (in steps of a quarter second). The tick does this; the mechanics test calls it to skip time.
- Ayrıca (açıklamasız): `Get`, `Light`, `Warm`, `GetBurningCount`

## Campfire.h

**ACampfire** (class) — A campfire. Any piece of wood is fuel: it burns as long as the wood it holds (a whole log 10 minutes, a half 5, a quarter 2.5), so cutting a log up into firewood is just …

- `static float FuelSecondsOf(const ACarvableLog* Piece)` — Seconds Piece would burn.
- `static ACampfire* Build(UWorld* World, const FVector& Ground)` — A fire pit with a ring of stones on the ground at Ground, not lit until it has fuel.
- `bool AddFuel(ACarvableLog* Piece)` — Throws Piece on the fire: it is used up (a burning piece lights the fire). False if the fire is already full or the piece is part of a structure. The caller takes the …
- `bool IsBurning() const` — Fuel alone does not make a fire: it has to be lit (a lighter, or burning wood).
- `bool Ignite()` — Lights the fire if it has fuel.
- `bool ApplyHeat(float Seconds)` — Heat from a lighter: the fire catches after a couple of seconds.
- `void Burn(float Seconds)` — Burns for Seconds: fuel goes down, the light follows, wood in reach is scorched. Called every tick as well.
- Ayrıca (açıklamasız): `ACampfire`, `GetFuelSeconds`

## CarvableLog.h

**FJointCut** (struct) — A joint cut into a piece (a cup, a socket...), kept so the same cut can be made in a replacement.


**FCrossPlacement** (struct) — Where a log laid across another one goes, and the saddle notch that seats it (see ACarvableLog::ComputeCrossPlacement).


**ACarvableLog** (class)

- `static ACarvableLog* SpawnFromStaticMesh(UWorld* World, UStaticMesh* Mesh, const FTransform& Transform)` — Spawns a carvable copy of a raw log mesh (e.g. /Game/Building/Raw/SM_RawLog_C).
- `static ACarvableLog* SpawnLog(UWorld* World, const FTransform& Transform)` — A whole log as a tree gives it (the greybox log, six modules long), falling into place.
- `static ACarvableLog* SpawnFromPickup(APickupItem* Pickup)` — Puts a log lying on the ground on the bench: replaces the pickup with a carvable log of the same mesh, in place.
- `static bool GetLogAxis(const AActor* Log, FVector& OutCentre, FVector& OutAxis, float& OutRadius, float& OutHalfLength)` — Axis of a log lying in the world: a carvable log or a carryable log item. False for anything else.
- `static float GetComplementarySnapYaw(float ReferenceYaw, float LowerYaw, float DesiredYaw)` — Pick the complementary +45/-45 degree course in a shared reference frame, alternating above an existing diagonal log.
- `static bool ComputeComplementarySnapPlacement(const ACarvableLog* Lower, const FVector& Aim, const ACarvableLog* Upper, float Refe …` — Place a horizontal log above Lower at the shared-frame complementary 45 degree angle with a seated notch.
- `static bool ComputeCrossPlacement(const AActor* Lower, const FVector& Aim, const FVector& Viewer, const ACarvableLog* Upper, FCros …` — Laying a log (UpperMesh) square across Lower, aimed at Aim by a player standing at Viewer, like the corner of a log wall: it crosses near one end (an overhang past the …
- `static bool ComputeWallCornerPlacement(const ACarvableLog* Course, const ACarvableLog* Adjacent, const FVector& Aim, const ACarvab …` — A real log-wall corner. Course and Adjacent meet at one of their ends; Upper follows Adjacent's wall direction and seats over Course at that shared end. Unlike …
- `static bool ComputeCornerPlacement(const ACarvableLog* Lower, const FVector& Aim, const FVector& Viewer, const ACarvableLog* Upper …` — A log-cabin corner joint: Upper laid square across the END of Lower (aim within the last module). The joint sits half a module in from Lower's end and half a module in …
- `static ACarvableLog* FindBeamOnPost(const ACarvableLog* Post)` — An anchored, horizontal log lying across the top of Post (a beam on a post), if there is one.
- `static bool ComputeBracePlacement(const ACarvableLog* Post, const ACarvableLog* Piece, FTransform& OutTransform)` — A brace for Piece: from the side of Post up to the underside of the beam on its top at 45 degrees, running towards the middle of the beam. It needs a piece between two …
- `static bool ComputeFloorPlacement(const AActor* Lower, const FVector& Aim, const FVector& Viewer, const ACarvableLog* Piece, FTran …` — A split piece laid across a lying log as a floor board: flat face up, running away from Viewer over the log, its near end a little past it, resting on top. False if …
- `static void GetCrossNotchGuide(const FCrossPlacement& Placement, const ACarvableLog* Upper, TArray<TArray<FVector>>& OutLines, TAr …` — Rim of that saddle notch on the upper log, in its local space (for the dashed guide before it is placed).
- `static ACarvableLog* PlaceAcross(AActor* Lower, const FCrossPlacement& Placement, ACarvableLog* Upper)` — Lays Upper across Lower as computed: moves it there (off the shoulder), cuts its saddle notch and seats it. Both logs are anchored where they are (a lying log item is …
- `float GetDamage() const` — 0 sound .. 1 burnt through or broken (wood goes darker as it takes damage). From the Wood structure rules: above 0.6 the joints of a piece let go (it no longer holds or …
- `void SimulateMaterialEnvironment(float ElapsedDays, float AmbientTemperatureC, bool bRainExposed, float RainfallMm)` — Apply one bounded world-weather interval to this existing wood condition state.
- `float GetIgniteSeconds() const` — Seconds of full heat it takes this piece to catch: its radius times 0.4, between 2 and 20.
- `bool ApplyHeat(float Seconds)` — Heat from a lighter, a flame or a fire. True once the piece has caught (this call or before).
- `bool Ignite()` — Catches fire at once. False if it is charred, or too many pieces already burn (MaxBurning).
- `float GetBurnSecondsLeft() const` — Seconds of burning left, and the share of its fuel used so far (0..1).
- `void StepBurn(float Seconds)` — Called by UBurnSubsystem: burns for Seconds (fuel, damage, flames).
- `bool CoolStep(float Seconds)` — Called by UBurnSubsystem: heat fades when nothing heats the piece. Returns true while it is still warming.
- `bool IsTemporary() const` — Scaffolding: part of the building site only until the structure is done.
- `static bool CanReplace(const ACarvableLog* Old, const ACarvableLog* Replacement)` — A damaged piece in a structure can be swapped for another one of the same size (see ReplaceDamaged).
- `static bool ReplaceDamaged(ACarvableLog* Old, ACarvableLog* Replacement)` — Replacement takes Old's place, anchored, with the same joints cut into it. Old is released from the structure, damage and all, for the caller to pick up.
- `void Anchor()` — Part of a structure now: stays exactly where it is, no physics, also after further cuts.
- `void Release()` — Off the structure to be carried: no longer anchored, physics stays off (the shoulder takes it).
- `void LetFall()` — No longer held up: anchoring is dropped and physics takes over, so it tips or falls where it is.
- `ACarvableLog* SnapAt(float LocalX)` — A beam that cannot carry its own span: it lets go and snaps in two at LocalX, both halves left to physics.
- `const TArray<FVector2f>& GetCutSpans() const` — Joints cut into this log, as local X ranges.
- `ELogMotion GetMotion() const` — Loose (free to roll and slide), Carried (shoulder, cart) or Installed (part of a structure).
- `bool Push(const FVector& Velocity)` — A shove: a loose piece takes Velocity (cm/s) and rolls or slides until it rests. False for carried or installed pieces.
- `void AddImpactDamage(float ImpulseSize)` — A hard landing or hit hurts wood. ImpulseSize is the normal impulse of the hit (kg cm/s); small knocks do nothing.
- `static void HitEnemy(ARaidEnemy* Enemy, float ImpulseSize)` — A moving piece that strikes an enemy hurts it, by the impulse of the blow.
- `static bool ComputeSlopePlacement(const FVector& LowEnd, const FVector& Forward, float AngleDeg, const ACarvableLog* Piece, FTrans …` — A piece leaning at AngleDeg (10..80) with its lower end on the ground at LowEnd, rising in the direction of Forward (flattened): a ramp, a rafter, a stair stringer. …
- `ACarvableLog* CutOpening(float CentreX, int32 Modules, const FVector& PushTowards = FVector::ZeroVector)` — Cuts a gap Modules wide (an even number, at least 2) out of this piece, on the module lines nearest CentreX, leaving a piece on each side: a door or window opening in a …
- `const TArray<TWeakObjectPtr<ACarvableLog>>& GetLastOpeningOffcuts() const` — The 75 cm pieces the last CutOpening dropped (for tests and the showcase).
- `bool CutGroove(float SideAngleDeg)` — Cuts a channel along the whole log, opening towards SideAngleDeg (0 = top).
- `float GetVolume() const` — Wood in this piece (cm3); what it weighs on the shoulder.
- `static float GetShoulderCapacity()` — How much wood fits on the shoulder: one whole log (390 x 70 cm), or two halves, or six one-module pieces.
- `bool IsSplit() const` — Split lengthwise at some point (a half, a quarter): not a round log any more.
- `bool IsFullLog() const` — A whole carryable log: four modules (the 300 cm raw log), not split.
- `FBox GetLocalBox() const` — The piece's extent in its own frame.
- `void BeginCarry(USceneComponent* AttachTo, const FTransform& Relative)` — On the player's shoulder: no physics, no collision, attached to AttachTo at Relative.
- `void PlaceAt(const FTransform& Transform, bool bAnchorIt)` — Off the shoulder to Transform: anchored (a post) or left to settle where it lands.
- `FTransform GetLayTransform(const FVector& Ground, float Yaw) const` — Lying on the ground at Ground, its length along Yaw.
- `FTransform GetStandTransform(const FVector& Ground, float Yaw) const` — Standing upright as a post with its lower end on Ground.
- `int32 GetTriangleCount() const` — Triangles in the current mesh; grows with every cut, used by the mechanics demo to check a cut landed.
- `float GetWidth() const` — Width across the axis (local Y) of the piece as it is now: 35 cm for a round log, 24 for a plank.
- `void CutSeat(const FVector& WorldPoint, const FVector& WorldSide, float Width, float Depth)` — Cuts a flat seat Depth deep and Width wide across this lying log at WorldPoint, on the side facing WorldSide (up for a post standing on it, down for the top of a post …
- `static float FloorBoardSeatDepth(float BeamRadius)` — How deep a floor board is let into a beam of this radius.
- `void GetModuleLines(TArray<float>& OutX) const` — Local X of the grid lines that lie inside the log (not its ends).
- `void GetSnapPoints(ECarveTool Tool, float Width, TArray<float>& OutX) const` — Where an axe can make this cut on this log, all on the module grid. Joints (notch, round, flat) go at the corners, one module in from each end, and on the middle line of …
- `bool FindSnapPoint(ECarveTool Tool, float LocalX, float Width, float& OutX) const` — The snap point nearest LocalX; false when the log has none left for this tool.
- `bool IsSpanFree(float X0, float X1) const` — No earlier joint overlaps [X0, X1].
- `bool Notch(float LocalX, float SideAngleDeg, float CrossAngleDeg, float MateRadius, float Depth)` — Cuts a cup for a crossing log of MateRadius. SideAngleDeg turns around the log axis (0 = top, 180 = bottom); CrossAngleDeg is the crossing angle (90 = square, 0 = …
- `bool CutProfile(float LocalX, float SideAngleDeg, float Width, float Depth, bool bRound)` — Cuts a crosswise notch Width long (along the log) and Depth deep, opening toward SideAngleDeg (0 = top, 180 = bottom). Round leaves a circular side profile, flat a …
- `bool CutDovetail(float LocalX, float SideAngleDeg, float Width, float Depth)` — A dovetail socket: Width wide at the floor, 70% of that at the surface, Depth deep, through the whole log.
- `bool ShapeEnd(bool bMaxEnd, ECarveTool Shape)` — Works one end: Point (a stake), Chamfer (bevelled edge) or Trim (cut a pointed or chamfered end back square). Pointed and chamfered ends are only made on flat ends with …
- `int32 GetSplitCount() const` — 0 whole, 1 half, 2 quarter: how many times the log has been split lengthwise.
- `float GetFlatAngle() const` — Direction (in the YZ plane, as Split's angle) the flat face of a split piece looks out of the wood.
- `void GetProfileFrame(float LocalX, float SideAngleDeg, float Width, float Depth, bool bRound, FVector& OutCentre, FVector& OutOut, …` — Frame of a profile notch for previews: axis direction out of the log, across direction, and the cutter centre.
- `void GetCutGuide(ECarveTool Tool, float LocalX, float SideAngleDeg, float AngleDeg, float Width, float Depth, TArray<TArray<FVecto …` — Where a cut would meet the log's surface, as local-space polylines lying just above the bark: the rim of a notch, the ring of a cross cut, the two lines of a split. Used …
- `ACarvableLog* CrossCut(float LocalX)` — Saws across at LocalX; the part beyond becomes a new log.
- `ACarvableLog* Split(float AngleDeg)` — Splits along the axis on the plane at AngleDeg around X (0 = vertical); the other half becomes a new log.
- `void GetNotchFrame(float LocalX, float SideAngleDeg, float CrossAngleDeg, float MateRadius, float Depth, FVector& OutCentre, FVect …` — Where the cup's mating log axis runs (local space), for previews.
- Ayrıca (açıklamasız): `ACarvableLog`, `AddDamage`, `SetDamage`, `GetBurnState`, `IsBurning`, `IsCharred`, `GetHeatShare`, `SetTemporary`, `IsAnchored`, `IsCarried`, `GetLength`, `GetRadius`, `GetCenterX`, `GetDiameter`, `GetMinX`, `GetMaxX`, `GetEnd`, `GetGridOriginX`

## DayLoopSubsystem.h

**FTemperatureReading** (struct) — The air a player feels at a spot, and what made it so.

- Ayrıca (açıklamasız): `Total`

**UDayLoopSubsystem** (class) — The day the player has to get through (Frostpunk, The Long Dark): it reads the shared simulation clock (day, hour, the day's low and high), makes the climate cold, moves …

- `bool IsRaidDay(int32 Day) const` — Night of a raid: the day number is a multiple of RaidEveryNights.
- `static FString RegionNameAt(const FVector& Where)` — Region names and how much colder (or warmer) each is: rocks colder and windier, the river damp.
- `static int32 RaidSizeForDay(int32 Day)` — Raiders of a raid night: three plus one per day survived; more at once as the days go by.
- `void SetTimeForTest(int32 Day, float Hour)` — For the mechanics test: puts the clock at Hour of Day, then runs the day's rules once.
- `void SkipHours(float Hours)` — Moves the clock on by Hours (crossing midnight runs the day's simulation, so crops and animals go on).
- `bool IsRaidRunning() const` — For the mechanics test: the raid of the night that is running, if any.
- Ayrıca (açıklamasız): `Get`, `GetDay`, `GetHour`, `IsNight`, `GetNextRaidDay`, `RegionOffsetAt`, `GetTemperatureAt`, `RaidAtOnceForDay`

## FarmKit.h

**FarmKit** (namespace) — Farming on top of the simulation's crops and animals (PhaseT): a field is sown with seeds from the pack and grows with the days, its grain goes into the pack; animals …

- `UPlantDefinition* WildGrain(UObject* Outer)` — A quick greybox grain: ripe in about six days of the simulation's weather.
- `ACropFieldActor* MakeField(UWorld* World, const FVector& Ground)` — A bare field, two metres square, on the ground at Ground.
- `ACropFieldActor* MakeWildPatch(UWorld* World, const FVector& Ground)` — A patch of wild grain, already ripe: where the first seeds come from.
- `bool Sow(ACropFieldActor* Field, AForestCharacter* Player)` — Sows SeedsPerField seeds from the player's pack.
- `int32 Harvest(ACropFieldActor* Field, AForestCharacter* Player)` — A ripe field's grain into the pack, with some seed back; the field is bare again. Returns the grain.
- `AAnimalActor* SpawnAnimal(UWorld* World, const FVector& Location, EAnimalSpecies Species)` — A chicken or a sheep standing at Location, grown up.
- `int32 CollectProducts(AAnimalActor* Animal, AForestCharacter* Player)` — Eggs, milk and wool the animal has made, into the pack.
- `bool Butcher(AAnimalActor* Animal, AForestCharacter* Player)` — Killed with the axe: meat, hide (and wool from a sheep) into the pack; the animal is gone.
- `FString Eat(AForestCharacter* Player)` — Eats the most filling thing in the pack (meat cooked by a fire nearby fills more). Returns what was eaten.
- `bool MakeHideGarment(AForestCharacter* Player)` — Makes one piece of hide clothing if there are hides and room for more warmth.
- Ayrıca (açıklamasız): `AnimalOf`, `IsRipe`, `IsSown`

## FireStone.h

**AFireStone** (class) — One stone laid on the ground for a fire pit. Seven stones close together (a ring) become a fire pit; until then each is just a stone lying where it was put.

- `static AFireStone* Place(UWorld* World, const FVector& Ground)` — A stone on the ground at Ground.
- `static int32 CountNear(UWorld* World, const FVector& Where)` — Stones lying within ring reach of Where.
- `static ACampfire* TryForm(UWorld* World, const AFireStone* New)` — If StonesPerPit stones lie together around New, they are taken up and a fire pit is built in the middle of them. Null while the ring is not complete.
- Ayrıca (açıklamasız): `AFireStone`

## ForestCharacter.h

**AForestCharacter** (class) — First person survival character: chop trees, break rocks, pick up drops, craft simple materials and place stone blocks. Input is built in code so the prototype needs no …

- `UInventoryComponent* GetInventoryComponent() const` — Canonical stack inventory shared with storage, crafting and persistence.
- `void GetInventorySlots(TArray<TPair<EForestItem, int32>>& OutSlots) const` — The backpack as slots, in display order: one entry per stack.
- `USurvivalComponent* GetSurvivalForTest()` — For the mechanics test: the survival values, writable.
- `EForestItem GetCarriedItem() const` — The log in the player's hands (None when empty; TrunkLog while a trunk is dragged by rope).
- `bool IsWallTool() const` — The log wall planner is selected (shown as the first build option).
- `static float GetCarveDepthFraction(int32 Index)` — Round / flat notch depth as a share of the log's thickness: 1/6, 1/3, 1/2.
- `bool IsCarveSpotValid() const` — The log in sight still has a place for the selected tool.
- `bool CanPlaceAcross() const` — E would lay the carried log across the log in sight.
- `const FString& GetSocketLabel() const` — Socket placement: what it does, e.g. "yan yana (ön yüz)".
- `float GetDiagonalAngle() const` — Diagonal placement: its angle to the log below, +45 or -45.
- `bool IsPlaceSupported() const` — The piece would be held up where it is about to go (the structure graph says so); false: it would fall.
- `bool IsPlacingStanding() const` — R while carrying: lay pieces down or stand them up as posts.
- `bool IsPlacingSlope() const` — R again: a piece leaning at an angle, a ramp or rafter; the wheel changes the angle.
- `bool IsLighterHeld() const` — F held: the lighter heats what is in sight (a piece of wood or a fire pit).
- `void TakeEnemyHit(float Amount)` — An enemy hit on the player.
- `float GetCarryLoad() const` — Shoulder load, 1 = full (one whole log).
- `ECarryPlace PlaceForShowcase(ACarvableLog* Piece, AActor* Target, const FVector& Aim, const FVector& Normal, float ViewYaw, EShowc …` — Puts Piece down exactly as the player would: shoulders it, faces ViewYaw, aims at Aim on Target (or the ground), picks the placement the game would pick and places it. …
- `static const TCHAR* CarryPlaceName(ECarryPlace Place)` — Name of a placement for reports.
- `static bool IsEndTool(ECarveTool Tool)` — Tools that work an end of the log.
- `bool TestCarveHit(ACarvableLog* Log, ECarveTool Tool, const FVector& WorldPoint, const FVector& WorldNormal)` — For the mechanics test: one axe hit on a carvable log with Tool, as if aimed at WorldPoint on the face with WorldNormal, through the same aim and two-hit logic the …
- `bool TestPickUp(ACarvableLog* Piece)` — For the mechanics test: E on a piece (true if it went onto the shoulder).
- `bool TestPlace(bool bStanding, const FHitResult& Hit)` — For the mechanics test: a left click with the carried piece aimed at Hit (true if it was placed).
- `bool TestPlaceSlope(const FHitResult& Hit, float AngleDeg)` — For the mechanics test: the lean mode chosen (angle in degrees) and the carried piece placed where the ground is aimed at.
- `ACampfire* TestBuildFirePit(const FHitResult& Hit, bool bGiveStones)` — For the mechanics test: [C] with the stones given first when bGiveStones. Returns the pit, or null.
- `ACampfire* TestPlaceStone(const FHitResult& Hit, bool bGiveStone)` — For the mechanics test: [C] once, with a stone given first. Returns the fire pit if this stone completed the ring.
- `bool TestPush(AActor* Target, const FVector& Velocity)` — For the mechanics test: [X] on Target.
- `void TestToggleTemporary(ACarvableLog* Piece)` — For the mechanics test: [V] on a piece.
- `ACarvableLog* TestUnload(AHaulCart* Cart)` — For the mechanics test: [E] on a cart (takes the last piece off it onto the shoulder).
- `bool TestPickUpResource(AActor* Resource)` — For the mechanics test: [E] on a loose stone or stick (a small stone and a stick go into the pack, a building stone into the hands).
- `AStickStructure* PlantStakeAt(const FHitResult& Hit)` — [I] Kazık çubuk: one stick from the pack driven into the ground at Hit. Null without a stick.
- `void TestDropStone()` — For the mechanics test: puts a carried stone down, empties the pack of sticks, gives sticks.
- `int32 TestGiveItem(EForestItem Item, int32 Count)` — For the mechanics test and the farm code: items into and out of the pack.
- `bool TestCraft(uint8 Entry, const FHitResult& Hit)` — For the mechanics test: [I] entry Entry made (on Hit when it is placed on the ground).
- `void EatFood()` — [8]: eats the most filling food in the pack.
- `bool TestReadNote(ARemnantNote* Note)` — For the mechanics test: E on a note, drinking where the player stands, riding a rope from its near end.
- `int32 TestGather(ARawMaterialNode* Node)` — [E] on a raw material source: one gathering into the pack. Returns how many items went in.
- `bool HasRopeAnchor() const` — The rope line being strung: its first end is set.
- `bool IsHelpOpen() const` — [F1]: the full list of controls.
- `void TestLight(AActor* Target, float Seconds)` — For the mechanics test: the lighter held on Target for Seconds.
- `void MechanicsDemo()` — Console: MechanicsDemo. Same as admin + L, without needing admin mode.
- Ayrıca (açıklamasız): `AForestCharacter`, `GetItemCount`, `GetUsedSlots`, `IsInventoryOpen`, `IsAdminMode`, `GetCurrentTool`, `GetSurvival`, `GetCarriedCount`, `GetFocusedHarvestable`, `GetFocusedPickup`, `GetFocusedRawNode`, `IsInBuildMode`, `GetSelectedBuildItem`, `IsPlacementValid`, `IsWallStartSet`, `GetFocusedBlueprint`, `GetFocusedLogActor`, `GetFocusedStoneActor`, `GetFocusedCarvable`, `GetCarveTool`, `GetCarveAngle`, `GetCarveHits`, `GetCarveWidth`, `GetCarveDepth`, `GetCarveDepthIndex`, `GetCarryPlace`, `GetSlopeAngle`, `IsProfileTool`, `GetCarveToolName`, `GetMessage`, `GetMessageAge`, `IsCarryingStone`, `TestUseUpSticks`, `TestGiveSticks`, `TestRemoveItem`, `CraftEntryName`, `TestDrink`, `TestRideRope`, `IsRiding`, `IsStickMode`, `ToggleHelp`, `GetStickModeLabel`

## ForestOutline.h

**ForestOutline** (namespace) — Line drawings for build plans and previews, in the style of Sons of the Forest blueprints: round pieces get rings at both ends plus lines along their length, square …

- `FLinearColor ValidColor()` — Plan colour for a spot that can be built on, and for one that cannot.
- `FLinearColor CarveColor()` — Colour of carving previews.
- `FLinearColor CutColor()` — Red of the dashed cut guide on trees and logs.
- `void DrawDashed(ULineBatchComponent* Lines, const FTransform& Transform, const TArray<FVector>& Points, bool bClosed, const FLinea …` — A dashed line through local-space Points (closed back to the first point if bClosed), like a cut marked on bark.
- `void DrawPieceBox(ULineBatchComponent* Lines, EForestItem Item, const FBox& Box, const FTransform& Transform, const FLinearColor&  …` — Outline of a piece given its local box (a carvable log): round items as a cylinder along X, others as a box.
- `void DrawPiece(ULineBatchComponent* Lines, EForestItem Item, const UStaticMesh* Mesh, const FTransform& Transform, const FLinearCo …` — Outline of a build piece (log, plank, stone...) placed with Transform (scale included).
- `void DrawCylinder(ULineBatchComponent* Lines, const FTransform& Transform, int32 Axis, const FVector& Centre, float Radius, float  …` — A cylinder whose axis is local X (Axis = 0) or local Z (Axis = 2), centred on Centre in local space.
- `void DrawBox(ULineBatchComponent* Lines, const FTransform& Transform, const FBox& Box, const FLinearColor& Color)` — The twelve edges of a local-space box.
- Ayrıca (açıklamasız): `InvalidColor`

## ForestTypes.h

**ForestItems** (namespace)

- `const TArray<EForestItem>& GetAllItems()` — Items in the order they are shown in the HUD.
- `FName GetInventoryID(EForestItem Item)` — Stable item ID shared by the character inventory and modular InventoryCore.
- `bool IsLogItem(EForestItem Item)` — Any of the three log stages (trunk, big log, log): they nest when stacked.
- `bool IsCarriedItem(EForestItem Item)` — Items that never enter the backpack: they are carried or dragged in the world (the three log stages).
- `UStaticMesh* GetMesh(EForestItem Item, int32 Variant = 0)` — Mesh used when the item lies in the world. Variant picks between pack variations.
- `FVector GetItemScale(EForestItem Item)` — Scale the item is shown at in the world (pickups, placed pieces, wall plans). Lets a piece read bigger without touching its mesh asset.
- `float GetItemWeight(EForestItem Item)` — Approximate stack weight used by storage and transfer policies (kg per item).
- `UStaticMesh* GetLogStageMesh(int32 Stage)` — Big logs left by a felled tree. Stage 1 is the trunk, cutting it gives two stage 2 logs, cutting those gives the carryable Log item (stage 3).
- `FVector GetLogStageScale(int32 Stage)` — Scale for a big log stage: each stage is twice as long as the next so halves line up exactly.
- `UStaticMesh* GetDebrisMesh(EHarvestType Type, int32 Variant)` — Small cosmetic pieces thrown off when a tree or rock is hit.
- `UMaterialInterface* GetHitMarkMaterial(EHarvestType Type)` — Decal left behind by an axe (wood) or pickaxe (rock) hit.
- `UStaticMesh* GetToolMesh(EForestTool Tool)` — Mesh of the tool held in first person.
- `UMaterialInterface* GetBuildGhostMaterial()` — Translucent material for the build placement preview, tinted through the GhostColor parameter.
- Ayrıca (açıklamasız): `GetDisplayName`

## HarvestableActor.h

**AHarvestableActor** (class) — A tree, rock or big log lying on the ground that the player can hit. Trees topple away from the player and leave a stage 1 log; cutting a big log splits it in two (stage …

- `void InitFrom(const UStaticMeshComponent* Source, EHarvestType InType)` — Copies mesh and materials from a level placed actor and sizes health and drops from it.
- `void InitFromSkeletal(const USkeletalMeshComponent* Source)` — A skeletal tree (Megaplant): the skeletal mesh is shown as is, a capsule round the trunk takes the hits, and the whole actor tips over from its base like a static tree.
- `void InitFromStaticTree(const UStaticMeshComponent* Source)` — A level-placed static tree (e.g. KiteDemo) tagged "Fellable": the mesh is shown without collision, a capsule round the trunk takes the hits. Same simple felling as the …
- `void InitRawLog(int32 LengthCm, const FVector& Center, const FVector& Direction)` — A raw UnrealExport log (SM_RawLog_<length>, along X, pivot at the centre) lying on the ground. Trees leave these; hitting one splits it into two of half the length: 600 …
- `static UStaticMesh* LoadRawLog(int32 LengthCm)` — SM_RawLog_<length>; for a length without its own mesh, the 150 cm one (InitRawLog then scales it).
- `class ACarvableLog* ConvertToCarvable()` — Picking up a raw log: it becomes the building piece every assembly uses (ACarvableLog, same mesh and place, cut, notched, laid, stood and carried by its volume). This …
- `void InitFromStaticRock(const UStaticMeshComponent* Source)` — A level-placed rock tagged "Breakable" (Megascans). Its size gives its stone tier: 3 large (100 cm or more), 2 medium (40 cm or more), 1 building stone. The pickaxe …
- `void InitLooseStone(int32 Tier, UStaticMesh* StoneMesh, float Scale, const FVector& Impulse)` — A loose stone of Tier (1 small, 2 medium) thrown from where it is with Impulse; its physics stops after 4 s.
- `bool CarveHit()` — One chisel strike. After CarveHitsNeeded strikes a medium stone becomes a flat wall stone, a building stone a squared cobblestone (grey or brown) of its size, at least …
- `static TArray<int32> RawLogsForHeight(float TreeHeightCm)` — Raw logs a felled tree leaves, longest first: 18 m or taller -> 600 + 300, 10 m or taller -> 600, else 300.
- `static void PlayFellingSound(UWorld* World, const TCHAR* Name, const FVector& Location, float Volume = 1.f)` — Plays /Game/Audio/Felling/<Name> at Location with distance falloff (silent if the asset is missing).
- `void InitLog(int32 InStage, const FVector& Center, const FVector& Direction)` — Lays a big log of the given stage (1 or 2) on the ground, centred at Center along Direction.
- `static int32 LogsFromHeight(float TreeHeightCm)` — Whole logs (six modules, 390 cm) a tree of this height gives: 85% of the height is usable trunk, at most four. A trunk or a half trunk carries the number it still holds …
- `static void DropRockResources(UWorld* World, const FVector& Center, int32 Count)` — A broken rock: one building stone and Count - 1 small stones, lying around Center.
- `static void DropBranchSticks(UWorld* World, const FVector& Base, const FVector& Direction, float Length)` — A felled tree's branches: raw sticks along the fall line from Base.
- `bool ApplyHit(float Damage, const FHitResult& Hit, const FVector& HitDirection)` — Returns true when this hit felled the tree, broke the rock or split the log.
- `bool IsWood() const` — Trees and big logs take the axe, rocks the pickaxe.
- `EForestItem GetLogItem() const` — Inventory item a big log becomes when picked up.
- `void BeginDrag(const FVector& Holder)` — Dragging a big log: Grab picks the end nearest Holder as the lifted end.
- `void DragTo(const FVector& Front)` — Lifted end follows Front, the other end trails on the ground.
- `void EndDrag()` — Lays the log back on the ground where it is.
- `void BeginShoulder(USceneComponent* AttachTo, const FTransform& Relative, bool bCastShadow = false)` — Shouldering a big log: attached to the player's view, no collision.
- `void PlaceOnGround(const FVector& Center, const FVector& Direction)` — Off the shoulder, laid on the ground centred at Center along Direction.
- Ayrıca (açıklamasız): `AHarvestableActor`, `GetRawLogLength`, `IsCarvable`, `GetLogCount`, `GetTreeHeight`, `SetLogCount`, `IsAlive`, `IsTree`, `IsTrunk`, `GetHealthPercent`, `GetDisplayName`, `IsDragged`

## HaulCart.h

**AHaulCart** (class) — Something wood is loaded on: a cart, a sled or a rack. Pieces keep their volume on it (a load never exceeds the capacity in wood) and are Carried while on it; unloading …

- `static AHaulCart* Spawn(UWorld* World, const FVector& Ground, float Yaw, EHaulKind Kind)` — A cart, sled or rack standing on Ground, facing Yaw.
- `float GetCapacity() const` — Wood the thing holds (cm3): this many shoulder loads of one whole log each.
- `bool Load(ACarvableLog* Piece)` — Puts the piece on the load (off the shoulder). False if it does not fit.
- `ACarvableLog* Unload()` — Takes the last piece off and sets it down loose beside the cart; null if empty.
- `bool Shove(const FVector& Velocity)` — A push: carts and sleds glide a while and stop; a rack does not move.
- Ayrıca (açıklamasız): `AHaulCart`, `KindName`, `GetKind`, `GetLoadVolume`, `GetLoadCount`, `CanLoad`

## LogConstructionActor.h

**ALogConstructionActor** (class) — A physical, structural log piece based on the 15 technical engineering guides. Supports Underscribing (Chapter 8), 6% Settling rule (Alaska Guide), Kessler shear …

- `void InitializeLog(EForestItem InItemType, ELogMoistureState InMoistureState = ELogMoistureState::Green)` — Initializes the log with a specific type and mesh
- `float ApplyStructuralDamage(float DamageAmount, const FVector& HitLocation, const FVector& HitDirection)` — Applies damage from axe/saw. If health reaches 0, triggers structural failure
- `virtual void TriggerChaosCollapse(const FVector& Impulse = FVector::ZeroVector) override` — Triggers dynamic Chaos RigidBody physics so the log falls, rolls and collides
- `void ApplyChinking()` — Applies chinking/daubing sealant between logs
- `void ApplyInteriorPlaster()` — Applies interior plaster / lime daubing
- `void UpdateVisualColor()` — Updates material tint color as value increases (Unchinked -> Chinked -> Plastered)
- `float GetRequiredSettlingSpace() const` — Calculates required settling space based on the Alaska Guide 6% rule (3/4 inch per foot)
- `float GetEffectiveRValue() const` — Effective thermal R-value based on BEES standard and chinking seal
- `float CalculateShearCapacity() const` — Lateral shear capacity based on Kessler & Hahney LBN 32 formula
- `EForestItem GetItemType() const` — --- Getters & Setters ---
- Ayrıca (açıklamasız): `ALogConstructionActor`, `IsChinked`, `IsPlastered`, `IsSimulatingPhysics`

## LogShowcase.h

**ALogShowcase** (class) — Builds, when play starts, a row of stations that show what logs can be made into, each built piece by piece with the player's own placement …

- `FString BuildAll()` — Builds every station now (also called by itself shortly after play starts). Returns the report.
- Ayrıca (açıklamasız): `ALogShowcase`

## LogWallBlueprint.h

**ALogWallBlueprint** (class) — A planned log wall, Sons of the Forest style: a see-through plan of stacked logs that the player fills one log at a time, bottom row first. Walls started from another …

- `static bool ComputeSlots(const FVector& Start, const FVector& End, float Phase, bool bFitExactly, TArray<FTransform>& OutSlots, FV …` — Lays logs from Start towards End. With bFitExactly the logs are stretched slightly so the wall ends exactly at End (used when closing onto another wall). OutEnd receives …
- `bool AddLog()` — Puts the next log into the plan. Returns false when the wall is already complete.
- `void ApplyChinking()` — Applies chinking/daubing between the logs in this wall
- `void CollapseWall(const FVector& ImpactDirection = FVector::ZeroVector)` — Triggers Chaos physics collapse of all placed logs in this wall
- Ayrıca (açıklamasız): `ALogWallBlueprint`, `Init`, `IsComplete`, `GetFilled`, `GetTotal`, `GetStart`, `GetEnd`, `GetPhase`, `IsChinked`, `GetGhosts`

## LooseResources.h

**LooseResources** (namespace) — Physical stones and sticks lying in the world, as a broken rock or a felled tree leaves them (Sons of the Forest: the resource is a thing you pick up, not a number). A …

- `AStoneActor* SpawnStone(UWorld* World, const FVector& Location, bool bBuildingStone)` — A small stone (pocketable, for fire rings) or a building stone (heavy, carried and placed).
- `AStickActor* SpawnStick(UWorld* World, const FVector& Location, const FRotator& Rotation)` — A raw stick, about half a metre long.
- `bool IsPocketStone(const AStoneActor* Stone)` — A stone light enough for one hand goes into the pack rather than into the hands.

## MechanicsDemo.h

**FDemoGoalLine** (struct) — One line of the guided demo checklist, as the HUD shows it.


**AMechanicsDemo** (class) — The wood chain: fell a tree, split the trunk into two big logs, split those into four carryable logs, then carve a raw log with every carving tool. Guided (default): the …

- `static void Start(UWorld* World, bool bAuto, bool bQuitWhenDone = false)` — Starts (or restarts) the demo in this world; bAuto plays it by itself, for tests.
- `bool IsGuiding() const` — Guided mode is running: the HUD draws the checklist.
- `void GetGuide(TArray<FDemoGoalLine>& OutLines, FString& OutHint) const` — Checklist lines and the hint for the current step.
- Ayrıca (açıklamasız): `AMechanicsDemo`

## OpeningBuckActor.h

**AOpeningBuckActor** (class) — Window and door buck framework based on Alaska Log Building Guide (Section 8). Manages Log Rough Opening (LRO) vs Commercial Rough Opening (CRO), 6% (3/4" per foot) …

- `void InitializeBuck(EOpeningType InType, float WidthCm, float HeightCm, bool bIsGreenLogWall = true)` — Initializes buck dimensions and opening type
- `float CalculateSettlingAllowance() const` — Calculates required vertical settling clearance space above the header (in cm)
- `float GetLogRoughOpeningHeight() const` — Total LRO height required including settling head clearance
- `float GetLogRoughOpeningWidth() const` — Width required for log opening including keyway splines
- `EOpeningType GetOpeningType() const` — --- Getters ---
- Ayrıca (açıklamasız): `AOpeningBuckActor`, `GetClearOpeningWidth`, `GetClearOpeningHeight`, `HasKeywaySpline`

## PickupItem.h

**APickupItem** (class) — A physics item lying in the world. Pickable items go to the player's inventory; cosmetic ones (bark, chips, pebbles) just fall and disappear. Physics is switched off …

- `void BeginCarry(USceneComponent* AttachTo, const FTransform& Relative)` — Held by the player: no physics or collision while attached.
- `void Drop(const FTransform& Transform)` — Back into the world at Transform, falling under physics until it settles.
- Ayrıca (açıklamasız): `APickupItem`, `Spawn`, `SpawnCosmetic`, `IsPickable`, `GetItem`, `GetCount`

## RaidEnemy.h

**ARaidEnemy** (class) — A raider. It walks to the player, or to the nearest installed wood when it has no one in reach, hits what is in its way (a wall is damaged like by a fire: past 60% the …

- `void SetTarget(AActor* Target)` — What the enemy goes for instead of looking for a target.
- `void StepAI(float Dt)` — Thinks and moves for Dt seconds. The tick does this; the mechanics test calls it to skip time.
- `void TakeHit(float Amount)` — Hurts the enemy; destroyed at zero.
- Ayrıca (açıklamasız): `ARaidEnemy`, `IsAlive`

**RaidKit** (struct) — Starts and stops a raid with the plan this game ships with (a night with one wave of raiders).

- `static bool StartDefaultRaid(UWorld* World)` — Builds the plan in code and starts it for the player in the world. False if it could not start.
- `static bool StartRaid(UWorld* World, FName EventID, int32 Total, int32 AtOnce)` — One wave of Total raiders, at most AtOnce of them at a time, under the event ID (a raid night).
- `static void StopRaid(UWorld* World)` — Cancels the active raid and removes only actors owned by its Raid Director.
- Ayrıca (açıklamasız): `CountEnemies`

## RawLogShapes.h

**RawLogShapes** (namespace) — Working the raw UnrealExport logs with the axe, as finished shapes rather than free carving. Every log is a length (600 / 300 / 150 / 75 cm, the 75 cm module) and a …

- `FShape FromMesh(const UStaticMesh* Mesh)` — The shape of a raw-log mesh from its name (SM_RawLog_300_Half...); invalid for anything else.
- `FShape FromLog(const ACarvableLog* Log)` — The shape of a building piece made from a raw-log mesh.
- `EAction ClassifyHit(const ACarvableLog* Log, const FHitResult& Hit, bool bChannelMode)` — What a hit at Hit does: an end face splits, the middle cuts across, the sides are hewn; bChannelMode: channel.
- `bool CanApply(const FShape& Shape, EAction Action, FString& OutWhy)` — Whether Action can be done to Shape; when not, OutWhy says why (shown to the player).
- `int32 HitsFor(EAction Action)` — Axe hits it takes.
- `bool Apply(ACarvableLog* Log, EAction Action, FString& OutMessage)` — Replaces Log by the pieces Action makes, where it lies; OutMessage describes the result. False if it cannot.
- `FString Describe(const FShape& Shape)` — "300 cm yarım kütük".
- `FString ActionName(EAction Action)` — "Enine kesiliyor".
- Ayrıca (açıklamasız): `LoadMesh`, `ProfileName`

**FShape** (struct)

- Ayrıca (açıklamasız): `IsValid`

## RawMaterialNode.h

**ARawMaterialNode** (class) — A source of raw materials gathered by hand with [E]: fibre from a bush, clay from a bank, flint from an outcrop. Raw materials are not building pieces; they bind and …

- `static TArray<FRawMaterialDrop> DefaultDrops(ERawSourceKind Kind)` — The drops a source of this kind gives unless set by hand.
- `int32 Harvest(AForestCharacter* Player, FString& OutGot)` — One gathering into the player's pack. Returns how many items went in; 0 when empty or the pack is full.
- `void Regrow()` — Back to a full supply (the respawn timer, or the mechanics test).
- `float GetSupplyRatio() const` — 1 when full, 0 when used up; always 1 for an endless source.
- `FString GetPrompt() const` — "[E] Topla: Bitki Lifi" text for the HUD.
- Ayrıca (açıklamasız): `ARawMaterialNode`, `Spawn`, `KindName`, `IsDepleted`, `GetRegrowSecondsLeft`

## StickStructure.h

**AStickStructure** (class) — One sharpened stick driven into the ground (Sons of the Forest's physical stick building: nothing is placed as a finished prefab, every stick is put in by hand). What …

- `static AStickStructure* Plant(UWorld* World, const FVector& Ground, float Yaw)` — A stake driven into the ground at Ground, standing up.
- `void AddDamage(float Amount)` — Wears it down; it breaks (is destroyed) at 1.
- `bool ApplyHeat(float Seconds)` — The lighter's flame: after IgniteSeconds the stake burns as a torch. True once it burns.
- Ayrıca (açıklamasız): `AStickStructure`, `GetDamage`, `IsLitTorch`, `GetBurnSecondsLeft`

## StoneMasonryActor.h

**AStoneMasonryActor** (class) — A stone foundation or wall actor implementing the 8 Dry Stone Walling principles from StoneworkPDF24: Grading, Throughstones, Hearting, Batter, Coping, and Crossing …

- `void InitializeStone(EMasonryType InType, bool bWithThroughStones = true, bool bWithHearting = true)` — Initializes stone foundation with specific bond and geometry
- `void ApplyMortar()` — Adds mortar to a dry stone wall, converting it to mortared masonry
- `void UpdateVisualColor()` — Updates material tint color as value increases (DryStone -> MortaredStone)
- `virtual float ApplyDamage(float DamageAmount, const FVector& HitDirection) override` — Deals damage to foundation stone. If damaged enough, foundation crumbles and drops upper structure
- `virtual void TriggerChaosCollapse(const FVector& Impulse = FVector::ZeroVector) override` — Triggers Chaos physics collapse of this stone block
- `bool ValidateDryStoneRules() const` — Checks if the wall satisfies the 8 Dry Stone principles
- `float GetCompressiveCapacity() const` — Maximum vertical compressive load this foundation can carry (in kN)
- `EMasonryType GetMasonryType() const` — --- Getters ---
- Ayrıca (açıklamasız): `AStoneMasonryActor`, `HasThroughStones`, `HasHearting`

## StructuralIntegritySubsystem.h

**UStructuralIntegritySubsystem** (class) — World subsystem managing structural integrity, load distribution, and cascading collapse simulations for log, stone, and plaster constructions.

- `void RegisterNode(ABuildingNodeActor* Node)` — Registers a building node with the structural graph
- `void UnregisterNode(ABuildingNodeActor* Node)` — Unregisters a building node from the graph
- `void RecalculateLoadNetwork()` — Recomputes support relationships and propagates vertical loads across the network
- `void TriggerCascadeFailure(ABuildingNodeActor* OriginNode, const FVector& InitialImpulse = FVector::ZeroVector)` — Propagates catastrophic failure upwards from a failed node. Any nodes resting on top that have no remaining grounded support path will collapse.
- `bool HasPathToGround(ABuildingNodeActor* Node, TSet<ABuildingNodeActor*>& Visited) const` — Checks if a node has a valid structural path to the ground / bedrock
- Ayrıca (açıklamasız): `UStructuralIntegritySubsystem`

## StructureLink.h

**StructureLink** (namespace) — Bridge between the carvable logs in the world and the structure graph of Wood/WoodStructureLogic. Every anchored log (post, corner log, beam) is a node: its real ends, …

- `WoodStructure::FNode MakeNode(const ACarvableLog* Log, int32 Id, const FTransform* Transform = nullptr)` — The graph node for Log, as it is or as it would be at Transform.
- `TSet<int32> Evaluate(TArray<WoodStructure::FNode>& Nodes, const TArray<WoodStructure::FEdge>& Edges, TMap<int32, float>* OutSnaps  …` — Support with the beam rules on top of the graph: a beam that is not lying on the ground holds only if its supports (posts, braces, the logs under it) are not further …
- `const TArray<FFeatureInfo>& Features(UWorld* World)` — What the standing pieces make up. Worked out again only after the structure has changed (Settle, damage, new pieces).
- `FString NameOf(EWoodFeatureKind Kind)` — Turkish name of a kind of structure.
- `FString DescribeFeaturesOf(UWorld* World, const ACarvableLog* Log)` — What Log is part of, e.g. "Platform, Korkuluk" (empty if it stands alone).
- `void MarkChanged()` — Something in the structure changed: the next Features() works it out afresh.
- `FString Describe(UWorld* World)` — Every standing piece with its role, whether it rests on the ground, and what holds it: for reading what the graph sees.
- `int32 Settle(UWorld* World)` — Lets every anchored piece that has no path to the ground fall. Returns how many fell.
- `bool WouldBeSupported(UWorld* World, const ACarvableLog* Piece, const FTransform& Transform)` — Would Piece, anchored at Transform among the pieces already standing, be held up?

**FFeatureInfo** (struct) — A walls, platform, door frame... the standing pieces make up (see WoodStructure::AnalyzeStructure).


## StructureManager.h

**AStructureManager** (class) — Holds every piece the player places. Each item type gets one instanced mesh, so a whole wall or log cabin costs a handful of draw calls.

- `const UInstancedStaticMeshComponent* FindPieces(EForestItem Item) const` — Placed pieces of one kind, or null if none were placed yet.
- Ayrıca (açıklamasız): `AStructureManager`, `AddPiece`

## SurvivalGuideSubsystem.h

**USurvivalGuideSubsystem** (class) — The survival demo: a checklist through every pillar of the game (fire, cold, food, farming, animals, clothes, ropes, bridges, notes, defences, the first raid night), …

- Ayrıca (açıklamasız): `Get`, `IsActive`, `GetLines`, `GetDoneCount`, `GetGoalCount`

## TreeFellingSubsystem.h

**UTreeFellingSubsystem** (class) — Makes level trees fellable in a game world. On begin play every SkeletalMeshActor (Megaplant) and StaticMeshActor (e.g. KiteDemo) tagged "Fellable" becomes an …

- `int32 ConvertFellableTrees()` — Converts the tagged skeletal trees of the world; returns how many. Safe to call twice.
- `bool ChopFrom(AActor* Chopper)` — One axe swing from Chopper towards its facing; true when something was hit. Also used by tests.
- `bool CarveFrom(AActor* Chopper)` — One chisel strike from Chopper on the stone in front of it; true when a carvable stone was hit. Right mouse.
- `int32 GetConvertedCount() const` — Trees converted in this world (alive or felled).
- Ayrıca (açıklamasız): `TEXT`

## Wood/WoodCarrierComponent.h

**UWoodCarrierComponent** (class)

- `FVector StackStep = FVector(0.f, 0.f, 12.f)` — Her ek parça için ofsete eklenen kayma.
- `void SetCarryAttachComponent(USceneComponent* InAttach)` — Taşıma noktasını ayarla (ör. karakter mesh'i veya omuz bileşeni).
- `UWoodPieceComponent* FindPieceInView(FVector Start, FVector Direction, float Distance = 300.f, float Radius = 15.f) const` — Bakış yönünde küre taramasıyla kütük bul.
- `bool DropLast()` — En son alınanı bırak.
- Ayrıca (açıklamasız): `UWoodCarrierComponent`, `FVector`, `GetCarriedLiters`, `CanCarry`, `GetCarried`, `TryPickUp`, `DropPiece`, `DropAll`, `ThrowLast`

## Wood/WoodCutLibrary.h

**UWoodCutLibrary** (class)

- `static int32 SnapToModuleLine(const FWoodPieceData& Data, float DistanceFromEndACm)` — Mekanik 17: A ucundan mesafeyi en yakın modül çizgisine yuvarlar (1..N-1).
- `static FWoodCutResult CrossCut(const FWoodPieceData& Data, int32 ModuleLine)` — Mekanik 16, 18: enine kesim.
- `static FWoodCutResult SplitLengthwise(const FWoodPieceData& Data)` — Mekanik 19, 20: yuvarlak kütüğü iki yarım kütüğe ayır.
- `static FWoodCutResult SplitQuarter(const FWoodPieceData& Data)` — Mekanik 22: yarım kütüğü iki çeyrek parçaya ayır.
- `static FWoodCutResult MakePlank(const FWoodPieceData& Data)` — Mekanik 21: yarım veya çeyrek parçayı düz tahtaya çevir.
- `static FWoodCutResult TrimEnd(const FWoodPieceData& Data, bool bEndA)` — Mekanik 23: sivri/pahlı ucu düzelt.
- `static FWoodCutResult AddNotch(const FWoodPieceData& Data, int32 ModuleLine, EWoodJointType JointType)` — Mekanik 24-27, 29: çentik aç. ModuleLine 0..N.
- `static FWoodCutResult CutGroove(const FWoodPieceData& Data)` — Mekanik 28: boyuna oluk.
- `static FWoodCutResult SharpenEnd(const FWoodPieceData& Data, bool bEndA)` — Mekanik 30: ucu sivriltme.
- `static FWoodCutResult ChamferEnd(const FWoodPieceData& Data, bool bEndA)` — Mekanik 31: pah kırma.
- `static FWoodCutResult MakeFirewood(const FWoodPieceData& Data)` — Mekanik 32: ateşlik odun. Her modül için kesite göre 4 / 2 / 1 çeyrek parça üretir.

## Wood/WoodCutterComponent.h

**UWoodCutterComponent** (class) — UWoodPieceComponent ile aynı aktöre eklenir. Kesim kuralları UWoodCutLibrary'de; burada sadece sonucu dünyaya uygular. Varsayılan görsel: parçanın ölçeği kesim oranında …

- `FVector LocalLengthAxis = FVector(1.f, 0.f, 0.f)` — Kütüğün uzun ekseni (bileşen yerel uzayı). Varsayılan X.
- `FVector LocalLateralAxis = FVector(0.f, 1.f, 0.f)` — Yatay eksen: yuvarlak kütük önce bu eksende yarılır. Varsayılan Y. Dikey eksen = Length x Lateral.
- `TArray<FVector> GetCutLineWorldPositions() const` — Tüm modül çizgilerinin (1..N-1) dünya konumları: kesim çizgilerini göstermek için.
- `int32 FindNearestCutLine(FVector WorldPoint) const` — Mekanik 17: dünya noktasına en yakın modül çizgisi.
- `void ReshapeVisual(UWoodPieceComponent* Piece, const FWoodPieceData& NewData, const FWoodSlice& Slice)` — Her sonuç parçası için çağrılır (kesilen parçanın kendisi dahil). Gerçek geometriyi burada üret; varsayılan implementasyon boştur.
- Ayrıca (açıklamasız): `UWoodCutterComponent`

## Wood/WoodPieceComponent.h

**UWoodPieceComponent** (class) — Herhangi bir aktöre eklenir; o aktörün kök primitive'ini kütük olarak yönetir. Mekanikler: 4 alma, 5 bırakma, 6 düşürme, 7 yuvarlama, 8 sürükleme, 9 itme, 10 çarpışma, …

- `bool PickUp(USceneComponent* AttachTo, FName Socket = NAME_None)` — Mekanik 4
- `bool Drop()` — Mekanik 5
- `bool ThrowPiece(FVector Velocity)` — Mekanik 6 / 74: hızla bırak ya da fırlat.
- `void Roll(FVector Direction, float Strength)` — Mekanik 7: yönde yuvarla.
- `void DragToward(FVector TargetLocation)` — Mekanik 8: hedefe doğru yerde çek.
- `void Push(FVector Impulse)` — Mekanik 9: it.
- `bool Install(AActor* Structure, USceneComponent* AttachTo)` — Yapıya bağla (Installed).
- `bool Uninstall()` — Mekanik 56: yapıdan sök, serbest kütük yap.
- Ayrıca (açıklamasız): `UWoodPieceComponent`, `GetState`, `GetInstalledOn`, `GetPrimitive`

## Wood/WoodStructureAnalysis.h

**FFeature** (struct)

- Ayrıca (açıklamasız): `açıklıklarda`

## Wood/WoodStructureGeometry.h

**Geo** (namespace)

- `inline double RawParam(const FVector& P, const FVector& A, const FVector& B)` — Noktanın doğru üzerindeki ham parametresi (kelepçesiz): 0 = A, 1 = B.
- `inline void ClosestSegSeg(const FVector& P1, const FVector& Q1, const FVector& P2, const FVector& Q2, double& OutS, double& OutT)` — İki doğru parçasının en yakın noktalarının parametreleri (Ericson, Real-Time Collision Detection 5.1.9).
- Ayrıca (açıklamasız): `ClampD`, `PointAt`, `DistToSegment`

## Wood/WoodStructureLogic.h

**FNode** (struct) — Yapı grafiğindeki bir parça: iki uç noktası olan kalın bir çubuk.

- Ayrıca (açıklamasız): `ucu`, `yıkık`, `Length`

**FEdge** (struct) — İki parça arasındaki bağlantı. Tek yönlüyse (bOneWay) A taşınan, B taşıyan parçadır. Çift yönlüyse (aynı yükseklikte uç uca) yön yoktur; ikisi birbirini destekleyebilir.


**FSnap** (struct) — Snap önerisi: Geom parçası Offset kadar kaydırılırsa hedefe oturur.


## Wood/WoodStructureSubsystem.h

**UWoodStructureSubsystem** (class) — Dünya başına bir tane. Sadece Game / PIE dünyalarında oluşur. Pivot varsayımı: parçanın pivotu uzunluğun ortasında, uzun eksen WoodCutterComponent::LocalLengthAxis …

- `FWoodSnapResult FindSnap(UWoodPieceComponent* Piece, const FTransform& Proposed)` — Mekanik 53, 12: önerilen konumu snap + zemin ile düzeltir. Hayalet önizleme için her karede çağrılabilir.
- `bool InstallPiece(UWoodPieceComponent* Piece, const FWoodSnapResult& Snap, AActor* Structure = nullptr)` — Snap sonucuna göre kur. Structure verilirse parça o aktörün köküne bağlanır.
- `bool PlaceCarried(UWoodCarrierComponent* Carrier, UWoodPieceComponent* Piece, const FTransform& Proposed, AActor* Structure = null …` — Mekanik 12: omuzdaki kütüğü istenen konuma yerleştir (snap + bırak + kur).
- `bool RemovePiece(UWoodPieceComponent* Piece, bool bForce = false)` — Mekanik 55-57: parçayı yapıdan sök. Taşıdığı parça varsa bForce olmadan reddeder.
- `TArray<UWoodPieceComponent*> PreviewRemoval(const UWoodPieceComponent* Piece) const` — Mekanik 55, 58: bu parça sökülse hangi parçalar desteğini kaybeder (UI vurgusu için).
- `bool IsPieceSupported(const UWoodPieceComponent* Piece) const` — Mekanik 54
- `EWoodOrientation GetPieceOrientation(const UWoodPieceComponent* Piece) const` — Parçanın mevcut dünya yönü.
- Ayrıca (açıklamasız): `IsPieceInstalledHere`, `GetConnectedPieces`, `GetUnsupportedPieces`, `GetInstalledCount`

## Wood/WoodStructureTypes.h

**FWoodSnapResult** (struct) — FindSnap sonucu: hayalet önizleme ve InstallPiece bununla çalışır.


## Wood/WoodTypes.h

**FWoodNotch** (struct) — Modül çizgisi üzerindeki çentik. ModuleLine 0..6 (0 = A ucu, 6 = tam boy).


**FWoodPieceData** (struct) — Her kütük parçasının taşıdığı veri (bölüm 2).

- `float GetVolumeLiters() const` — Yaklaşık hacim (litre); taşıma kapasitesi bununla hesaplanır.
- `float GetLateralCm() const` — Kesit boyutları. Kural: Yatay (Lateral) eksen önce yarılır, Dikey (Vertical) eksen sonra.
- Ayrıca (açıklamasız): `GetModuleLengthCm`, `GetLengthCm`, `GetVerticalCm`

**FWoodSlice** (struct) — Bir kesim çıktısının, kesimden önceki parçanın kutusu içindeki yeri. S: uzunluk boyunca 0..1 (A ucundan B ucuna). L ve V: -0.5..0.5 (merkezden). Varsayılan değer = …


## WorldFeatures.h

**ARiverStrip** (class) — A river: wading through it is slow and the water is cold (Death Stranding: the world is hard to cross until you build across it). Logs laid over it as a bridge keep the …

- `static ARiverStrip* Spawn(UWorld* World, const FVector& Center, const FVector2D& HalfSize)` — A strip of water HalfSize (x, y) around Center, its surface at the ground's height.
- `bool Contains(const FVector& Feet) const` — Standing in the water: inside the strip with the feet at most a little above its surface.
- Ayrıca (açıklamasız): `ARiverStrip`, `IsInWater`

**ARopeLine** (class) — A rope strung between two points: ride along it to the other end (up a cliff, over a river). Each rope is a ropeway in the shared route graph …

- `static ARopeLine* Span(UWorld* World, const FVector& A, const FVector& B)` — A rope from A to B; null if it would be too short or too long.
- `bool EndNear(const FVector& Where, bool& bOutEndB) const` — The end within reach of Where (true: B), if there is one.
- Ayrıca (açıklamasız): `ARopeLine`, `GetA`, `GetB`, `GetLinkKey`, `GetNodeKey`, `CountAll`

**ARemnantNote** (class) — A remnant of the people who were here before: a note on a stake, read with E (Subnautica: the story is told by what is left in the world, never by a cutscene).

- Ayrıca (açıklamasız): `ARemnantNote`, `Place`

**ACliffBlock** (class) — A block of rock rising out of the ground: too high to climb without a rope.

- Ayrıca (açıklamasız): `ACliffBlock`, `Spawn`, `GetTopZ`

## Açıklaması eksik fonksiyonlar

203 / 511 fonksiyonun açıklaması yok. Yukarıda sınıfların altında "Ayrıca" satırlarında isimleriyle listeli; isimleri kendini açıklayan getter fonksiyonları için sorun değil.

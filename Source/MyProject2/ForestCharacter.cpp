// Copyright Epic Games, Inc. All Rights Reserved.

#include "ForestCharacter.h"
#include "RawLogShapes.h"
#include "InventoryCore/InventoryComponent.h"
#include "InventoryCore/InventoryItem.h"
#include "StickPhaseS1_Core/StickActor.h"
#include "StickCore/StickDefinition.h"
#include "StoneCore/StoneDefinition.h"
#include "Core/StructuralMaterialTypes.h"
#include "UObject/SoftObjectPath.h"
#include "HarvestableActor.h"
#include "LogWallBlueprint.h"
#include "LogConstructionActor.h"
#include "StoneMasonryActor.h"
#include "OpeningBuckActor.h"
#include "PickupItem.h"
#include "ForestOutline.h"
#include "StructureLink.h"
#include "Campfire.h"
#include "HaulCart.h"
#include "FireStone.h"
#include "RaidEnemy.h"
#include "DayLoopSubsystem.h"
#include "LooseResources.h"
#include "StickStructure.h"
#include "FarmKit.h"
#include "RawMaterialNode.h"
#include "WorldFeatures.h"
#include "ScenarioSubsystem.h"
#include "WorldCore/RegionLogisticsSubsystem.h"
#include "PhaseT2_Plant/CropFieldActor.h"
#include "PhaseT6_Animal/AnimalActor.h"
#include "Components/LineBatchComponent.h"
#include "MechanicsDemo.h"
#include "StructureManager.h"
#include "SurvivalComponent.h"
#include "WorldPhaseW1_Chunk/ChunkManager.h"
#include "CableComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "StoneCore/StoneDefinition.h"
#include "StonePhaseB_Placement/StoneCarryComponent.h"
#include "StonePhaseB_Placement/StonePlacementComponent.h"

namespace
{
	/** Bounds of a piece at the scale it is shown in the world. */
	FBox ScaledBox(const UStaticMesh* Mesh, EForestItem Item)
	{
		const FBox Box = Mesh->GetBoundingBox();
		const FVector Scale = ForestItems::GetItemScale(Item);
		return FBox(Box.Min * Scale, Box.Max * Scale);
	}

	/** World centre of a placed piece; LocalBox already carries the piece scale. */
	FVector PieceCenter(const FTransform& Transform, const FBox& LocalBox)
	{
		return Transform.GetLocation() + Transform.GetRotation().RotateVector(LocalBox.GetCenter());
	}

	/** True if one of the pieces already sits (nearly) at this centre. */
	bool IsSlotTaken(const UInstancedStaticMeshComponent* Pieces, const FVector& Center, const FBox& LocalBox)
	{
		const float Taken = FMath::Max(10.f, LocalBox.GetSize().GetMin() * 0.3f);
		for (const int32 Index : Pieces->GetInstancesOverlappingSphere(Center, Taken, /*bSphereInWorldSpace*/ true))
		{
			FTransform Instance;
			Pieces->GetInstanceTransform(Index, Instance, /*bWorldSpace*/ true);
			if (FVector::Dist(PieceCenter(Instance, LocalBox), Center) < Taken)
			{
				return true;
			}
		}
		return false;
	}
}

AForestCharacter::AForestCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(40.f, 92.f);
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->JumpZVelocity = 420.f;
	GetCharacterMovement()->AirControl = 0.3f;

	Survival = CreateDefaultSubobject<USurvivalComponent>(TEXT("Survival"));
	WorldChunks = CreateDefaultSubobject<UChunkManager>(TEXT("WorldChunks"));
	WorldChunks->ActivationRadius = 256.f; // metres: 4 chunks (64 m) each way
	CoreInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("CoreInventory"));
	CoreInventory->MaxSlots = InventorySlots;
	CoreInventory->bIgnoreWeight = true; // Preserve the playable character's existing 24-slot rule.
	StoneCarry = CreateDefaultSubobject<UStoneCarryComponent>(TEXT("StoneCarry"));
	StonePlacement = CreateDefaultSubobject<UStonePlacementComponent>(TEXT("StonePlacement"));

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, 70.f));
	Camera->bUsePawnControlRotation = true;
	Camera->SetFieldOfView(BaseFOV);

	// Placement ghost lives in world space so it does not follow the camera.
	BuildPreview = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildPreview"));
	BuildPreview->SetupAttachment(RootComponent);
	BuildPreview->SetUsingAbsoluteLocation(true);
	BuildPreview->SetUsingAbsoluteRotation(true);
	BuildPreview->SetUsingAbsoluteScale(true);
	BuildPreview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BuildPreview->SetCastShadow(false);
	BuildPreview->SetVisibility(false);
	BuildPreview->SetHiddenInGame(true);

	CarvePreview = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CarvePreview"));
	CarvePreview->SetupAttachment(RootComponent);
	CarvePreview->SetUsingAbsoluteLocation(true);
	CarvePreview->SetUsingAbsoluteRotation(true);
	CarvePreview->SetUsingAbsoluteScale(true);
	CarvePreview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CarvePreview->SetCastShadow(false);
	CarvePreview->SetVisibility(false);
	CarvePreview->SetHiddenInGame(true);

	ToolPivot = CreateDefaultSubobject<USceneComponent>(TEXT("ToolPivot"));
	ToolPivot->SetupAttachment(Camera);
	ToolPivot->SetRelativeLocation(FVector(40.f, 26.f, -30.f));

	ToolMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ToolMesh"));
	ToolMesh->SetupAttachment(ToolPivot);
	ToolMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ToolMesh->SetCastShadow(false);

	WallPreview = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WallPreview"));
	WallPreview->SetupAttachment(RootComponent);
	WallPreview->SetUsingAbsoluteLocation(true);
	WallPreview->SetUsingAbsoluteRotation(true);
	WallPreview->SetUsingAbsoluteScale(true);
	WallPreview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WallPreview->SetCastShadow(false);
	WallPreview->SetHiddenInGame(true);

	PreviewLines = CreateDefaultSubobject<ULineBatchComponent>(TEXT("PreviewLines"));
	PreviewLines->SetupAttachment(RootComponent);
	PreviewLines->SetUsingAbsoluteLocation(true);
	PreviewLines->SetUsingAbsoluteRotation(true);
	PreviewLines->SetUsingAbsoluteScale(true);
	PreviewLines->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewLines->SetCastShadow(false);
}

void AForestCharacter::BeginPlay()
{
	Super::BeginPlay();
	InitializeCoreInventoryDefinitions();

	Structures = GetWorld()->SpawnActor<AStructureManager>();
	RespawnTransform = GetActorTransform();
	Survival->OnDeath = [this]() { HandleDeath(); };
	if (UDayLoopSubsystem* Loop = UDayLoopSubsystem::Get(GetWorld()))
	{
		Loop->OnAnnouncement.AddUObject(this, &AForestCharacter::ShowMessage);
	}
	if (UMaterialInterface* Ghost = ForestItems::GetBuildGhostMaterial())
	{
		GhostMaterial = UMaterialInstanceDynamic::Create(Ghost, this);
	}
	EquipTool(EForestTool::Axe);
	CarveCylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	CarveCube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	CarvePreview->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Gameplay/Materials/MI_BuildGhost_Blue.MI_BuildGhost_Blue")));
	WallPreview->SetWorldTransform(FTransform::Identity);
	WallPreview->SetStaticMesh(ForestItems::GetMesh(EForestItem::Log));
	for (int32 Slot = 0; GhostMaterial && Slot < WallPreview->GetNumMaterials(); ++Slot)
	{
		WallPreview->SetMaterial(Slot, GhostMaterial);
	}
	ShowMessage(TEXT("Ormana hoş geldin. Ağaçları kes, kayaları kır."));
}

void AForestCharacter::InitializeCoreInventoryDefinitions()
{
	if (!CoreInventory)
	{
		return;
	}

	for (const EForestItem Item : ForestItems::GetAllItems())
	{
		// Trunk, big-log and carried-log stages are physical actors, never stack entries.
		if (Item == EForestItem::None || ForestItems::IsCarriedItem(Item))
		{
			continue;
		}

		const FName ItemID = ForestItems::GetInventoryID(Item);
		if (ItemID.IsNone())
		{
			continue;
		}

		bool bHasDefinition = false;
		for (UInventoryItemData* Existing : CoreInventory->ItemDefinitions)
		{
			if (Existing && Existing->ItemID == ItemID)
			{
				bHasDefinition = true; // Respect a project's configured definition.
				break;
			}
		}
		if (bHasDefinition)
		{
			continue;
		}

		const FName ObjectName(*FString::Printf(TEXT("RuntimeItem_%s"), *ItemID.ToString()));
		UInventoryItemData* Definition = NewObject<UInventoryItemData>(CoreInventory, ObjectName);
		if (!Definition)
		{
			continue;
		}
		Definition->ItemID = ItemID;
		Definition->DisplayName = FText::FromString(ForestItems::GetDisplayName(Item));
		Definition->Description = FText::FromString(FString::Printf(
			TEXT("Stackable survival resource: %s"), *ForestItems::GetDisplayName(Item)));
		Definition->Category = EInventoryCategory::Resource;
		Definition->Rarity = EItemRarity::Common;
		Definition->MaxStack = StackSize;
		Definition->Weight = ForestItems::GetItemWeight(Item);
		Definition->bCanBeDropped = true;
		Definition->bStackable = true;
		Definition->bCanBeIngredient = true;
		CoreInventory->ItemDefinitions.Add(Definition);
	}

	// The component may have begun play before this actor populated its runtime definitions.
	CoreInventory->InitializeRuntimeInventory();
}

UInputAction* AForestCharacter::MakeAction(const TCHAR* Name, EInputActionValueType Type)
{
	UInputAction* Action = NewObject<UInputAction>(this, FName(Name));
	Action->ValueType = Type;
	Actions.Add(Action);
	return Action;
}

void AForestCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UInputAction* MoveAction = MakeAction(TEXT("IA_Move"), EInputActionValueType::Axis2D);
	UInputAction* LookAction = MakeAction(TEXT("IA_Look"), EInputActionValueType::Axis2D);
	UInputAction* JumpAction = MakeAction(TEXT("IA_Jump"), EInputActionValueType::Boolean);
	UInputAction* SprintAction = MakeAction(TEXT("IA_Sprint"), EInputActionValueType::Boolean);
	UInputAction* PrimaryInput = MakeAction(TEXT("IA_Primary"), EInputActionValueType::Boolean);
	UInputAction* InteractAction = MakeAction(TEXT("IA_Interact"), EInputActionValueType::Boolean);
	UInputAction* CraftBlockAction = MakeAction(TEXT("IA_CraftBlock"), EInputActionValueType::Boolean);
	UInputAction* InventoryAction = MakeAction(TEXT("IA_Inventory"), EInputActionValueType::Boolean);
	UInputAction* BuildAction = MakeAction(TEXT("IA_Build"), EInputActionValueType::Boolean);
	UInputAction* RotateAction = MakeAction(TEXT("IA_Rotate"), EInputActionValueType::Boolean);
	UInputAction* CycleAction = MakeAction(TEXT("IA_CycleBuild"), EInputActionValueType::Axis1D);
	UInputAction* CancelAction = MakeAction(TEXT("IA_CancelBuild"), EInputActionValueType::Boolean);

	MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Forest"));

	// WASD into a single 2D axis: Y = forward, X = right.
	auto Swizzle = [this]()
	{
		UInputModifierSwizzleAxis* Modifier = NewObject<UInputModifierSwizzleAxis>(MappingContext);
		Modifier->Order = EInputAxisSwizzle::YXZ;
		return Modifier;
	};
	auto Negate = [this]() { return NewObject<UInputModifierNegate>(MappingContext); };

	MappingContext->MapKey(MoveAction, EKeys::W).Modifiers.Add(Swizzle());
	FEnhancedActionKeyMapping& Back = MappingContext->MapKey(MoveAction, EKeys::S);
	Back.Modifiers.Add(Swizzle());
	Back.Modifiers.Add(Negate());
	MappingContext->MapKey(MoveAction, EKeys::D);
	MappingContext->MapKey(MoveAction, EKeys::A).Modifiers.Add(Negate());

	MappingContext->MapKey(LookAction, EKeys::Mouse2D);
	MappingContext->MapKey(JumpAction, EKeys::SpaceBar);
	MappingContext->MapKey(SprintAction, EKeys::LeftShift);
	MappingContext->MapKey(PrimaryInput, EKeys::LeftMouseButton);
	MappingContext->MapKey(InteractAction, EKeys::E);
	MappingContext->MapKey(InventoryAction, EKeys::Tab);
	MappingContext->MapKey(RotateAction, EKeys::R);
	MappingContext->MapKey(CycleAction, EKeys::MouseWheelAxis);
	MappingContext->MapKey(CycleAction, EKeys::Q); // laptop friendly: next build option
	MappingContext->MapKey(CancelAction, EKeys::RightMouseButton);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(MappingContext, 0);
		}
	}

	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AForestCharacter::Move);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AForestCharacter::Look);
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
	Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	Input->BindAction(SprintAction, ETriggerEvent::Started, this, &AForestCharacter::StartSprint);
	Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &AForestCharacter::StopSprint);
	Input->BindAction(PrimaryInput, ETriggerEvent::Started, this, &AForestCharacter::PrimaryAction);
	Input->BindAction(InteractAction, ETriggerEvent::Started, this, &AForestCharacter::Interact);
	Input->BindAction(InventoryAction, ETriggerEvent::Started, this, &AForestCharacter::ToggleInventory);
	UInputAction* ToggleTrowelAction = MakeAction(TEXT("IA_ToggleTrowel"), EInputActionValueType::Boolean);
	Input->BindAction(RotateAction, ETriggerEvent::Started, this, &AForestCharacter::RotateBuild);
	Input->BindAction(CycleAction, ETriggerEvent::Started, this, &AForestCharacter::CycleBuildItem);
	Input->BindAction(CancelAction, ETriggerEvent::Started, this, &AForestCharacter::CancelBuild);

#if !UE_BUILD_SHIPPING
	UInputAction* DebugGiveAction = MakeAction(TEXT("IA_DebugGive"), EInputActionValueType::Boolean);
	UInputAction* DebugRoomAction = MakeAction(TEXT("IA_DebugRoom"), EInputActionValueType::Boolean);
	MappingContext->MapKey(DebugGiveAction, EKeys::G);
	UInputAction* DebugHarvestAction = MakeAction(TEXT("IA_DebugHarvest"), EInputActionValueType::Boolean);
	MappingContext->MapKey(DebugHarvestAction, EKeys::J);
	Input->BindAction(DebugHarvestAction, ETriggerEvent::Started, this, &AForestCharacter::DebugHarvestNearest);
	UInputAction* DebugStarveAction = MakeAction(TEXT("IA_DebugStarve"), EInputActionValueType::Boolean);
	UInputAction* DebugHurtAction = MakeAction(TEXT("IA_DebugHurt"), EInputActionValueType::Boolean);
	MappingContext->MapKey(DebugStarveAction, EKeys::N);
	MappingContext->MapKey(DebugHurtAction, EKeys::M);
	Input->BindAction(DebugStarveAction, ETriggerEvent::Started, this, &AForestCharacter::DebugStarve);
	Input->BindAction(DebugHurtAction, ETriggerEvent::Started, this, &AForestCharacter::DebugHurt);
	Input->BindAction(DebugGiveAction, ETriggerEvent::Started, this, &AForestCharacter::DebugGiveMaterials);
	Input->BindAction(DebugRoomAction, ETriggerEvent::Started, this, &AForestCharacter::DebugBuildRoom);
	UInputAction* AdminAction = MakeAction(TEXT("IA_Admin"), EInputActionValueType::Boolean);
	MappingContext->MapKey(AdminAction, EKeys::P);
	Input->BindAction(AdminAction, ETriggerEvent::Started, this, &AForestCharacter::ToggleAdminMode);
	UInputAction* CarvableAction = MakeAction(TEXT("IA_DebugCarvable"), EInputActionValueType::Boolean);
	MappingContext->MapKey(CarvableAction, EKeys::K);
	Input->BindAction(CarvableAction, ETriggerEvent::Started, this, &AForestCharacter::DebugSpawnCarvableLogs);
	UInputAction* DamageAction = MakeAction(TEXT("IA_DebugDamage"), EInputActionValueType::Boolean);
	MappingContext->MapKey(DamageAction, EKeys::Y);
	Input->BindAction(DamageAction, ETriggerEvent::Started, this, &AForestCharacter::DebugDamageStructure);
	UInputAction* DemoAction = MakeAction(TEXT("IA_DebugDemo"), EInputActionValueType::Boolean);
	MappingContext->MapKey(DemoAction, EKeys::L);
	UInputAction* LighterAction = MakeAction(TEXT("IA_Lighter"), EInputActionValueType::Boolean);
	MappingContext->MapKey(LighterAction, EKeys::F);
	Input->BindAction(LighterAction, ETriggerEvent::Started, this, &AForestCharacter::LighterPressed);
	Input->BindAction(LighterAction, ETriggerEvent::Completed, this, &AForestCharacter::LighterReleased);
	UInputAction* FirePitAction = MakeAction(TEXT("IA_FirePit"), EInputActionValueType::Boolean);
	MappingContext->MapKey(FirePitAction, EKeys::C);
	Input->BindAction(FirePitAction, ETriggerEvent::Started, this, &AForestCharacter::PlaceFireStone);
	UInputAction* HelpAction = MakeAction(TEXT("IA_Help"), EInputActionValueType::Boolean);
	MappingContext->MapKey(HelpAction, EKeys::F1);
	Input->BindAction(HelpAction, ETriggerEvent::Started, this, &AForestCharacter::ToggleHelp);
	UInputAction* EatAction = MakeAction(TEXT("IA_Eat"), EInputActionValueType::Boolean);
	MappingContext->MapKey(EatAction, EKeys::Eight);
	Input->BindAction(EatAction, ETriggerEvent::Started, this, &AForestCharacter::EatFood);
	UInputAction* StickAction = MakeAction(TEXT("IA_StickMode"), EInputActionValueType::Boolean);
	MappingContext->MapKey(StickAction, EKeys::I);
	Input->BindAction(StickAction, ETriggerEvent::Started, this, &AForestCharacter::ToggleStickMode);
	UInputAction* TempAction = MakeAction(TEXT("IA_Temporary"), EInputActionValueType::Boolean);
	MappingContext->MapKey(TempAction, EKeys::V);
	Input->BindAction(TempAction, ETriggerEvent::Started, this, &AForestCharacter::ToggleTemporary);
	UInputAction* PushAction = MakeAction(TEXT("IA_Push"), EInputActionValueType::Boolean);
	MappingContext->MapKey(PushAction, EKeys::X);
	Input->BindAction(PushAction, ETriggerEvent::Started, this, &AForestCharacter::PushAimed);
	UInputAction* OpeningAction = MakeAction(TEXT("IA_Opening"), EInputActionValueType::Boolean);
	MappingContext->MapKey(OpeningAction, EKeys::O);
	Input->BindAction(OpeningAction, ETriggerEvent::Started, this, &AForestCharacter::OpenAimed);
	UInputAction* RaidAction = MakeAction(TEXT("IA_DebugRaid"), EInputActionValueType::Boolean);
	MappingContext->MapKey(RaidAction, EKeys::Z);
	Input->BindAction(RaidAction, ETriggerEvent::Started, this, &AForestCharacter::DebugRaid);
	UInputAction* SkipAction = MakeAction(TEXT("IA_DebugSkipTime"), EInputActionValueType::Boolean);
	MappingContext->MapKey(SkipAction, EKeys::Nine);
	Input->BindAction(SkipAction, ETriggerEvent::Started, this, &AForestCharacter::DebugSkipTime);
	UInputAction* HaulerAction = MakeAction(TEXT("IA_DebugHaulers"), EInputActionValueType::Boolean);
	MappingContext->MapKey(HaulerAction, EKeys::U);
	Input->BindAction(HaulerAction, ETriggerEvent::Started, this, &AForestCharacter::DebugHaulers);
	Input->BindAction(DemoAction, ETriggerEvent::Started, this, &AForestCharacter::DebugMechanicsDemo);
#endif
}

void AForestCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateFocus();
	UpdateCarryPlacement();
	UpdateCarvePreview();
	if (bBuildMode)
	{
		UpdateBuildPreview();
	}
	DrawPreviewLines();

	UpdateTool(DeltaSeconds);
	if (const UDayLoopSubsystem* Loop = UDayLoopSubsystem::Get(GetWorld()))
	{
		Survival->SetAmbientTemperature(Loop->GetTemperatureAt(GetActorLocation()).Total());
	}
	if (bLighterHeld && !bBuildMode)
	{
		FHitResult Aim;
		if (TraceFromView(350.f, Aim, true) && Aim.GetActor())
		{
			HeatTarget(Aim.GetActor(), DeltaSeconds * 3.f);
		}
	}

	// Sprint only while moving and while stamina allows it.
	const bool bMoving = GetVelocity().SizeSquared2D() > 100.f;
	const bool bCarrying = IsHandsBusy();
	const bool bSprinting = !bCarrying && bWantsSprint && bMoving && Survival->CanSprint();
	Survival->SetSprinting(bSprinting);
	GetCharacterMovement()->MaxWalkSpeed = bCarrying ? CarrySpeed() : (bSprinting ? SprintSpeed : WalkSpeed);
	UpdateCarry(DeltaSeconds);
	if (ARiverStrip::IsInWater(GetWorld(), GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight())))
	{
		GetCharacterMovement()->MaxWalkSpeed *= ARiverStrip::WadeSpeedFactor;
	}
	UpdateRide(DeltaSeconds);

	FOVPunch = FMath::FInterpTo(FOVPunch, 0.f, DeltaSeconds, 12.f);
	Camera->SetFieldOfView(BaseFOV - FOVPunch);
}

void AForestCharacter::DrawPreviewLines()
{
	PreviewLines->Flush();
	const FLinearColor PlanColor = bGhostValid ? ForestOutline::ValidColor() : ForestOutline::InvalidColor();

	// Making: where a stake or a field would go.
	if (bStickMode && !IsHandsBusy() && (StickBuildIndex == CraftStake || StickBuildIndex == CraftField))
	{
		FHitResult Spot;
		if (StickBuildSpot(Spot))
		{
			const bool bStake = StickBuildIndex == CraftStake;
			const bool bAfford = GetItemCount(EForestItem::Stick) >= (bStake ? 1 : 2);
			const FVector Forward = FVector(GetActorForwardVector().X, GetActorForwardVector().Y, 0.f).GetSafeNormal();
			const FVector Half = bStake ? FVector(AStickStructure::RadiusCm, AStickStructure::RadiusCm, AStickStructure::HeightCm * 0.5f) : FVector(100.f, 100.f, 5.f);
			ForestOutline::DrawBox(PreviewLines, FTransform(Forward.Rotation(), Spot.ImpactPoint + FVector(0.f, 0.f, Half.Z)), FBox(-Half, Half),
				bAfford ? ForestOutline::ValidColor() : ForestOutline::InvalidColor());
		}
	}

	if (BuildPreview->IsVisible())
	{
		ForestOutline::DrawPiece(PreviewLines, SelectedBuildItem, BuildPreview->GetStaticMesh(), BuildPreview->GetComponentTransform(), PlanColor);
	}
	const UStaticMesh* LogMesh = WallPreview->GetStaticMesh();
	for (int32 Index = 0; Index < WallPreview->GetInstanceCount(); ++Index)
	{
		FTransform Instance;
		WallPreview->GetInstanceTransform(Index, Instance, /*bWorldSpace*/ true);
		ForestOutline::DrawPiece(PreviewLines, EForestItem::Log, LogMesh, Instance, PlanColor);
	}

	DrawCutGuide();

	// Where the carried piece goes: its outline there, and for a crossing the saddle notch it will get.
	if (const ACarvableLog* Piece = GetTopCarried(); Piece && CarryPlace != ECarryPlace::None)
	{
		// Only a base line where the piece will lie (along its underside) with short end marks; a crossing notches by itself.
		const FTransform& Where = CarryPlace == ECarryPlace::Across ? CrossPlacement.Upper : PlaceTransform;
		const FBox Box = Piece->GetLocalBox();
		const FLinearColor Color = bPlaceSupported ? ForestOutline::ValidColor() : ForestOutline::InvalidColor();
		const FVector A = Where.TransformPosition(FVector(Box.Min.X, 0.f, Box.Min.Z + 1.f));
		const FVector B = Where.TransformPosition(FVector(Box.Max.X, 0.f, Box.Min.Z + 1.f));
		const FVector Across = Where.TransformVectorNoScale(FVector(0.f, Box.GetExtent().Y, 0.f));
		PreviewLines->DrawLine(A, B, Color, SDPG_World, 2.f, 0.f);
		PreviewLines->DrawLine(A - Across, A + Across, Color, SDPG_World, 2.f, 0.f);
		PreviewLines->DrawLine(B - Across, B + Across, Color, SDPG_World, 2.f, 0.f);
	}
}

void AForestCharacter::DrawCutGuide()
{
	if (bBuildMode || IsHandsBusy())
	{
		return;
	}
	const FLinearColor Red = ForestOutline::CutColor();
	constexpr int32 RingSteps = 64;

	// Carvable log: where the selected tool would cut it, the rim traced on the bark.
	if (ACarvableLog* Log = FocusedCarvable.Get())
	{
		if (!CarvePreview->IsVisible())
		{
			return;
		}
		// The module grid: a faint short mark on every line; the places this tool could go get a longer white mark.
		const float Lift = Log->GetRadius() + 1.f;
		auto Mark = [this, Log, Lift](float X, float HalfDegrees, const FLinearColor& Color)
		{
			TArray<FVector> Tick;
			for (float Degrees = -HalfDegrees; Degrees <= HalfDegrees + 0.1f; Degrees += 5.f)
			{
				const float Angle = FMath::DegreesToRadians(CarveSide + Degrees);
				Tick.Add(FVector(X, FMath::Sin(Angle) * Lift, FMath::Cos(Angle) * Lift));
			}
			ForestOutline::DrawDashed(PreviewLines, Log->GetActorTransform(), Tick, false, Color);
		};
		TArray<float> Lines;
		Log->GetModuleLines(Lines);
		for (const float X : Lines)
		{
			Mark(X, 10.f, FLinearColor(0.55f, 0.55f, 0.55f));
		}
		TArray<float> Points;
		Log->GetSnapPoints(CarveTool, CarveWidth, Points);
		for (const float X : Points)
		{
			if (!FMath::IsNearlyEqual(X, CarveX, 1.f))
			{
				Mark(X, 30.f, FLinearColor::White);
			}
		}

		TArray<TArray<FVector>> Guide;
		TArray<bool> Closed;
		Log->GetCutGuide(CarveTool, CarveX, CarveSide, CarveAngle, CarveWidth, FMath::Min(CarveDepth, Log->GetRadius()), Guide, Closed);
		for (int32 Index = 0; Index < Guide.Num(); ++Index)
		{
			ForestOutline::DrawDashed(PreviewLines, Log->GetActorTransform(), Guide[Index], Closed[Index], Red);
		}
		return;
	}

	const AHarvestableActor* Target = FocusedHarvestable.Get();
	if (!Target || !Target->IsWood() || CurrentTool != EForestTool::Axe)
	{
		return;
	}
	TArray<FVector> Ring;
	if (Target->IsTree())
	{
		// Standing tree: a ring round the trunk at the height being aimed at, hugging the bark.
		const FVector Axis = Target->GetActorLocation();
		const float Radius = FVector::Dist2D(FocusHitPoint, Axis) + 1.f;
		for (int32 Step = 0; Step < RingSteps; ++Step)
		{
			const float Angle = 2.f * PI * Step / RingSteps;
			Ring.Add(FVector(Axis.X + FMath::Cos(Angle) * Radius, Axis.Y + FMath::Sin(Angle) * Radius, FocusHitPoint.Z));
		}
		ForestOutline::DrawDashed(PreviewLines, FTransform::Identity, Ring, true, Red);
		return;
	}
	// Lying trunk or big log: it always splits in the middle.
	const UStaticMeshComponent* LogMesh = Target->FindComponentByClass<UStaticMeshComponent>();
	if (!LogMesh || !LogMesh->GetStaticMesh())
	{
		return;
	}
	const FBox Box = LogMesh->GetStaticMesh()->GetBoundingBox();
	const FVector Centre = Box.GetCenter();
	const float Radius = (Box.GetExtent().Y + Box.GetExtent().Z) * 0.5f + 0.8f;
	for (int32 Step = 0; Step < RingSteps; ++Step)
	{
		const float Angle = 2.f * PI * Step / RingSteps;
		Ring.Add(Centre + FVector(0.f, FMath::Sin(Angle) * Radius, FMath::Cos(Angle) * Radius));
	}
	ForestOutline::DrawDashed(PreviewLines, LogMesh->GetComponentTransform(), Ring, true, Red);
}

void AForestCharacter::EquipTool(EForestTool Tool)
{
	CurrentTool = Tool;
	UStaticMesh* ToolStaticMesh = ForestItems::GetToolMesh(Tool);
	ToolMesh->SetStaticMesh(ToolStaticMesh);
	if (!ToolStaticMesh)
	{
		return;
	}

	// Greybox tools stand along Z with the head on top and the edge toward +X: the grip is a hand's width above the handle end.
	const FBox Box = ToolStaticMesh->GetBoundingBox();
	const FQuat Rotation = FQuat::Identity;
	const FVector Grip(0.f, 0.f, Box.Min.Z + 8.f);
	const float Scale = 1.f;
	ToolMesh->SetRelativeScale3D(FVector(Scale));
	ToolMesh->SetRelativeRotation(Rotation);
	ToolMesh->SetRelativeLocation(-Rotation.RotateVector(Grip) * Scale);
}

void AForestCharacter::ToggleTrowel()
{
	if (CurrentTool == EForestTool::Trowel)
	{
		EquipTool(EForestTool::Axe);
		ShowMessage(TEXT("Balta kuşanıldı"));
	}
	else
	{
		EquipTool(EForestTool::Trowel);
		ShowMessage(TEXT("Mala (Chinking & Harç Aracı) kuşanıldı  —  Kütük duvarlara veya taş temele sol tıkla"));
	}
}

void AForestCharacter::UpdateTool(float DeltaSeconds)
{
	const float RestPitch = 12.f;
	float Pitch = RestPitch;
	if (SwingTime >= 0.f)
	{
		SwingTime += DeltaSeconds;
		const float Raise = 0.12f, Strike = 0.24f, Recover = 0.5f;
		if (SwingTime < Raise)
		{
			Pitch = FMath::InterpEaseOut(RestPitch, 45.f, SwingTime / Raise, 2.f);
		}
		else if (SwingTime < Strike)
		{
			Pitch = FMath::InterpEaseIn(45.f, -60.f, (SwingTime - Raise) / (Strike - Raise), 2.f);
		}
		else if (SwingTime < Recover)
		{
			Pitch = FMath::InterpEaseInOut(-60.f, RestPitch, (SwingTime - Strike) / (Recover - Strike), 2.f);
		}
		else
		{
			SwingTime = -1.f;
		}

		// The hit lands when the blade comes down, not on the click.
		if (bPendingHit && SwingTime >= Strike - 0.03f)
		{
			bPendingHit = false;
			PerformHit();
		}
	}
	ToolPivot->SetRelativeRotation(FRotator(Pitch, -12.f, -8.f));
}

void AForestCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Axis.X);
}

void AForestCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(-Axis.Y);
}

void AForestCharacter::StartSprint()
{
	bWantsSprint = true;
}

void AForestCharacter::StopSprint()
{
	bWantsSprint = false;
}

void AForestCharacter::HandleDeath()
{
	// GDD 6.2: back to the last respawn point; the base and stored items stay as they are.
	DropCarried();
	ShowMessage(TEXT("\u00D6ld\u00FCn. Son dinlenme noktas\u0131nda uyand\u0131n."));
	SetActorTransform(RespawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
	if (AController* PC = GetController())
	{
		PC->SetControlRotation(RespawnTransform.Rotator());
	}
	GetCharacterMovement()->StopMovementImmediately();
	Survival->ResetAll();
	Survival->Eat(-50.f);
	Survival->Drink(-50.f);
}

float AForestCharacter::CarrySpeed() const
{
	float Speed = WalkSpeed;
	if (StoneCarry && StoneCarry->IsCarrying())
	{
		const AStoneActor* Stone = StoneCarry->GetCarried();
		const UStoneDefinition* Definition = Stone ? Stone->GetDefinition() : nullptr;
		const float Mass = Definition ? FMath::Max(0.0f, Definition->Mass) : 20.0f;
		Speed = FMath::Clamp(WalkSpeed - Mass * 1.5f, 180.0f, WalkSpeed);
	}
	else if (CarriedItem == EForestItem::TrunkLog)
	{
		Speed = 150.f;
	}
	else if (CarriedItem == EForestItem::BigLog)
	{
		Speed = 170.f;
	}
	else if (CarriedItem == EForestItem::Log)
	{
		// A whole log on the shoulder slows a walk from 380 to about 220.
		Speed = 340.f - 120.f * GetCarryLoad();
	}
	if (Survival->GetStamina() <= 0.f)
	{
		Speed *= 0.5f;
	}
	return Speed;
}

bool AForestCharacter::IsHandsBusy() const
{
	return CarriedItem != EForestItem::None || (StoneCarry && StoneCarry->IsCarrying());
}

void AForestCharacter::RefreshToolVisibility()
{
	ToolPivot->SetVisibility(!bBuildMode && !IsHandsBusy(), /*bPropagateToChildren*/ true);
}

bool AForestCharacter::StartCarry(ACarvableLog* Piece)
{
	if (!Piece || Piece->IsCarried())
	{
		return false;
	}
	if (CarriedItem != EForestItem::None && CarriedItem != EForestItem::Log)
	{
		ShowMessage(TEXT("Elin dolu"));
		return false;
	}
	const float After = GetCarryLoad() + Piece->GetVolume() / ACarvableLog::GetShoulderCapacity();
	if (After > 1.f)
	{
		ShowMessage(FString::Printf(TEXT("Omzun dolu: bu parça yükü %%%.0f yapar (en fazla %%100)"), After * 100.f));
		return false;
	}
	// Taken out of a structure: whatever rested on it is checked again once it is on the shoulder.
	const bool bFromStructure = Piece->IsAnchored();
	Piece->Release();
	CarriedItem = EForestItem::Log;
	CarriedLogs.Add(Piece);
	Piece->BeginCarry(Camera, FTransform::Identity);
	ArrangeShoulder();
	if (bFromStructure)
	{
		SettleStructure();
	}
	FocusedCarvable = nullptr;
	FocusedPickup = nullptr;
	RefreshToolVisibility();
	ShowMessage(FString::Printf(TEXT("Omzunda %d parça, yük %%%.0f  —  [Sol tık] koy, [R] yatır/dik, [E] bırak"),
		CarriedLogs.Num(), GetCarryLoad() * 100.f));
	return true;
}

void AForestCharacter::ArrangeShoulder()
{
	// Stacked on the right shoulder along the view, each piece centred on the shoulder, resting on the one below.
	const FQuat Along = FRotator(-6.f, 4.f, 0.f).Quaternion();
	float Height = 0.f;
	for (int32 Slot = 0; Slot < CarriedLogs.Num(); ++Slot)
	{
		ACarvableLog* Piece = CarriedLogs[Slot].Get();
		if (!Piece)
		{
			continue;
		}
		const float Radius = Piece->GetRadius();
		const FVector Shoulder(30.f, 42.f - Slot * 6.f, -38.f + Height + Radius - 35.f);
		Piece->SetActorRelativeTransform(FTransform(Along, Shoulder - Along.RotateVector(FVector(Piece->GetCenterX(), 0.f, 0.f))));
		Height += Radius * 1.6f;
	}
}

void AForestCharacter::ShoulderBigLog(AHarvestableActor* BigLog)
{
	CarriedItem = EForestItem::BigLog;
	ShoulderedLog = BigLog;
	BigLog->BeginShoulder(Camera, FTransform(FRotator(-8.f, 3.f, 0.f), FVector(20.f, 48.f, -45.f)));
	FocusedHarvestable = nullptr;
	RefreshToolVisibility();
	ShowMessage(TEXT("Büyük kütük omzunda, çok ağır  —  [E] bırak"));
}

float AForestCharacter::RopeEndZ() const
{
	// The tied end is lifted a little off the ground by the rope.
	return GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 25.f;
}

void AForestCharacter::StartRopeDrag(AHarvestableActor* Trunk)
{
	UStaticMeshComponent* TrunkMesh = Trunk->FindComponentByClass<UStaticMeshComponent>();
	if (!TrunkMesh || !TrunkMesh->GetStaticMesh())
	{
		return;
	}
	// The rope goes round the end nearest the player.
	const FBox Box = TrunkMesh->GetStaticMesh()->GetBoundingBox();
	const float Length = Box.GetSize().X * Trunk->GetActorScale3D().X;
	const FVector Center = Trunk->GetActorTransform().TransformPosition(Box.GetCenter());
	const FVector Axis = FVector(Trunk->GetActorForwardVector().X, Trunk->GetActorForwardVector().Y, 0.f).GetSafeNormal();
	const FVector EndA = Center + Axis * Length * 0.5f;
	const FVector EndB = Center - Axis * Length * 0.5f;
	RopeFront = FVector::DistSquared2D(EndA, GetActorLocation()) < FVector::DistSquared2D(EndB, GetActorLocation()) ? EndA : EndB;
	RopeFront.Z = RopeEndZ();

	Trunk->BeginDrag(GetActorLocation());
	Trunk->DragTo(RopeFront);
	RopedLog = Trunk;
	CarriedItem = EForestItem::TrunkLog;

	// From the right hand to the tied end, sagging a little.
	Rope = NewObject<UCableComponent>(this);
	Rope->SetupAttachment(Camera);
	Rope->SetRelativeLocation(FVector(35.f, 25.f, -45.f));
	Rope->CableLength = RopeLength;
	Rope->NumSegments = 12;
	Rope->CableWidth = 3.f;
	Rope->bAttachEnd = true;
	Rope->SetAttachEndToComponent(TrunkMesh);
	Rope->EndLocation = TrunkMesh->GetComponentTransform().InverseTransformPosition(RopeFront);
	if (UMaterialInterface* RopeBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		UMaterialInstanceDynamic* RopeMaterial = UMaterialInstanceDynamic::Create(RopeBase, this);
		RopeMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.32f, 0.24f, 0.14f));
		Rope->SetMaterial(0, RopeMaterial);
	}
	Rope->RegisterComponent();

	FocusedHarvestable = nullptr;
	RefreshToolVisibility();
	ShowMessage(TEXT("Gövdenin ucuna ip bağlandı, sürükleniyor  —  [E] bırak"));
}

void AForestCharacter::StopRopeDrag()
{
	if (AHarvestableActor* Trunk = RopedLog.Get())
	{
		Trunk->EndDrag();
	}
	RopedLog = nullptr;
	if (Rope)
	{
		Rope->DestroyComponent();
		Rope = nullptr;
	}
	if (CarriedItem == EForestItem::TrunkLog)
	{
		CarriedItem = EForestItem::None;
	}
	RefreshToolVisibility();
}

ACarvableLog* AForestCharacter::GetTopCarried() const
{
	return CarriedLogs.Num() > 0 ? CarriedLogs.Last().Get() : nullptr;
}

float AForestCharacter::GetCarryLoad() const
{
	float Wood = 0.f;
	for (const TWeakObjectPtr<ACarvableLog>& Piece : CarriedLogs)
	{
		Wood += Piece.IsValid() ? Piece->GetVolume() : 0.f;
	}
	return Wood / ACarvableLog::GetShoulderCapacity();
}

void AForestCharacter::UpdateCarryPlacement()
{
	FHitResult Hit;
	const bool bHit = CarriedItem == EForestItem::Log && !bBuildMode && TraceFromView(ReachDistance + 150.f, Hit);
	UpdateCarryPlacementAt(Hit, bHit);
}

void AForestCharacter::UpdateCarryPlacementAt(const FHitResult& Hit, bool bHit)
{
	CarryPlace = ECarryPlace::None;
	CrossLower = nullptr;
	bPlaceSupported = true;
	ACarvableLog* Piece = GetTopCarried();
	if (!Piece || !bHit)
	{
		return;
	}
	// A fire: any wood goes on it. A damaged piece of a structure: swap it for the carried one of the same size.
	PlaceTarget = Hit.GetActor();
	if (ACampfire* Fire = Cast<ACampfire>(Hit.GetActor()))
	{
		CarryPlace = ECarryPlace::Fuel;
		return;
	}
	// A cart, sled or rack takes whatever fits on it.
	if (AHaulCart* Cart = Cast<AHaulCart>(Hit.GetActor()))
	{
		CarryPlace = Cart->CanLoad(Piece) ? ECarryPlace::Load : ECarryPlace::None;
		return;
	}
	if (ACarvableLog* Damaged = Cast<ACarvableLog>(Hit.GetActor()); Damaged && ACarvableLog::CanReplace(Damaged, Piece))
	{
		CarryPlace = ECarryPlace::Repair;
		return;
	}
	// Onto a post: a long piece lies on its top as a beam, its end flush with the post, running towards another post
	// of the same height if there is one within reach.
	if (const ACarvableLog* Post = Cast<ACarvableLog>(Hit.GetActor()); Post && Post->IsAnchored() && !Piece->IsSplit())
	{
		FVector PostCentre, PostAxis;
		float PostRadius, PostHalf;
		const bool bPost = ACarvableLog::GetLogAxis(Post, PostCentre, PostAxis, PostRadius, PostHalf) && FMath::Abs(PostAxis.Z) > 0.95f;
		// A post that already carries a beam takes a brace (a piece two to four modules long), nothing else on top.
		if (bPost && ACarvableLog::FindBeamOnPost(Post))
		{
			if (ACarvableLog::ComputeBracePlacement(Post, Piece, PlaceTransform))
			{
				CarryPlace = ECarryPlace::Brace;
				bPlaceSupported = StructureLink::WouldBeSupported(GetWorld(), Piece, PlaceTransform);
			}
			return;
		}
		if (bPost && Piece->GetLength() >= 3.f * ACarvableLog::ModuleLength - 1.f)
		{
			const FVector Top = PostCentre + PostAxis * (PostAxis.Z > 0.f ? PostHalf : -PostHalf);
			const FVector Forward = FVector(GetActorForwardVector().X, GetActorForwardVector().Y, 0.f).GetSafeNormal();
			FVector Direction = Forward;
			float Farthest = 0.f;
			for (TActorIterator<ACarvableLog> It(GetWorld()); It; ++It)
			{
				FVector OtherCentre, OtherAxis;
				float OtherRadius, OtherHalf;
				if (*It == Post || !It->IsAnchored() || !ACarvableLog::GetLogAxis(*It, OtherCentre, OtherAxis, OtherRadius, OtherHalf)
					|| FMath::Abs(OtherAxis.Z) < 0.95f)
				{
					continue;
				}
				const FVector OtherTop = OtherCentre + OtherAxis * (OtherAxis.Z > 0.f ? OtherHalf : -OtherHalf);
				const FVector Flat(OtherTop.X - Top.X, OtherTop.Y - Top.Y, 0.f);
				const float Distance = Flat.Size();
				// Same height (the grid cuts posts the same length), the far post fully under the beam, ahead of the player.
				if (FMath::Abs(OtherTop.Z - Top.Z) <= 5.f && Distance > PostRadius + OtherRadius
					&& Distance <= Piece->GetLength() - PostRadius - OtherRadius && Distance > Farthest
					&& FVector::DotProduct(Flat / Distance, Forward) > 0.85f)
				{
					Farthest = Distance;
					Direction = Flat / Distance;
				}
			}
			const FQuat Rotation = FRotator(0.f, Direction.Rotation().Yaw, 0.f).Quaternion();
			const FVector End(Top.X - Direction.X * PostRadius, Top.Y - Direction.Y * PostRadius, Top.Z + Piece->GetRadius() + 0.5f);
			PlaceTransform = FTransform(Rotation, End - Rotation.RotateVector(FVector(Piece->GetLocalBox().Min.X, 0.f, 0.f)));
			CarryPlace = ECarryPlace::Beam;
			bPlaceSupported = StructureLink::WouldBeSupported(GetWorld(), Piece, PlaceTransform);
			return;
		}
	}
	// A split piece onto a lying log: a floor board, flat face up, resting on it.
	AActor* Lower = Hit.GetActor();
	// A post stood on the top of an installed lying log (a platform beam): its foot rests on it.
	if ((bPlaceStanding || bPlaceSlope) && Lower && Hit.ImpactNormal.Z > 0.6f && !Piece->IsSplit())
	{
		FVector BaseCentre, BaseAxis;
		float BaseRadius, BaseHalf;
		const ACarvableLog* Base = Cast<ACarvableLog>(Lower);
		if (Base && Base->IsAnchored() && ACarvableLog::GetLogAxis(Base, BaseCentre, BaseAxis, BaseRadius, BaseHalf) && FMath::Abs(BaseAxis.Z) < 0.3f)
		{
			const FVector Facing = FVector(GetActorForwardVector().X, GetActorForwardVector().Y, 0.f).GetSafeNormal();
			if (bPlaceSlope)
			{
				ACarvableLog::ComputeSlopePlacement(Hit.ImpactPoint, Facing, SlopeAngle, Piece, PlaceTransform);
				CarryPlace = ECarryPlace::Slope;
			}
			else
			{
				PlaceTransform = Piece->GetStandTransform(Hit.ImpactPoint, Facing.Rotation().Yaw);
				CarryPlace = ECarryPlace::Stand;
			}
			bPlaceSupported = StructureLink::WouldBeSupported(GetWorld(), Piece, PlaceTransform);
			return;
		}
	}
	if (Lower && Lower->IsA<ACarvableLog>() && ACarvableLog::ComputeFloorPlacement(Lower, Hit.ImpactPoint, GetActorLocation(), Piece, PlaceTransform))
	{
		CarryPlace = ECarryPlace::Floor;
		bPlaceSupported = StructureLink::WouldBeSupported(GetWorld(), Piece, PlaceTransform);
		return;
	}
	// Onto a lying log: square across it, notched (whichever mode is chosen, a post does not stand on a log).
	if (Lower && (Lower->IsA<ACarvableLog>() || Lower->IsA<APickupItem>())
		&& ACarvableLog::ComputeCrossPlacement(Lower, Hit.ImpactPoint, GetActorLocation(), Piece, CrossPlacement))
	{
		CrossLower = Lower;
		CarryPlace = ECarryPlace::Across;
		bPlaceSupported = StructureLink::WouldBeSupported(GetWorld(), Piece, CrossPlacement.Upper);
		return;
	}
	// On the ground (anything flat enough): lying across the view, or standing up as a post.
	if (Hit.ImpactNormal.Z < 0.7f || (Lower && (Lower->IsA<ACarvableLog>() || Lower->IsA<APickupItem>())))
	{
		return;
	}
	const FVector Forward = FVector(GetActorForwardVector().X, GetActorForwardVector().Y, 0.f).GetSafeNormal();
	if (bPlaceSlope)
	{
		if (ACarvableLog::ComputeSlopePlacement(Hit.ImpactPoint, Forward, SlopeAngle, Piece, PlaceTransform))
		{
			CarryPlace = ECarryPlace::Slope;
			bPlaceSupported = StructureLink::WouldBeSupported(GetWorld(), Piece, PlaceTransform);
		}
		return;
	}
	float Yaw = bPlaceStanding ? Forward.Rotation().Yaw : Forward.Rotation().Yaw + 90.f;
	FVector Ground = Hit.ImpactPoint;
	// Snap to the nearest lying log within 4 m: parallel or square to it (never at an odd angle), and on its 75 cm
	// module grid, so pieces laid next to each other line up into one structure.
	const ACarvableLog* Near = nullptr;
	float NearDistance = 400.f;
	for (TActorIterator<ACarvableLog> It(GetWorld()); It; ++It)
	{
		FVector C, A;
		float R, H;
		if (*It == Piece || It->IsCarried() || !ACarvableLog::GetLogAxis(*It, C, A, R, H) || FMath::Abs(A.Z) > 0.3f)
		{
			continue;
		}
		const FVector Closest = FMath::ClosestPointOnSegment(Ground, C - A * H, C + A * H);
		const float Distance = FVector::Dist2D(Closest, Ground);
		if (Distance < NearDistance)
		{
			NearDistance = Distance;
			Near = *It;
		}
	}
	if (Near)
	{
		const FTransform NearFrame = Near->GetActorTransform();
		const float NearYaw = NearFrame.GetRotation().Rotator().Yaw;
		Yaw = NearYaw + 90.f * FMath::RoundToFloat(FMath::FindDeltaAngleDegrees(NearYaw, Yaw) / 90.f);
		FVector Local = NearFrame.InverseTransformPosition(Ground);
		const float MinX = Near->GetLocalBox().Min.X;
		const float Module = ACarvableLog::ModuleLength;
		Local.X = MinX + FMath::RoundToFloat((Local.X - MinX) / Module) * Module;
		Local.Y = FMath::RoundToFloat(Local.Y / Module) * Module;
		Ground = FVector(NearFrame.TransformPosition(Local).X, NearFrame.TransformPosition(Local).Y, Ground.Z);
	}
	PlaceTransform = bPlaceStanding ? Piece->GetStandTransform(Ground, Yaw) : Piece->GetLayTransform(Ground, Yaw);
	CarryPlace = bPlaceStanding ? ECarryPlace::Stand : ECarryPlace::Lay;
}

bool AForestCharacter::PlaceCarried()
{
	ACarvableLog* Piece = GetTopCarried();
	if (!Piece || CarryPlace == ECarryPlace::None)
	{
		ShowMessage(TEXT("Buraya konmaz  —  yere ya da yerdeki bir kütüğe bak"));
		return false;
	}
	CarriedLogs.Pop();
	switch (CarryPlace)
	{
	case ECarryPlace::Across:
		ACarvableLog::PlaceAcross(CrossLower.Get(), CrossPlacement, Piece);
		ShowMessage(TEXT("Kütük çaprazlama oturdu  —  altına eyer çentiği açıldı"));
		SettleStructure();
		break;
	case ECarryPlace::Stand:
		// A post stays exactly where it is put.
		Piece->PlaceAt(PlaceTransform, /*bAnchorIt*/ true);
		ACarvableLog::OnLogStoodUp.Broadcast(Piece);
		ShowMessage(TEXT("Direk dikildi"));
		SettleStructure();
		break;
	case ECarryPlace::Beam:
		Piece->PlaceAt(PlaceTransform, /*bAnchorIt*/ true);
		ShowMessage(TEXT("Kiriş kondu"));
		SettleStructure();
		break;
	case ECarryPlace::Brace:
		Piece->PlaceAt(PlaceTransform, /*bAnchorIt*/ true);
		ShowMessage(TEXT("Payanda kondu"));
		SettleStructure();
		break;
	case ECarryPlace::Floor:
		Piece->PlaceAt(PlaceTransform, /*bAnchorIt*/ true);
		ShowMessage(TEXT("Döşeme tahtası kondu"));
		SettleStructure();
		break;
	case ECarryPlace::Slope:
		Piece->PlaceAt(PlaceTransform, /*bAnchorIt*/ true);
		ShowMessage(FString::Printf(TEXT("Eğik parça kondu (%.0f°)"), SlopeAngle));
		SettleStructure();
		break;
	case ECarryPlace::Load:
	{
		AHaulCart* Cart = Cast<AHaulCart>(PlaceTarget.Get());
		if (Cart && Cart->Load(Piece))
		{
			ShowMessage(FString::Printf(TEXT("%s: yüklendi (%d parça)  —  indirmek için [E]"), *AHaulCart::KindName(Cart->GetKind()), Cart->GetLoadCount()));
		}
		else
		{
			CarriedLogs.Add(Piece);
			ShowMessage(TEXT("Sığmıyor"));
		}
		break;
	}
	case ECarryPlace::Fuel:
	{
		ACampfire* Fire = Cast<ACampfire>(PlaceTarget.Get());
		const float Seconds = ACampfire::FuelSecondsOf(Piece);
		if (Fire && Fire->AddFuel(Piece))
		{
			ShowMessage(FString::Printf(TEXT("Ateşe odun atıldı (+%.0f sn), toplam %.0f sn%s"), Seconds, Fire->GetFuelSeconds(), Fire->IsBurning() ? TEXT("") : TEXT("  —  yakmak için [F] çakmak")));
		}
		else
		{
			// A full fire keeps its wood: it goes back on the shoulder.
			CarriedLogs.Add(Piece);
			ShowMessage(TEXT("Ateş dolu"));
		}
		break;
	}
	case ECarryPlace::Repair:
	{
		ACarvableLog* Damaged = Cast<ACarvableLog>(PlaceTarget.Get());
		if (ACarvableLog::ReplaceDamaged(Damaged, Piece))
		{
			// The damaged one comes off the structure and onto the shoulder, for firewood or scrap.
			StartCarry(Damaged);
			ShowMessage(TEXT("Hasarlı parça değiştirildi"));
			SettleStructure();
		}
		break;
	}
	default:
		Piece->PlaceAt(PlaceTransform, /*bAnchorIt*/ false);
		break;
	}
	CarryPlace = ECarryPlace::None;
	if (CarriedLogs.Num() == 0)
	{
		CarriedItem = EForestItem::None;
	}
	ArrangeShoulder();
	RefreshToolVisibility();
	return true;
}

void AForestCharacter::SettleStructure()
{
	const int32 Fallen = StructureLink::Settle(GetWorld());
	if (Fallen > 0)
	{
		ShowMessage(FString::Printf(TEXT("%d parça desteksiz kaldı ve düştü"), Fallen));
	}
}

bool AForestCharacter::TestPickUp(ACarvableLog* Piece)
{
	return StartCarry(Piece);
}

ACampfire* AForestCharacter::TestBuildFirePit(const FHitResult& Hit, bool bGiveStones)
{
	if (bGiveStones)
	{
		AddItem(EForestItem::Stone, FirePitStones);
	}
	if (GetItemCount(EForestItem::Stone) < FirePitStones)
	{
		return nullptr;
	}
	// Seven stones, one after another, round the spot.
	ACampfire* Pit = nullptr;
	for (int32 Index = 0; Index < FirePitStones; ++Index)
	{
		const float Angle = 2.f * PI * Index / FirePitStones;
		FHitResult Spot = Hit;
		Spot.ImpactPoint = Hit.ImpactPoint + FVector(FMath::Cos(Angle) * 45.f, FMath::Sin(Angle) * 45.f, 0.f);
		Pit = TestPlaceStone(Spot, false);
	}
	return Pit;
}

void AForestCharacter::TestLight(AActor* Target, float Seconds)
{
	HeatTarget(Target, Seconds);
}

void AForestCharacter::HeatTarget(AActor* Target, float Seconds)
{
	if (ACarvableLog* Piece = Cast<ACarvableLog>(Target))
	{
		Piece->ApplyHeat(Seconds);
		if (!Piece->IsBurning() && !Piece->IsCharred())
		{
			ShowMessage(FString::Printf(TEXT("Tutuşuyor %%%.0f"), Piece->GetHeatShare() * 100.f));
		}
	}
	else if (AStickStructure* Stake = Cast<AStickStructure>(Target))
	{
		if (Stake->ApplyHeat(Seconds))
		{
			ShowMessage(TEXT("Kazık meşale oldu"));
		}
	}
	else if (ACampfire* Fire = Cast<ACampfire>(Target))
	{
		Fire->ApplyHeat(Seconds);
		if (Fire->GetFuelSeconds() <= 0.f)
		{
			ShowMessage(TEXT("Ateşte yakıt yok  —  önce odun at"));
		}
	}
}

void AForestCharacter::LighterPressed()
{
	bLighterHeld = true;
}

void AForestCharacter::LighterReleased()
{
	bLighterHeld = false;
}

void AForestCharacter::PlaceFireStone()
{
	if (IsHandsBusy())
	{
		ShowMessage(TEXT("Taş koymak için elin boş olmalı"));
		return;
	}
	if (GetItemCount(EForestItem::Stone) < 1)
	{
		ShowMessage(FString::Printf(TEXT("Ateş yeri için %d taş gerek: taş topla, düz zemine [C] ile tek tek koy"), FirePitStones));
		return;
	}
	FHitResult Hit;
	if (!TraceFromView(500.f, Hit, true) || Hit.ImpactNormal.Z < 0.7f || Hit.GetActor()->IsA<ACarvableLog>())
	{
		ShowMessage(TEXT("Taş düz zemine konur  —  yere bak"));
		return;
	}
	if (ACampfire* Pit = TestPlaceStone(Hit, false))
	{
		ShowMessage(TEXT("Ateş yeri tamam  —  odun at, [F] çakmakla yak"));
		(void)Pit;
	}
	else
	{
		ShowMessage(FString::Printf(TEXT("Taş koyuldu (%d / %d)"), AFireStone::CountNear(GetWorld(), Hit.ImpactPoint), FirePitStones));
	}
}

ACampfire* AForestCharacter::TestPlaceStone(const FHitResult& Hit, bool bGiveStone)
{
	if (bGiveStone)
	{
		AddItem(EForestItem::Stone, 1);
	}
	if (GetItemCount(EForestItem::Stone) < 1)
	{
		return nullptr;
	}
	RemoveItem(EForestItem::Stone, 1);
	AFireStone* Stone = AFireStone::Place(GetWorld(), Hit.ImpactPoint);
	return AFireStone::TryForm(GetWorld(), Stone);
}

void AForestCharacter::TakeEnemyHit(float Amount)
{
	Survival->ApplyDamage(Amount);
}

void AForestCharacter::ToggleTemporary()
{
	ACarvableLog* Piece = FocusedCarvable.Get();
	if (!Piece || !Piece->IsAnchored())
	{
		ShowMessage(TEXT("İskele işareti yapıdaki bir parçaya bakınca konur"));
		return;
	}
	TestToggleTemporary(Piece);
	ShowMessage(Piece->IsTemporary() ? TEXT("Geçici (iskele) işaretlendi") : TEXT("İskele işareti kaldırıldı"));
}

void AForestCharacter::TestToggleTemporary(ACarvableLog* Piece)
{
	if (Piece)
	{
		Piece->SetTemporary(!Piece->IsTemporary());
		StructureLink::MarkChanged();
	}
}

void AForestCharacter::PushAimed()
{
	FHitResult Hit;
	if (!TraceFromView(350.f, Hit, true) || !Hit.GetActor())
	{
		return;
	}
	const FVector Forward = FVector(GetActorForwardVector().X, GetActorForwardVector().Y, 0.f).GetSafeNormal();
	if (TestPush(Hit.GetActor(), Forward * 300.f))
	{
		ShowMessage(TEXT("İtildi"));
	}
	else
	{
		ShowMessage(TEXT("İtilmez: yapıda ya da omuzda"));
	}
}

bool AForestCharacter::TestPush(AActor* Target, const FVector& Velocity)
{
	if (ACarvableLog* Piece = Cast<ACarvableLog>(Target))
	{
		return Piece->Push(Velocity);
	}
	if (AHaulCart* Cart = Cast<AHaulCart>(Target))
	{
		return Cart->Shove(Velocity);
	}
	return false;
}

ACarvableLog* AForestCharacter::TestUnload(AHaulCart* Cart)
{
	ACarvableLog* Piece = Cart ? Cart->Unload() : nullptr;
	if (Piece)
	{
		StartCarry(Piece);
	}
	return Piece;
}

void AForestCharacter::OpenAimed()
{
	FHitResult Hit;
	ACarvableLog* Piece = TraceFromView(500.f, Hit, true) ? Cast<ACarvableLog>(Hit.GetActor()) : nullptr;
	if (!Piece || !Piece->IsAnchored())
	{
		ShowMessage(TEXT("Açıklık, duvardaki yatay bir kütüğe bakınca açılır"));
		return;
	}
	const float LocalX = Piece->GetActorTransform().InverseTransformPosition(Hit.ImpactPoint).X;
	if (Piece->CutOpening(LocalX, 2))
	{
		ShowMessage(TEXT("Açıklık açıldı (2 modül)"));
		SettleStructure();
	}
	else
	{
		ShowMessage(TEXT("Burada açıklık açılmaz: iki yanında da en az 1 modül kalmalı"));
	}
}

FString AForestCharacter::CraftEntryName(uint8 Entry)
{
	switch (Entry)
	{
	case CraftStake:    return TEXT("Kazık çubuk");
	case CraftField:    return TEXT("Tarla");
	case CraftGarment:  return TEXT("Deri giysi");
	case CraftRope:     return TEXT("İp");
	case CraftRopeLine: return TEXT("İp hattı");
	default:            return TEXT("");
	}
}

bool AForestCharacter::TestCraft(uint8 Entry, const FHitResult& Hit)
{
	StickBuildIndex = Entry % CraftEntryCount;
	switch (StickBuildIndex)
	{
	case CraftStake:
		return PlantStakeAt(Hit) != nullptr;
	case CraftField:
		if (GetItemCount(EForestItem::Stick) < 2)
		{
			ShowMessage(TEXT("Tarla sınırı için 2 çubuk gerek"));
			return false;
		}
		if (!FarmKit::MakeField(GetWorld(), Hit.ImpactPoint))
		{
			return false;
		}
		RemoveItem(EForestItem::Stick, 2);
		ShowMessage(TEXT("Tarla açıldı  —  [E] ile tohum ek"));
		return true;
	case CraftGarment:
		if (FarmKit::MakeHideGarment(this))
		{
			ShowMessage(FString::Printf(TEXT("Deri giysi yapıldı: soğuğa yalıtım %%%.0f"), Survival->Insulation * 100.f));
			return true;
		}
		ShowMessage(FString::Printf(TEXT("Deri giysi için %d deri gerek (giysiler zaten tamsa daha fazlası işe yaramaz)"), FarmKit::HidesPerGarment));
		return false;
	case CraftRope:
		// Plant fibre and vines first: wool and hide are worth more as clothing.
		if (GetItemCount(EForestItem::PlantFiber) >= 3)
		{
			RemoveItem(EForestItem::PlantFiber, 3);
		}
		else if (GetItemCount(EForestItem::Vine) >= 2)
		{
			RemoveItem(EForestItem::Vine, 2);
		}
		else if (GetItemCount(EForestItem::Wool) >= 2)
		{
			RemoveItem(EForestItem::Wool, 2);
		}
		else if (GetItemCount(EForestItem::Hide) >= 1)
		{
			RemoveItem(EForestItem::Hide, 1);
		}
		else
		{
			ShowMessage(TEXT("İp için 3 lif, 2 asma, 2 yün ya da 1 deri gerek"));
			return false;
		}
		AddItem(EForestItem::Rope, 1);
		ShowMessage(FString::Printf(TEXT("İp büküldü (%d ip)"), GetItemCount(EForestItem::Rope)));
		return true;
	case CraftRopeLine:
	{
		if (!bRopeAnchorSet)
		{
			if (GetItemCount(EForestItem::Rope) < 1)
			{
				ShowMessage(TEXT("İp hattı için 1 ip gerek: [I] listesinden lif, asma, yün ya da deriden ip bük"));
				return false;
			}
			bRopeAnchorSet = true;
			RopeAnchor = Hit.ImpactPoint;
			ShowMessage(TEXT("İpin bir ucu bağlandı  —  öbür ucu için bak ve [Sol tık]"));
			return true;
		}
		ARopeLine* NewLine = ARopeLine::Span(GetWorld(), RopeAnchor, Hit.ImpactPoint);
		if (!NewLine)
		{
			ShowMessage(FString::Printf(TEXT("İp %.0f-%.0f m arası olmalı"), ARopeLine::MinLengthCm / 100.f, ARopeLine::MaxLengthCm / 100.f));
			return false;
		}
		bRopeAnchorSet = false;
		RemoveItem(EForestItem::Rope, 1);
		ShowMessage(FString::Printf(TEXT("İp hattı gerildi (%.0f m)  —  bir ucunda [E] ile kay ya da tırman"), FVector::Dist(NewLine->GetA(), NewLine->GetB()) / 100.f));
		return true;
	}
	default:
		return false;
	}
}

AStickStructure* AForestCharacter::PlantStakeAt(const FHitResult& Hit)
{
	if (GetItemCount(EForestItem::Stick) < 1)
	{
		ShowMessage(TEXT("Kazık için çubuk gerek: devrilen ağaçların dalları"));
		return nullptr;
	}
	const FVector Forward = FVector(GetActorForwardVector().X, GetActorForwardVector().Y, 0.f).GetSafeNormal();
	AStickStructure* Stake = AStickStructure::Plant(GetWorld(), Hit.ImpactPoint, Forward.Rotation().Yaw);
	if (!Stake)
	{
		return nullptr;
	}
	RemoveItem(EForestItem::Stick, 1);
	ShowMessage(TEXT("Kazık çakıldı  —  aralıkları dar dikilen kazıklar yolu kapatır; [F] ile yakılan kazık meşale olur"));
	return Stake;
}

bool AForestCharacter::TestReadNote(ARemnantNote* Note)
{
	if (!Note)
	{
		return false;
	}
	UScenarioSubsystem* Scenario = UScenarioSubsystem::Get(GetWorld());
	const bool bNew = Scenario && Scenario->RecordNote(Note->Title);
	ShowMessage(FString::Printf(TEXT("%s%s: %s"), bNew ? TEXT("[Yeni not] ") : TEXT(""), *Note->Title, *Note->Text));
	return bNew;
}

bool AForestCharacter::IsWaterAtHand() const
{
	const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, 90.f);
	const FVector Ahead = Feet + FVector(GetActorForwardVector().X, GetActorForwardVector().Y, 0.f).GetSafeNormal() * 150.f;
	return ARiverStrip::IsInWater(GetWorld(), Feet) || ARiverStrip::IsInWater(GetWorld(), Ahead);
}

int32 AForestCharacter::TestGather(ARawMaterialNode* Node)
{
	if (!Node)
	{
		return 0;
	}
	if (Node->IsDepleted())
	{
		ShowMessage(Node->GetPrompt());
		return 0;
	}
	FString Got;
	const int32 Added = Node->Harvest(this, Got);
	ShowMessage(Added > 0 ? Got : FString(TEXT("Çanta dolu; bir şey alınamadı")));
	return Added;
}

bool AForestCharacter::TestDrink()
{
	if (!IsWaterAtHand())
	{
		return false;
	}
	Survival->Drink(30.f);
	ShowMessage(TEXT("Su içildi"));
	return true;
}

bool AForestCharacter::TestRideRope(ARopeLine* Line)
{
	if (!Line)
	{
		return false;
	}
	bool bAtB = false;
	Line->EndNear(GetActorLocation(), bAtB);
	RideRope = Line;
	RideFrom = bAtB ? Line->GetB() : Line->GetA();
	RideTo = bAtB ? Line->GetA() : Line->GetB();
	RideT = 0.f;
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	ShowMessage(TEXT("İpte..."));
	return true;
}

void AForestCharacter::UpdateRide(float DeltaSeconds)
{
	if (!RideRope.IsValid())
	{
		return;
	}
	const float Length = FMath::Max(1.f, FVector::Dist(RideFrom, RideTo));
	RideT = FMath::Min(1.f, RideT + DeltaSeconds * ARopeLine::RideSpeed / Length);
	SetActorLocation(FMath::Lerp(RideFrom, RideTo, RideT) + FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 5.f), false, nullptr, ETeleportType::TeleportPhysics);
	if (RideT >= 1.f)
	{
		RideRope.Reset();
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

void AForestCharacter::EatFood()
{
	const FString Eaten = FarmKit::Eat(this);
	ShowMessage(Eaten.IsEmpty() ? FString(TEXT("Yiyecek yok: tahıl ek, hayvan besle ya da avla")) : FString::Printf(TEXT("Yendi: %s"), *Eaten));
}

void AForestCharacter::ToggleStickMode()
{
	bStickMode = !bStickMode;
	ShowMessage(bStickMode ? GetStickModeLabel() : FString(TEXT("Çubuk modu kapandı")));
}

FString AForestCharacter::GetStickModeLabel() const
{
	FString Cost;
	switch (StickBuildIndex)
	{
	case CraftStake:    Cost = FString::Printf(TEXT("1 çubuk, çantada %d"), GetItemCount(EForestItem::Stick)); break;
	case CraftField:    Cost = FString::Printf(TEXT("2 çubuk, çantada %d"), GetItemCount(EForestItem::Stick)); break;
	case CraftGarment:  Cost = FString::Printf(TEXT("%d deri, çantada %d"), FarmKit::HidesPerGarment, GetItemCount(EForestItem::Hide)); break;
	case CraftRope:     Cost = FString::Printf(TEXT("3 lif / 2 asma / 2 yün / 1 deri; lif %d, asma %d, yün %d, deri %d"),
		GetItemCount(EForestItem::PlantFiber), GetItemCount(EForestItem::Vine), GetItemCount(EForestItem::Wool), GetItemCount(EForestItem::Hide)); break;
	default:            Cost = FString::Printf(TEXT("1 ip, çantada %d; %s"), GetItemCount(EForestItem::Rope), bRopeAnchorSet ? TEXT("ikinci ucu seç") : TEXT("birinci ucu seç")); break;
	}
	return FString::Printf(TEXT("Yapım: %s (%s)  —  [Q] değiştir, [Sol tık] yap, [I] kapat"), *CraftEntryName(StickBuildIndex), *Cost);
}

bool AForestCharacter::StickBuildSpot(FHitResult& OutHit) const
{
	return TraceFromView(500.f, OutHit, true) && OutHit.ImpactNormal.Z > 0.7f && OutHit.GetActor() && !OutHit.GetActor()->IsA<ACarvableLog>();
}

void AForestCharacter::TestDropStone()
{
	if (StoneCarry && StoneCarry->IsCarrying())
	{
		StoneCarry->Drop();
		RefreshToolVisibility();
	}
}

void AForestCharacter::TestUseUpSticks()
{
	RemoveItem(EForestItem::Stick, GetItemCount(EForestItem::Stick));
}

void AForestCharacter::TestGiveSticks(int32 Count)
{
	AddItem(EForestItem::Stick, Count);
}

bool AForestCharacter::IsCarryingStone() const
{
	return StoneCarry && StoneCarry->IsCarrying();
}

bool AForestCharacter::TestPickUpResource(AActor* Resource)
{
	if (!Resource)
	{
		return false;
	}
	if (AStoneActor* Stone = Cast<AStoneActor>(Resource))
	{
		if (LooseResources::IsPocketStone(Stone))
		{
			if (AddItem(EForestItem::Stone, 1) != 1)
			{
				return false;
			}
			Stone->Destroy();
			return true;
		}
		const UStoneDefinition* Definition = Stone->GetDefinition();
		const EStoneCarryMode Mode = Definition && Definition->Mass <= StoneCarry->MaxSingleHandMass ? EStoneCarryMode::SingleHand : EStoneCarryMode::TwoHands;
		return CarriedItem == EForestItem::None && StoneCarry && !StoneCarry->IsCarrying() && StoneCarry->PickUp(Stone, Mode);
	}
	if (AStickActor* Stick = Cast<AStickActor>(Resource))
	{
		if (AddItem(EForestItem::Stick, 1) != 1)
		{
			return false;
		}
		Stick->Destroy();
		return true;
	}
	return false;
}

void AForestCharacter::DebugRaid()
{
	if (RequireAdmin())
	{
		ShowMessage(RaidKit::StartDefaultRaid(GetWorld()) ? TEXT("Baskın başladı") : TEXT("Baskın başlamadı"));
	}
}

void AForestCharacter::DebugSkipTime()
{
	if (!RequireAdmin())
	{
		return;
	}
	if (UDayLoopSubsystem* Loop = UDayLoopSubsystem::Get(GetWorld()))
	{
		Loop->SkipHours(3.f);
		ShowMessage(FString::Printf(TEXT("Zaman ileri: gün %d, %02d:00"), Loop->GetDay(), FMath::FloorToInt(Loop->GetHour())));
	}
}

void AForestCharacter::DebugHaulers()
{
	if (!RequireAdmin())
	{
		return;
	}
	const FVector Forward = FVector(GetActorForwardVector().X, GetActorForwardVector().Y, 0.f).GetSafeNormal();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);
	const FVector Base = GetActorLocation() - FVector(0.f, 0.f, 90.f) + Forward * 450.f;
	const EHaulKind Kinds[] = { EHaulKind::Cart, EHaulKind::Sled, EHaulKind::Rack };
	for (int32 Index = 0; Index < 3; ++Index)
	{
		AHaulCart::Spawn(GetWorld(), Base + Right * (Index - 1) * 380.f, Forward.Rotation().Yaw + 90.f, Kinds[Index]);
	}
	ShowMessage(TEXT("El arabası, kızak ve sehpa konuldu  —  parçayla bakıp [Sol tık], indirmek için [E], itmek için [X]"));
}

bool AForestCharacter::TestPlaceSlope(const FHitResult& Hit, float AngleDeg)
{
	bPlaceStanding = false;
	bPlaceSlope = true;
	SlopeAngle = AngleDeg;
	UpdateCarryPlacementAt(Hit, true);
	const bool bPlaced = PlaceCarried();
	bPlaceSlope = false;
	return bPlaced;
}

bool AForestCharacter::TestPlace(bool bStanding, const FHitResult& Hit)
{
	bPlaceSlope = false;
	bPlaceStanding = bStanding;
	UpdateCarryPlacementAt(Hit, true);
	return PlaceCarried();
}

void AForestCharacter::DropOne()
{
	const FVector Forward = FVector(GetActorForwardVector().X, GetActorForwardVector().Y, 0.f).GetSafeNormal();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);
	if (CarriedItem == EForestItem::TrunkLog)
	{
		StopRopeDrag();
	}
	else if (CarriedItem == EForestItem::Log && CarriedLogs.Num() > 0)
	{
		if (ACarvableLog* Piece = CarriedLogs.Pop().Get())
		{
			const FVector Ahead = GetActorLocation() + Forward * 140.f + FVector(0, 0, 20.f + CarriedLogs.Num() * 30.f);
			Piece->PlaceAt(Piece->GetLayTransform(Ahead, Right.Rotation().Yaw), /*bAnchorIt*/ false);
		}
		if (CarriedLogs.Num() == 0)
		{
			CarriedItem = EForestItem::None;
		}
		ArrangeShoulder();
	}
	else if (AHarvestableActor* BigLog = ShoulderedLog.Get())
	{
		BigLog->PlaceOnGround(GetActorLocation() + Forward * 150.f, Right);
		ShoulderedLog = nullptr;
		CarriedItem = EForestItem::None;
	}
	RefreshToolVisibility();
}

void AForestCharacter::DropCarried()
{
	while (IsHandsBusy())
	{
		const int32 Before = CarriedLogs.Num();
		const bool bHadRope = RopedLog.IsValid();
		const bool bHadBig = ShoulderedLog.IsValid();
		DropOne();
		if (Before == CarriedLogs.Num() && bHadRope == RopedLog.IsValid() && bHadBig == ShoulderedLog.IsValid())
		{
			CarriedItem = EForestItem::None;
			break;
		}
	}
	RefreshToolVisibility();
}

bool AForestCharacter::ConsumeCarried(EForestItem Item, int32 Count)
{
	if (Item == EForestItem::Log && CarriedItem == EForestItem::Log && GetItemCount(EForestItem::Log) >= Count)
	{
		// A wall or a recipe takes whole, full-length logs; cut pieces stay on the shoulder.
		for (int32 Index = CarriedLogs.Num() - 1; Index >= 0 && Count > 0; --Index)
		{
			ACarvableLog* Piece = CarriedLogs[Index].Get();
			if (Piece && Piece->IsFullLog())
			{
				CarriedLogs.RemoveAt(Index);
				Piece->Destroy();
				--Count;
			}
		}
		if (CarriedLogs.Num() == 0)
		{
			CarriedItem = EForestItem::None;
		}
		ArrangeShoulder();
		RefreshToolVisibility();
		return true;
	}
	if (Item == EForestItem::BigLog && CarriedItem == EForestItem::BigLog && Count == 1)
	{
		if (AHarvestableActor* BigLog = ShoulderedLog.Get())
		{
			BigLog->Destroy();
		}
		ShoulderedLog = nullptr;
		CarriedItem = EForestItem::None;
		RefreshToolVisibility();
		return true;
	}
	return false;
}

void AForestCharacter::UpdateCarry(float DeltaSeconds)
{
	// Drop references to anything destroyed by another path.
	CarriedLogs.RemoveAll([](const TWeakObjectPtr<ACarvableLog>& Log) { return !Log.IsValid(); });
	if (CarriedItem == EForestItem::Log && CarriedLogs.Num() == 0)
	{
		CarriedItem = EForestItem::None;
		RefreshToolVisibility();
	}
	if (CarriedItem == EForestItem::BigLog && !ShoulderedLog.IsValid())
	{
		CarriedItem = EForestItem::None;
		RefreshToolVisibility();
	}
	if (CarriedItem == EForestItem::TrunkLog && !RopedLog.IsValid())
	{
		StopRopeDrag();
	}
	if (!IsHandsBusy())
	{
		return;
	}

	// Loads cost stamina while walking (GDD 6.1).
	if (GetVelocity().SizeSquared2D() > 100.f)
	{
		float Cost = 0.f;
		if (CarriedItem == EForestItem::TrunkLog)
		{
			Cost = 6.f;
		}
		else if (CarriedItem == EForestItem::BigLog)
		{
			Cost = 5.f;
		}
		else
		{
			Cost = 1.f + 3.f * GetCarryLoad();
		}
		Survival->TryUseStamina(Cost * DeltaSeconds);
	}

	// The rope only pulls once it is taut; the far end drags along the ground.
	if (AHarvestableActor* Trunk = RopedLog.Get())
	{
		const FVector Forward = FVector(GetActorForwardVector().X, GetActorForwardVector().Y, 0.f).GetSafeNormal();
		const FVector Hitch = GetActorLocation() - Forward * 30.f;
		FVector ToHitch = Hitch - RopeFront;
		ToHitch.Z = 0.f;
		const float Taut = RopeLength * 0.9f;
		if (ToHitch.Size() > Taut)
		{
			RopeFront += ToHitch.GetSafeNormal() * (ToHitch.Size() - Taut);
		}
		RopeFront.Z = RopeEndZ();
		Trunk->DragTo(RopeFront);
		if (Rope)
		{
			if (const UStaticMeshComponent* TrunkMesh = Trunk->FindComponentByClass<UStaticMeshComponent>())
			{
				Rope->EndLocation = TrunkMesh->GetComponentTransform().InverseTransformPosition(RopeFront);
			}
		}
	}
}

void AForestCharacter::ToggleAdminMode()
{
	bAdminMode = !bAdminMode;
	ShowMessage(bAdminMode ? TEXT("Admin modu A\u00C7IK") : TEXT("Admin modu kapal\u0131"));
}

bool AForestCharacter::RequireAdmin()
{
	if (!bAdminMode)
	{
		ShowMessage(TEXT("Admin modu kapal\u0131  \u2014  [P] ile a\u00E7"));
	}
	return bAdminMode;
}

void AForestCharacter::DebugStarve()
{
	if (!RequireAdmin())
	{
		return;
	}
	Survival->Eat(-1000.f);
	Survival->Drink(-1000.f);
	ShowMessage(TEXT("Test: a\u00E7l\u0131k ve susuzluk s\u0131f\u0131rland\u0131"));
}

void AForestCharacter::DebugHurt()
{
	if (!RequireAdmin())
	{
		return;
	}
	Survival->ApplyDamage(35.f);
}

bool AForestCharacter::TraceFromView(float Distance, FHitResult& OutHit, bool bIgnorePlans) const
{
	const FVector Start = Camera->GetComponentLocation();
	const FVector End = Start + Camera->GetForwardVector() * Distance;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ForestView), false, this);
	if (bIgnorePlans)
	{
		// Placing pieces looks through see-through wall plans; they are only filled with [E].
		for (TActorIterator<ALogWallBlueprint> It(GetWorld()); It; ++It)
		{
			Params.AddIgnoredComponent(It->GetGhosts());
		}
	}
	return GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, Params);
}

void AForestCharacter::UpdateFocus()
{
	FocusedHarvestable = nullptr;
	FocusedPickup = nullptr;
	FocusedRawNode = nullptr;
	FocusedBlueprint = nullptr;
	FocusedLogActor = nullptr;
	FocusedStoneActor = nullptr;
	FocusedCarvable = nullptr;

	FHitResult Hit;
	if (bBuildMode || !TraceFromView(ReachDistance, Hit))
	{
		return;
	}
	FocusHitPoint = Hit.ImpactPoint;
	if (ACarvableLog* Carvable = Cast<ACarvableLog>(Hit.GetActor()))
	{
		FocusedCarvable = Carvable;
		if (RawLogShapes::FromLog(Carvable).IsValid())
		{
			// Raw logs are worked as finished shapes by where they are hit: no cut guide, no aim.
			CarvePreview->SetVisibility(false);
			bCarveSpotValid = false;
		}
		else
		{
			UpdateCarveAim(Hit, Carvable);
		}
		if (CurrentTool != EForestTool::Axe && SwingTime < 0.f)
		{
			EquipTool(EForestTool::Axe);
		}
	}
	else if (ALogWallBlueprint* Blueprint = Cast<ALogWallBlueprint>(Hit.GetActor()))
	{
		FocusedBlueprint = Blueprint;
	}
	else if (ALogConstructionActor* LogPiece = Cast<ALogConstructionActor>(Hit.GetActor()))
	{
		FocusedLogActor = LogPiece;
	}
	else if (AStoneMasonryActor* StonePiece = Cast<AStoneMasonryActor>(Hit.GetActor()))
	{
		FocusedStoneActor = StonePiece;
	}
	else if (AHarvestableActor* Harvestable = Cast<AHarvestableActor>(Hit.GetActor()))
	{
		if (Harvestable->IsAlive())
		{
			FocusedHarvestable = Harvestable;
			// Pick the right tool for what we look at, but never mid-swing.
			const EForestTool Wanted = Harvestable->IsWood() ? EForestTool::Axe : EForestTool::Pickaxe;
			if (Wanted != CurrentTool && SwingTime < 0.f)
			{
				EquipTool(Wanted);
			}
		}
	}
	else if (ARawMaterialNode* RawNode = Cast<ARawMaterialNode>(Hit.GetActor()))
	{
		FocusedRawNode = RawNode;
	}
	else if (APickupItem* Pickup = Cast<APickupItem>(Hit.GetActor()))
	{
		if (Pickup->IsPickable())
		{
			FocusedPickup = Pickup;
			// A log can be carved with the axe: put the pickaxe away when looking at one.
			if (Pickup->GetItem() == EForestItem::Log && CurrentTool == EForestTool::Pickaxe && SwingTime < 0.f)
			{
				EquipTool(EForestTool::Axe);
			}
		}
	}
}

void AForestCharacter::PrimaryAction()
{
	if (bStickMode && !IsHandsBusy())
	{
		FHitResult Spot;
		if (StickBuildIndex == CraftRopeLine)
		{
			if (TraceFromView(600.f, Spot, true))
			{
				TestCraft(StickBuildIndex, Spot);
			}
		}
		else if (StickBuildIndex == CraftGarment || StickBuildIndex == CraftRope || StickBuildSpot(Spot))
		{
			TestCraft(StickBuildIndex, Spot);
		}
		else
		{
			ShowMessage(TEXT("Kazık ve tarla düz zemine  —  yere bak"));
		}
		return;
	}
	// Physical-stone placement takes priority over legacy log/build placement input.
	if (StonePlacement && StonePlacement->IsPlacing())
	{
		if (StonePlacement->ConfirmPlacement())
		{
			RefreshToolVisibility();
			GetCharacterMovement()->MaxWalkSpeed = CarrySpeed();
			ShowMessage(TEXT("Taş yerleştirildi."));
		}
		else
		{
			ShowMessage(TEXT("Taşı yerleştirmek için geçerli bir yüzeye nişan al."));
		}
		return;
	}

	if (StoneCarry && StoneCarry->IsCarrying())
	{
		if (StonePlacement && StonePlacement->BeginPlacement(StoneCarry->GetCarried()))
		{
			ShowMessage(TEXT("Taş yerleştirme: Q/tekerlek ile yüz-soket seç, R ile döndür, sol tık onayla."));
		}
		else
		{
			ShowMessage(TEXT("Taş yerleştirme başlatılamadı."));
		}
		return;
	}

	if (bBuildMode)
	{
		if (IsWallTool())
		{
			WallClick();
		}
		else
		{
			PlaceSelected();
		}
		return;
	}

	if (CarriedItem == EForestItem::Log)
	{
		PlaceCarried();
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastSwingTime < SwingCooldown)
	{
		return;
	}
	if (IsHandsBusy())
	{
		ShowMessage(TEXT("Elin dolu. [E] ile b\u0131rak."));
		return;
	}
	if (!Survival->TryUseStamina(SwingStaminaCost))
	{
		ShowMessage(TEXT("\u00C7ok yorgunsun"));
		return;
	}
	LastSwingTime = Now;
	SwingTime = 0.f;
	bPendingHit = true;
}

void AForestCharacter::PerformHit()
{
	FOVPunch = 3.f;

	FHitResult Hit;
	if (!TraceFromView(ReachDistance, Hit))
	{
		return;
	}
	if (ACarvableLog* Carvable = Cast<ACarvableLog>(Hit.GetActor()))
	{
		if (RawLogShapes::FromLog(Carvable).IsValid())
		{
			ApplyShapeHit(Carvable, Hit);
		}
		else
		{
			ApplyCarveHit(Carvable);
		}
		return;
	}
	if (AAnimalActor* Animal = Cast<AAnimalActor>(Hit.GetActor()))
	{
		if (FarmKit::Butcher(Animal, this))
		{
			ShowMessage(TEXT("Hayvan kesildi: et ve deri çantaya girdi"));
		}
		return;
	}
	if (ARaidEnemy* Enemy = Cast<ARaidEnemy>(Hit.GetActor()))
	{
		Enemy->TakeHit(25.f);
		ShowMessage(Enemy->IsAlive() ? TEXT("Düşmana vuruldu") : TEXT("Düşman öldü"));
		return;
	}
	if (CurrentTool == EForestTool::Trowel)
	{
		// Trowel applies chinking sealant to log walls, interior plaster, or mortar to stone foundations
		if (ALogWallBlueprint* Wall = Cast<ALogWallBlueprint>(Hit.GetActor()))
		{
			Wall->ApplyChinking();
			ShowMessage(TEXT("Kütük aralarına chinking çekildi! R-Değeri ve hava sızdırmazlığı arttı."));
			return;
		}
		if (ALogConstructionActor* LogPiece = Cast<ALogConstructionActor>(Hit.GetActor()))
		{
			if (!LogPiece->IsChinked())
			{
				LogPiece->ApplyChinking();
				ShowMessage(TEXT("Kütüğe chinking uygulandı! R-Değeri arttı, kayma direnci güçlendi."));
			}
			else if (!LogPiece->IsPlastered())
			{
				LogPiece->ApplyInteriorPlaster();
				ShowMessage(TEXT("İç duvara sıva (plaster) uygulandı! Yangın direnci ve R-1.5 eklendi."));
			}
			else
			{
				ShowMessage(TEXT("Bu kütük zaten tam yalıtımlı ve sıvalı."));
			}
			return;
		}
		if (AStoneMasonryActor* StonePiece = Cast<AStoneMasonryActor>(Hit.GetActor()))
		{
			StonePiece->ApplyMortar();
			ShowMessage(TEXT("Taş temele harç uygulandı! Harçlı taş duvar (Mortared Masonry) seviyesine geçti."));
			return;
		}

		ShowMessage(TEXT("Mala ile kütük duvarlara veya taş temele harç/sıva uygulayabilirsin."));
		return;
	}

	// A log lying on the ground goes on the "bench": it becomes a carvable log in place, ready for notches and cuts.
	if (APickupItem* Pickup = Cast<APickupItem>(Hit.GetActor()))
	{
		if (Pickup->GetItem() == EForestItem::Log && CurrentTool == EForestTool::Axe)
		{
			if (ACarvableLog::SpawnFromPickup(Pickup))
			{
				ShowMessage(TEXT("Kütük oymaya hazır  —  [R] araç seç, aynı yere 2 vuruş"));
			}
			return;
		}
	}

	if (AHarvestableActor* Harvestable = Cast<AHarvestableActor>(Hit.GetActor()))
	{
		const FVector Direction = (Hit.ImpactPoint - Camera->GetComponentLocation()).GetSafeNormal();
		// A raw log of up to 300 cm lying where it fell is worked like any building piece: it becomes one in place and the
		// hit cuts, splits, hews or channels it by where it lands. 600 cm logs only cut across, into two 300s.
		if (Harvestable->IsTrunk() && Harvestable->GetRawLogLength() > 0 && Harvestable->GetRawLogLength() <= AHarvestableActor::LongestShoulderLogCm)
		{
			FocusedHarvestable = nullptr;
			if (ACarvableLog* Piece = Harvestable->ConvertToCarvable())
			{
				ApplyShapeHit(Piece, Hit);
			}
			return;
		}
		if (Harvestable->ApplyHit(HitDamage, Hit, Direction))
		{
			ShowMessage(Harvestable->IsTrunk() ? TEXT("Kütük ikiye bölündü!") : Harvestable->IsTree() ? TEXT("Ağaç devriliyor!") : TEXT("Kaya parçalandı!"));
		}
	}
	else if (ALogWallBlueprint* Wall = Cast<ALogWallBlueprint>(Hit.GetActor()))
	{
		const FVector Direction = (Hit.ImpactPoint - Camera->GetComponentLocation()).GetSafeNormal();
		Wall->CollapseWall(Direction);
		ShowMessage(TEXT("Duvar çöktü! Kütükler devriliyor!"));
	}
	else if (ALogConstructionActor* LogPiece = Cast<ALogConstructionActor>(Hit.GetActor()))
	{
		const FVector Direction = (Hit.ImpactPoint - Camera->GetComponentLocation()).GetSafeNormal();
		LogPiece->ApplyStructuralDamage(HitDamage, Hit.ImpactPoint, Direction);
		ShowMessage(TEXT("Kütüğe darbe vuruldu!"));
	}
	else if (AStoneMasonryActor* StonePiece = Cast<AStoneMasonryActor>(Hit.GetActor()))
	{
		const FVector Direction = (Hit.ImpactPoint - Camera->GetComponentLocation()).GetSafeNormal();
		StonePiece->ApplyDamage(HitDamage, Direction);
		ShowMessage(TEXT("Taş temele darbe vuruldu!"));
	}
	else if (AOpeningBuckActor* BuckPiece = Cast<AOpeningBuckActor>(Hit.GetActor()))
	{
		const FVector Direction = (Hit.ImpactPoint - Camera->GetComponentLocation()).GetSafeNormal();
		BuckPiece->ApplyDamage(HitDamage, Direction);
		ShowMessage(TEXT("Açıklık kasasına darbe vuruldu!"));
	}
}

void AForestCharacter::Interact()
{
	ALogWallBlueprint* Blueprint = FocusedBlueprint.Get();
	APickupItem* FocusedLog = FocusedPickup.Get();

	if (StonePlacement && StonePlacement->IsPlacing())
	{
		ShowMessage(TEXT("Taş yerleştirme açık. Sol tık onaylar, sağ tık iptal eder."));
		return;
	}

	// A physical stone is a world actor, not an item count in the legacy backpack.
	if (StoneCarry && StoneCarry->IsCarrying())
	{
		if (StoneCarry->Drop())
		{
			RefreshToolVisibility();
			GetCharacterMovement()->MaxWalkSpeed = CarrySpeed();
			ShowMessage(TEXT("Taş yere bırakıldı."));
		}
		return;
	}

	// Untying the rope comes first: it is the only thing E does while dragging a trunk.
	if (CarriedItem == EForestItem::TrunkLog)
	{
		StopRopeDrag();
		return;
	}
	// A rope end within reach: ride it. A note in sight: read it. Water: drink.
	if (CarriedItem == EForestItem::None && !IsRiding())
	{
		for (TActorIterator<ARopeLine> It(GetWorld()); It; ++It)
		{
			bool bAtB = false;
			if (It->EndNear(GetActorLocation() - FVector(0.f, 0.f, 90.f), bAtB))
			{
				TestRideRope(*It);
				return;
			}
		}
		FHitResult NoteHit;
		if (TraceFromView(400.f, NoteHit, true))
		{
			if (ARemnantNote* Note = Cast<ARemnantNote>(NoteHit.GetActor()))
			{
				TestReadNote(Note);
				return;
			}
		}
		if (IsWaterAtHand())
		{
			TestDrink();
			return;
		}
	}
	// A field: sow it, or bring in a ripe crop. An animal: take what it has made. A raw material source: gather.
	if (CarriedItem == EForestItem::None)
	{
		FHitResult FarmHit;
		if (TraceFromView(400.f, FarmHit, true))
		{
			if (ARawMaterialNode* RawNode = Cast<ARawMaterialNode>(FarmHit.GetActor()))
			{
				TestGather(RawNode);
				return;
			}
			if (ACropFieldActor* Field = Cast<ACropFieldActor>(FarmHit.GetActor()))
			{
				if (FarmKit::IsRipe(Field))
				{
					const int32 Grain = FarmKit::Harvest(Field, this);
					ShowMessage(FString::Printf(TEXT("Hasat: +%d tahıl, +%d tohum"), Grain, FarmKit::SeedsPerField + 2));
				}
				else if (!FarmKit::IsSown(Field))
				{
					ShowMessage(FarmKit::Sow(Field, this) ? FString(TEXT("Tarla ekildi: günler geçtikçe büyür"))
						: FString::Printf(TEXT("Ekmek için %d tohum gerek (%d var)"), FarmKit::SeedsPerField, GetItemCount(EForestItem::Seed)));
				}
				else
				{
					ShowMessage(TEXT("Ekin büyüyor  —  henüz olgun değil"));
				}
				return;
			}
			if (AAnimalActor* Animal = Cast<AAnimalActor>(FarmHit.GetActor()))
			{
				const int32 Got = FarmKit::CollectProducts(Animal, this);
				ShowMessage(Got > 0 ? FString::Printf(TEXT("+%d ürün (yumurta / yün)"), Got) : FString(TEXT("Şimdilik verecek bir şeyi yok")));
				return;
			}
		}
	}
	// A loaded cart, sled or rack in sight: E takes the last piece off it.
	if (CarriedItem == EForestItem::None)
	{
		FHitResult CartHit;
		if (TraceFromView(400.f, CartHit, true))
		{
			if (AHaulCart* Cart = Cast<AHaulCart>(CartHit.GetActor()); Cart && Cart->GetLoadCount() > 0)
			{
				TestUnload(Cart);
				return;
			}
		}
	}

	// Pick up the same physical stone actor that will later be installed in the world.
	if (CarriedItem == EForestItem::None && StoneCarry && !StoneCarry->IsCarrying())
	{
		FHitResult StoneHit;
		if (TraceFromView(400.f, StoneHit, true))
		{
			if (AStoneActor* Stone = Cast<AStoneActor>(StoneHit.GetActor()))
			{
				UStoneDefinition* Definition = Stone->GetDefinition();
				// A small stone goes into the pack (fire rings, stone blocks); a building stone is carried and placed.
				if (Definition && LooseResources::IsPocketStone(Stone))
				{
					if (!CanAddItem(EForestItem::Stone, 1) || AddItem(EForestItem::Stone, 1) != 1)
					{
						ShowMessage(TEXT("Çanta dolu; taş yerde kaldı."));
						return;
					}
					Stone->Destroy();
					ShowMessage(FString::Printf(TEXT("+1 Taş (%d)"), GetItemCount(EForestItem::Stone)));
					return;
				}
				if (!Definition)
				{
					ShowMessage(TEXT("Bu taşın tanımı eksik; güvenli biçimde alınamıyor."));
					return;
				}

				const EStoneCarryMode Mode = Definition->Mass <= StoneCarry->MaxSingleHandMass
					? EStoneCarryMode::SingleHand
					: EStoneCarryMode::TwoHands;
				if (StoneCarry->PickUp(Stone, Mode))
				{
					RefreshToolVisibility();
					GetCharacterMovement()->MaxWalkSpeed = CarrySpeed();
					ShowMessage(TEXT("Taş alındı. Sol tıkla yerleştirme önizlemesini başlat."));
				}
				else
				{
					ShowMessage(TEXT("Bu taş çok ağır veya şu anda alınamıyor."));
				}
				return;
			}

			if (AStickActor* Stick = Cast<AStickActor>(StoneHit.GetActor()))
			{
				UStickDefinition* StickDefinition = Stick->GetDefinition();
				if (!StickDefinition ||
					Stick->GetStructuralState_Implementation() != EStructuralNodeState::Loose ||
					StickDefinition->Shape != EStickShape::Raw)
				{
					ShowMessage(TEXT("Yalnızca serbest ve işlenmemiş çubuk stoğa alınabilir."));
					return;
				}
				if (!CanAddItem(EForestItem::Stick, 1) || AddItem(EForestItem::Stick, 1) != 1)
				{
					ShowMessage(TEXT("Çanta dolu; çubuk dünyada bırakıldı."));
					return;
				}
				Stick->Destroy();
				ShowMessage(TEXT("+1 Çubuk"));
				return;
			}
		}
	}
	// Any piece of wood onto the shoulder, as long as its weight fits (a log item lying about becomes a piece first).
	if (CarriedItem == EForestItem::None || CarriedItem == EForestItem::Log)
	{
		ACarvableLog* Piece = FocusedCarvable.Get();
		if (!Piece && FocusedLog && FocusedLog->GetItem() == EForestItem::Log)
		{
			Piece = ACarvableLog::SpawnFromPickup(FocusedLog);
		}
		if (Piece)
		{
			StartCarry(Piece);
			return;
		}
	}
	// A carried log goes into the wall plan it is aimed at, otherwise E puts one down.
	if (IsHandsBusy() && !(Blueprint && CarriedItem == EForestItem::Log))
	{
		DropOne();
		return;
	}

	// Big logs on the ground: half a trunk goes on the shoulder, a whole trunk is dragged by rope.
	if (AHarvestableActor* BigLog = FocusedHarvestable.Get())
	{
		// Raw logs from a felled tree: up to 300 cm they become building pieces on the shoulder (one 300, two 150 or
		// four 75 cm logs fill it); 600 cm is too heavy and is dragged by rope.
		if (BigLog->IsTrunk() && BigLog->GetRawLogLength() > 0 && BigLog->GetRawLogLength() <= AHarvestableActor::LongestShoulderLogCm)
		{
			FocusedHarvestable = nullptr;
			if (ACarvableLog* Piece = BigLog->ConvertToCarvable())
			{
				StartCarry(Piece);
			}
			return;
		}
		if (BigLog->IsTrunk())
		{
			if (BigLog->GetLogItem() == EForestItem::BigLog)
			{
				ShoulderBigLog(BigLog);
			}
			else
			{
				StartRopeDrag(BigLog);
			}
			return;
		}
	}

	if (Blueprint)
	{
		if (!RemoveItem(EForestItem::Log, 1))
		{
			ShowMessage(TEXT("Omzunda k\u00FCt\u00FCk yok. Bir k\u00FCt\u00FC\u011Fe bak\u0131p [E] ile omzuna al."));
			return;
		}
		Blueprint->AddLog();
		ShowMessage(Blueprint->IsComplete()
			? FString(TEXT("Duvar tamamlandı!"))
			: FString::Printf(TEXT("Duvar: %d / %d"), Blueprint->GetFilled(), Blueprint->GetTotal()));
		return;
	}

	APickupItem* Pickup = FocusedPickup.Get();
	if (!Pickup)
	{
		return;
	}
	if (ForestItems::IsCarriedItem(Pickup->GetItem()))
	{
		StartCarry(ACarvableLog::SpawnFromPickup(Pickup));
		return;
	}
	if (!CanAddItem(Pickup->GetItem(), Pickup->GetCount()))
	{
		ShowMessage(TEXT("\u00C7anta dolu"));
		return;
	}
	const int32 PickupCount = Pickup->GetCount();
	const int32 AddedCount = AddItem(Pickup->GetItem(), PickupCount);
	if (AddedCount != PickupCount)
	{
		ShowMessage(TEXT("Alım tamamlanamadı; dünyadaki kaynak korunuyor."));
		return;
	}
	ShowMessage(FString::Printf(TEXT("+%d %s"), PickupCount, *ForestItems::GetDisplayName(Pickup->GetItem())));
	Pickup->Destroy();
	FocusedPickup = nullptr;
}

AActor* AForestCharacter::DropInventoryItem(EForestItem Item, int32 Count)
{
	if (!CoreInventory || !GetWorld() || Count <= 0 || Item == EForestItem::None ||
		ForestItems::IsCarriedItem(Item))
	{
		return nullptr;
	}
	const FName ItemID = ForestItems::GetInventoryID(Item);
	if (ItemID.IsNone() || !CoreInventory->CanDropItem(ItemID, Count))
	{
		return nullptr;
	}

	const FVector Forward = FVector(GetActorForwardVector().X, GetActorForwardVector().Y, 0.0f).GetSafeNormal();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();
	const FVector BaseLocation = GetActorLocation() + Forward * 105.0f + FVector(0.0f, 0.0f, 35.0f);
	TArray<AActor*> SpawnedActors;
	SpawnedActors.Reserve((Item == EForestItem::Stone || Item == EForestItem::Stick) ? Count : 1);

	auto RollbackSpawnedActors = [&SpawnedActors]()
	{
		for (AActor* Spawned : SpawnedActors)
		{
			if (IsValid(Spawned))
			{
				Spawned->Destroy();
			}
		}
	};

	if (Item == EForestItem::Stone)
	{
		UStaticMesh* StoneMesh = ForestItems::GetMesh(Item);
		if (!StoneMesh)
		{
			return nullptr;
		}

		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FVector Location = BaseLocation + Right * (static_cast<float>(Index) * 42.0f);
			AStoneActor* Stone = GetWorld()->SpawnActor<AStoneActor>(
				AStoneActor::StaticClass(), Location, FRotator::ZeroRotator);
			if (!Stone)
			{
				RollbackSpawnedActors();
				return nullptr;
			}

			UStoneDefinition* Definition = NewObject<UStoneDefinition>(Stone, TEXT("InventoryStoneDefinition"));
			if (!Definition)
			{
				Stone->Destroy();
				RollbackSpawnedActors();
				return nullptr;
			}
			Definition->StoneName = TEXT("InventoryStone");
			Definition->Type = EStoneType::Granite;
			Definition->Shape = EStoneShape::Small;
			Definition->Role = EStoneRole::None;
			Definition->Dimensions = FVector(13.0f, 10.0f, 9.0f);
			Definition->Mass = 2.0f;
			Definition->Volume = 0.0002f;
			Definition->CompressiveStrength = 180.0f;
			Definition->TensileStrength = 15.0f;
			Definition->Mesh = TSoftObjectPtr<UStaticMesh>(
				FSoftObjectPath(TEXT("/Game/Greybox/Meshes/SM_GB_Stone.SM_GB_Stone")));
			Stone->InitializeFromDefinition(Definition);
			if (Stone->MeshComponent && !Stone->MeshComponent->GetStaticMesh())
			{
				Stone->MeshComponent->SetStaticMesh(StoneMesh);
			}
			if (!Stone->MeshComponent || !Stone->MeshComponent->GetStaticMesh())
			{
				Stone->Destroy();
				RollbackSpawnedActors();
				return nullptr;
			}
			SpawnedActors.Add(Stone);
		}
	}
	else if (Item == EForestItem::Stick)
	{
		UStaticMesh* StickMesh = ForestItems::GetMesh(Item);
		if (!StickMesh)
		{
			return nullptr;
		}

		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FVector Location = BaseLocation + Right * (static_cast<float>(Index) * 12.0f);
			AStickActor* Stick = GetWorld()->SpawnActor<AStickActor>(
				AStickActor::StaticClass(), Location, GetActorRotation());
			if (!Stick)
			{
				RollbackSpawnedActors();
				return nullptr;
			}

			UStickDefinition* Definition = NewObject<UStickDefinition>(Stick, TEXT("InventoryStickDefinition"));
			if (!Definition)
			{
				Stick->Destroy();
				RollbackSpawnedActors();
				return nullptr;
			}
			Definition->StickName = TEXT("InventoryStick");
			Definition->Shape = EStickShape::Raw;
			Definition->Size = EStickSize::Short;
			Definition->Role = EStickRole::Crafting;
			Definition->Length = 55.0f;
			Definition->Diameter = 2.8f;
			Definition->Mass = 0.2f;
			Definition->Volume = 0.0006f;
			Definition->Durability = 30.0f;
			Definition->MaxStackSize = StackSize;
			Definition->Mesh = TSoftObjectPtr<UStaticMesh>(
				FSoftObjectPath(TEXT("/Game/Greybox/Meshes/SM_GB_Stick.SM_GB_Stick")));
			Stick->InitializeFromDefinition(Definition);
			if (Stick->MeshComponent && !Stick->MeshComponent->GetStaticMesh())
			{
				Stick->MeshComponent->SetStaticMesh(StickMesh);
			}
			if (!Stick->MeshComponent || !Stick->MeshComponent->GetStaticMesh())
			{
				Stick->Destroy();
				RollbackSpawnedActors();
				return nullptr;
			}
			SpawnedActors.Add(Stick);
		}
	}
	else
	{
		const FTransform DropTransform(FRotator(0.0f, GetActorRotation().Yaw, 0.0f), BaseLocation);
		APickupItem* Pickup = APickupItem::Spawn(
			GetWorld(), Item, Count, DropTransform, Forward * 160.0f + FVector(0.0f, 0.0f, 90.0f));
		if (!Pickup)
		{
			return nullptr;
		}
		SpawnedActors.Add(Pickup);
	}

	// One inventory debit commits the entire drop. If any physical spawn or the debit
	// fails, every temporary actor is removed before returning.
	if (CoreInventory->DropItem(ItemID, Count) != EInventoryResult::Success)
	{
		RollbackSpawnedActors();
		return nullptr;
	}
	return SpawnedActors.IsEmpty() ? nullptr : SpawnedActors[0];
}

int32 AForestCharacter::GetItemCount(EForestItem Item) const
{
	if (ForestItems::IsCarriedItem(Item))
	{
		// A trunk on a sled cannot be built with directly.
		if (Item == EForestItem::Log)
		{
			int32 Whole = 0;
			for (const TWeakObjectPtr<ACarvableLog>& Piece : CarriedLogs)
			{
				Whole += Piece.IsValid() && Piece->IsFullLog() ? 1 : 0;
			}
			return CarriedItem == EForestItem::Log ? Whole : 0;
		}
		return Item == EForestItem::TrunkLog ? 0 : (CarriedItem == Item ? GetCarriedCount() : 0);
	}

	const FName ItemID = ForestItems::GetInventoryID(Item);
	return CoreInventory && !ItemID.IsNone() ? CoreInventory->GetItemCount(ItemID) : 0;
}

int32 AForestCharacter::GetUsedSlots() const
{
	return CoreInventory ? CoreInventory->GetUsedSlotCount() : 0;
}

bool AForestCharacter::CanAddItem(EForestItem Item, int32 Count) const
{
	if (!CoreInventory || Count <= 0 || Item == EForestItem::None ||
		ForestItems::IsCarriedItem(Item))
	{
		return false;
	}
	const FName ItemID = ForestItems::GetInventoryID(Item);
	return !ItemID.IsNone() && CoreInventory->CanAddItem(ItemID, Count);
}

int32 AForestCharacter::AddItem(EForestItem Item, int32 Count)
{
	if (!CanAddItem(Item, Count))
	{
		return 0;
	}
	const FName ItemID = ForestItems::GetInventoryID(Item);
	return CoreInventory->AddItem(ItemID, Count) == EInventoryResult::Success ? Count : 0;
}

void AForestCharacter::GetInventorySlots(TArray<TPair<EForestItem, int32>>& OutSlots) const
{
	OutSlots.Reset();
	for (const EForestItem Item : ForestItems::GetAllItems())
	{
		if (ForestItems::IsCarriedItem(Item))
		{
			continue;
		}
		const int32 Count = GetItemCount(Item);
		for (int32 Left = Count; Left > 0; Left -= StackSize)
		{
			OutSlots.Emplace(Item, FMath::Min(Left, StackSize));
		}
	}
}

void AForestCharacter::ToggleInventory()
{
	bInventoryOpen = !bInventoryOpen;
}

bool AForestCharacter::RemoveItem(EForestItem Item, int32 Count)
{
	if (Count <= 0 || Item == EForestItem::None)
	{
		return false;
	}
	if (ForestItems::IsCarriedItem(Item))
	{
		return ConsumeCarried(Item, Count);
	}
	const FName ItemID = ForestItems::GetInventoryID(Item);
	return CoreInventory && !ItemID.IsNone() &&
		CoreInventory->RemoveItem(ItemID, Count) == EInventoryResult::Success;
}

void AForestCharacter::Craft(EForestItem Input, int32 InputCount, EForestItem Output, int32 OutputCount)
{
	if (InputCount <= 0 || OutputCount <= 0 || GetItemCount(Input) < InputCount)
	{
		ShowMessage(FString::Printf(TEXT("Yetersiz malzeme: %d %s gerekli"),
			InputCount, *ForestItems::GetDisplayName(Input)));
		return;
	}
	if (!CoreInventory)
	{
		ShowMessage(TEXT("Envanter sistemi hazır değil."));
		return;
	}

	if (ForestItems::IsCarriedItem(Input))
	{
		// Physical log consumption is separate from stack inventory. Preflight the
		// output first; no callback is emitted between preflight and the synchronous
		// actor removal/addition on this game-thread transaction.
		if (!CanAddItem(Output, OutputCount))
		{
			ShowMessage(TEXT("Çanta dolu"));
			return;
		}
		if (!ConsumeCarried(Input, InputCount))
		{
			ShowMessage(TEXT("Kütük işlemi tamamlanamadı; malzeme korunuyor."));
			return;
		}
		if (AddItem(Output, OutputCount) != OutputCount)
		{
			UE_LOG(LogTemp, Error, TEXT("Phase 6: output inventory preflight drifted while consuming physical wood."));
			ShowMessage(TEXT("Üretim işlemi durduruldu; envanter durumu kontrol edilmeli."));
			return;
		}
	}
	else
	{
		const FName InputID = ForestItems::GetInventoryID(Input);
		const FName OutputID = ForestItems::GetInventoryID(Output);
		if (InputID.IsNone() || OutputID.IsNone() ||
			!CoreInventory->ExchangeItems(InputID, InputCount, OutputID, OutputCount))
		{
			ShowMessage(TEXT("Üretim reddedildi: kapasite veya kaynak durumu değişti."));
			return;
		}
	}
	ShowMessage(FString::Printf(TEXT("%d %s -> %d %s"), InputCount,
		*ForestItems::GetDisplayName(Input), OutputCount, *ForestItems::GetDisplayName(Output)));
}

void AForestCharacter::CraftBeam()
{
	Craft(EForestItem::Log, 1, EForestItem::Beam, 2);
}

void AForestCharacter::CraftFirewood()
{
	Craft(EForestItem::Log, 1, EForestItem::Firewood, 4);
}

void AForestCharacter::CraftStoneBlock()
{
	Craft(EForestItem::Stone, 3, EForestItem::StoneBlock, 1);
}

void AForestCharacter::CraftHalfLog()
{
	Craft(EForestItem::Log, 1, EForestItem::HalfLog, 2);
}

void AForestCharacter::CraftShortLog()
{
	Craft(EForestItem::Log, 1, EForestItem::ShortLog, 3);
}

void AForestCharacter::CraftHollowLog()
{
	Craft(EForestItem::Log, 1, EForestItem::HollowLog, 1);
}

void AForestCharacter::CraftPlank()
{
	Craft(EForestItem::HalfLog, 1, EForestItem::Plank, 2);
}

void AForestCharacter::ToggleBuildMode()
{
	if (StonePlacement && StonePlacement->IsPlacing())
	{
		ShowMessage(TEXT("Önce taş yerleştirmesini onayla veya iptal et."));
		return;
	}
	if (!bBuildMode && StoneCarry && StoneCarry->IsCarrying())
	{
		ShowMessage(TEXT("Fiziksel taşı yerleştirme modundayken klasik inşa moduna geçemezsin."));
		return;
	}

	bBuildMode = !bBuildMode;
	BuildPreview->SetVisibility(false);
	RefreshToolVisibility();
	WallPreview->ClearInstances();
	bWallStartSet = false;
	if (!bBuildMode)
	{
		ShowMessage(TEXT("\u0130n\u015Fa modu kapand\u0131"));
		return;
	}
	// Open on single pieces (logs first) when the player carries any; the wall plan comes last.
	if (IsWallTool() || GetItemCount(SelectedBuildItem) <= 0)
	{
		SelectBuildItem(EForestItem::None);
		SelectNextAvailable(1);
	}
	ShowMessage(TEXT("\u0130n\u015Fa modu: tekerlek ile se\u00E7, sol t\u0131k yerle\u015Ftir, sa\u011F t\u0131k iptal"));
}

void AForestCharacter::DebugGiveMaterials()
{
	if (!RequireAdmin())
	{
		return;
	}
	// Logs never go to the backpack: lay some out in front of the player instead.
	const FVector Forward = FVector(GetActorForwardVector().X, GetActorForwardVector().Y, 0.f).GetSafeNormal();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);
	const FVector Base = GetActorLocation() + Forward * 300.f;
	for (int32 Index = 0; Index < 4; ++Index)
	{
		ACarvableLog::SpawnLog(GetWorld(), FTransform(Right.Rotation(), Base + Forward * (Index * 80.f) + FVector(0, 0, 40.f)));
	}
	for (const int32 Stage : { 2, 1 })
	{
		const FVector Center = Base + Forward * (Stage == 2 ? 500.f : 750.f);
		if (AHarvestableActor* BigLog = GetWorld()->SpawnActor<AHarvestableActor>(AHarvestableActor::StaticClass(), FTransform(Center)))
		{
			BigLog->InitLog(Stage, Center, Right);
		}
	}
	AddItem(EForestItem::HalfLog, 4);
	AddItem(EForestItem::ShortLog, 6);
	AddItem(EForestItem::HollowLog, 2);
	AddItem(EForestItem::Plank, 6);
	AddItem(EForestItem::Beam, 10);
	AddItem(EForestItem::Firewood, 10);
	AddItem(EForestItem::Stone, 10);
	AddItem(EForestItem::StoneBlock, 10);
	AddItem(EForestItem::WindowBuck, 2);
	AddItem(EForestItem::DoorBuck, 1);
	ShowMessage(TEXT("Test malzemesi verildi (Kütük, Taş, Kasalar)"));
}

void AForestCharacter::DebugHarvestNearest()
{
	if (!RequireAdmin())
	{
		return;
	}
	const FVector Eye = Camera->GetComponentLocation();
	const FVector Forward = Camera->GetForwardVector();
	AHarvestableActor* Best = nullptr;
	float BestDist = 4000.f;
	for (TActorIterator<AHarvestableActor> It(GetWorld()); It; ++It)
	{
		const FVector ToTarget = It->GetActorLocation() - Eye;
		const float Dist = ToTarget.Size2D();
		if (It->IsWood() && It->IsAlive() && Dist < BestDist && FVector::DotProduct(ToTarget.GetSafeNormal2D(), Forward.GetSafeNormal2D()) > 0.3f)
		{
			BestDist = Dist;
			Best = *It;
		}
	}
	if (Best)
	{
		FHitResult Hit;
		Hit.ImpactPoint = Best->GetActorLocation() + FVector(0, 0, 50.f);
		Hit.ImpactNormal = -Forward;
		Best->ApplyHit(100000.f, Hit, Forward);
	}
}

void AForestCharacter::DebugBuildRoom()
{
	if (!RequireAdmin())
	{
		return;
	}
	// Same path as the player: four walls chained corner to corner, closing onto the first.
	FVector Origin = GetActorLocation() + GetActorForwardVector() * 500.f;
	FHitResult Ground;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DebugRoom), false, this);
	if (GetWorld()->LineTraceSingleByChannel(Ground, Origin + FVector(0, 0, 500.f), Origin - FVector(0, 0, 1000.f), ECC_Visibility, Params))
	{
		Origin = Ground.ImpactPoint;
	}
	const FVector Forward = FVector(GetActorForwardVector().X, GetActorForwardVector().Y, 0.f).GetSafeNormal();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);
	const TArray<FVector> Corners = { Origin, Origin + Right * 500.f, Origin + Right * 500.f + Forward * 400.f, Origin + Forward * 400.f };

	// Spawn dry stone foundation under corners
	for (const FVector& Corner : Corners)
	{
		FActorSpawnParameters StoneSpawnParams;
		StoneSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AStoneMasonryActor* Stone = GetWorld()->SpawnActor<AStoneMasonryActor>(AStoneMasonryActor::StaticClass(), FTransform(Corner - FVector(0, 0, 20.f)), StoneSpawnParams))
		{
			Stone->InitializeStone(EMasonryType::DryStone);
		}
	}

	FVector Start = Corners[0];
	float Phase = 0.f;
	for (int32 Index = 0; Index < Corners.Num(); ++Index)
	{
		const bool bClosing = Index == Corners.Num() - 1;
		TArray<FTransform> Slots;
		FVector RealEnd;
		if (!ALogWallBlueprint::ComputeSlots(Start, Corners[(Index + 1) % Corners.Num()], Phase, bClosing, Slots, RealEnd))
		{
			return;
		}
		ALogWallBlueprint* Wall = GetWorld()->SpawnActor<ALogWallBlueprint>();
		Wall->Init(Slots, Start, RealEnd, Phase);
		while (Wall->AddLog())
		{
		}
		Start = RealEnd;
		Phase = 1.f - Phase;
	}
	ShowMessage(TEXT("Test odası kuruldu (Taş Temel + 4 Kütük Duvar)"));
}

void AForestCharacter::CancelBuild()
{
	if (StonePlacement && StonePlacement->IsPlacing())
	{
		if (StonePlacement->CancelPlacement())
		{
			RefreshToolVisibility();
			GetCharacterMovement()->MaxWalkSpeed = CarrySpeed();
			ShowMessage(TEXT("Taş yerleştirme iptal edildi; taş eski konumuna döndü."));
		}
		else
		{
			ShowMessage(TEXT("Taş yerleştirme iptal edilemedi."));
		}
		return;
	}

	if (bBuildMode && bWallStartSet)
	{
		bWallStartSet = false;
		WallPreview->ClearInstances();
		ShowMessage(TEXT("Duvar plan\u0131 b\u0131rak\u0131ld\u0131"));
	}
}

bool AForestCharacter::AimWallPoint(FVector& OutPoint) const
{
	FHitResult Hit;
	if (!TraceFromView(BuildDistance, Hit, /*bIgnorePlans*/ true) || Hit.ImpactNormal.Z < 0.6f)
	{
		return false;
	}
	OutPoint = Hit.ImpactPoint;
	return true;
}

bool AForestCharacter::FindWallEndpoint(const FVector& Near, FVector& OutPoint, float& OutPhase) const
{
	const float SnapDistance = 100.f;
	float BestDistSq = FMath::Square(SnapDistance);
	bool bFound = false;
	for (TActorIterator<ALogWallBlueprint> It(GetWorld()); It; ++It)
	{
		for (const FVector& Point : { It->GetStart(), It->GetEnd() })
		{
			const float DistSq = FVector::DistSquared2D(Point, Near);
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				OutPoint = Point;
				OutPhase = It->GetPhase();
				bFound = true;
			}
		}
	}
	return bFound;
}

void AForestCharacter::UpdateWallPreview()
{
	WallPreview->ClearInstances();
	BuildPreview->SetVisibility(false);
	bPlacementValid = false;

	FVector Aim;
	if (!AimWallPoint(Aim))
	{
		return;
	}
	FVector Snapped;
	float SnapPhase = 0.f;
	const bool bSnap = FindWallEndpoint(Aim, Snapped, SnapPhase);

	// Before the first click show where the wall would start: one log along the view.
	const FVector Start = bWallStartSet ? WallStart : (bSnap ? Snapped : Aim);
	const FVector End = bWallStartSet ? (bSnap ? Snapped : Aim) : Start + GetActorRightVector() * 250.f;
	const float Phase = bWallStartSet ? WallPhase : (bSnap ? 1.f - SnapPhase : 0.f);

	TArray<FTransform> Slots;
	FVector RealEnd;
	bPlacementValid = ALogWallBlueprint::ComputeSlots(Start, End, Phase, bWallStartSet && bSnap, Slots, RealEnd);
	const int32 Shown = bWallStartSet ? Slots.Num() : FMath::Min(1, Slots.Num());
	for (int32 Index = 0; Index < Shown; ++Index)
	{
		WallPreview->AddInstance(Slots[Index], /*bWorldSpace*/ true);
	}
	SetGhostValid(bPlacementValid);
}

void AForestCharacter::WallClick()
{
	FVector Aim;
	if (!AimWallPoint(Aim))
	{
		ShowMessage(TEXT("Duvar d\u00FCz bir zemine kurulmal\u0131"));
		return;
	}
	FVector Snapped;
	float SnapPhase = 0.f;
	const bool bSnap = FindWallEndpoint(Aim, Snapped, SnapPhase);

	if (!bWallStartSet)
	{
		// Starting from another wall's end shifts the rows half a log so the corner interlocks.
		WallStart = bSnap ? Snapped : Aim;
		WallPhase = bSnap ? 1.f - SnapPhase : 0.f;
		bWallStartSet = true;
		ShowMessage(TEXT("Duvar\u0131n biti\u015F noktas\u0131na t\u0131kla (sa\u011F t\u0131k iptal)"));
		return;
	}

	TArray<FTransform> Slots;
	FVector RealEnd;
	if (!ALogWallBlueprint::ComputeSlots(WallStart, bSnap ? Snapped : Aim, WallPhase, bSnap, Slots, RealEnd))
	{
		ShowMessage(TEXT("Duvar \u00E7ok k\u0131sa"));
		return;
	}

	ALogWallBlueprint* Wall = GetWorld()->SpawnActor<ALogWallBlueprint>();
	Wall->Init(Slots, WallStart, RealEnd, WallPhase);

	// Keep going from this corner so a whole room can be laid out in a few clicks.
	WallStart = RealEnd;
	WallPhase = 1.f - WallPhase;
	ShowMessage(FString::Printf(TEXT("Duvar plan\u0131: %d k\u00FCt\u00FCk. Sonraki k\u00F6\u015Feye t\u0131kla, sa\u011F t\u0131k bitir. Plana bak\u0131p [E] ile k\u00FCt\u00FCk ekle."), Slots.Num()));
}

void AForestCharacter::RotateBuild()
{
	if (StonePlacement && StonePlacement->IsPlacing())
	{
		StonePlacement->RotateStone(1.0f);
		return;
	}

	if (bBuildMode)
	{
		BuildYawOffset = FMath::Fmod(BuildYawOffset + 90.f, 360.f);
	}
	else if (CarriedItem == EForestItem::Log)
	{
		// Lay -> stand -> lean -> lay.
		if (bPlaceSlope)
		{
			bPlaceSlope = false;
			bPlaceStanding = false;
		}
		else if (bPlaceStanding)
		{
			bPlaceStanding = false;
			bPlaceSlope = true;
		}
		else
		{
			bPlaceStanding = true;
		}
		ShowMessage(bPlaceSlope ? FString::Printf(TEXT("Koyma: eğik %.0f°  (tekerlek: açı)"), SlopeAngle)
			: bPlaceStanding ? FString(TEXT("Koyma: dik (direk)")) : FString(TEXT("Koyma: yatır")));
	}
	else if (FocusedCarvable.IsValid() && RawLogShapes::FromLog(FocusedCarvable.Get()).IsValid())
	{
		bChannelMode = !bChannelMode;
		ShapeHits = 0;
		ShowMessage(bChannelMode ? TEXT("Oluk açma: kütüğün üstüne vur  —  [R] normal") : TEXT("Normal: ortadan vur = enine kes, uca vur = boyuna yar, yana vur = düzle  —  [R] oluk"));
	}
	else if (FocusedCarvable.IsValid())
	{
		CarveTool = static_cast<ECarveTool>((static_cast<uint8>(CarveTool) + 1) % static_cast<uint8>(ECarveTool::Count));
		CarveAngle = CarveTool == ECarveTool::Notch ? 90.f : 0.f;
		CarveHits = 0;
		ShowMessage(FString::Printf(TEXT("Oyma aracı: %s"), GetCarveToolName(CarveTool)));
	}
}

void AForestCharacter::CycleBuildItem(const FInputActionValue& Value)
{
	const float Axis = Value.Get<float>();
	if (StonePlacement && StonePlacement->IsPlacing())
	{
		if (!FMath::IsNearlyZero(Axis))
		{
			StonePlacement->CycleFace();
		}
		return;
	}
	if (bStickMode && !FMath::IsNearlyZero(Axis))
	{
		StickBuildIndex = static_cast<uint8>((StickBuildIndex + (Axis > 0.f ? 1 : CraftEntryCount - 1)) % CraftEntryCount);
		ShowMessage(GetStickModeLabel());
		return;
	}
	if (bBuildMode && !FMath::IsNearlyZero(Axis))
	{
		SelectNextAvailable(Axis > 0.f ? 1 : -1);
	}
	else if (!bBuildMode && CarriedItem == EForestItem::Log && bPlaceSlope && !FMath::IsNearlyZero(Axis))
	{
		SlopeAngle = FMath::Clamp(SlopeAngle + (Axis > 0.f ? 15.f : -15.f), 15.f, 75.f);
		ShowMessage(FString::Printf(TEXT("Eğim: %.0f°"), SlopeAngle));
	}
	else if (!bBuildMode && FocusedCarvable.IsValid() && IsProfileTool(CarveTool) && !FMath::IsNearlyZero(Axis))
	{
		// Wheel: one of three depths, a sixth, a third or half the log's thickness.
		CarveDepthIndex = (CarveDepthIndex + (Axis > 0.f ? 1 : 2)) % 3;
		CarveHits = 0;
	}
	else if (!bBuildMode && FocusedCarvable.IsValid() && CarveTool != ECarveTool::CrossCut && !FMath::IsNearlyZero(Axis))
	{
		// Notch: square corner (90) or lengthwise groove (0). Split: down (0) or across (90).
		CarveAngle = CarveAngle > 45.f ? 0.f : 90.f;
		CarveHits = 0;
	}
}

void AForestCharacter::SelectNextAvailable(int32 Direction)
{
	// Every item the player carries, then the wall planner (None) which is always available.
	TArray<EForestItem> Options;
	for (const EForestItem Item : ForestItems::GetAllItems())
	{
		if (GetItemCount(Item) > 0)
		{
			Options.Add(Item);
		}
	}
	Options.Add(EForestItem::None);
	const int32 Current = FMath::Max(0, Options.IndexOfByKey(SelectedBuildItem));
	SelectBuildItem(Options[(Current + Direction + Options.Num()) % Options.Num()]);
}

void AForestCharacter::SelectBuildItem(EForestItem Item)
{
	if (Item == SelectedBuildItem)
	{
		return;
	}
	SelectedBuildItem = Item;
	bWallStartSet = false;
	WallPreview->ClearInstances();
	BuildPreview->SetVisibility(false);
	BuildPreview->SetStaticMesh(ForestItems::GetMesh(Item, 0));
	if (GhostMaterial)
	{
		for (int32 Slot = 0; Slot < BuildPreview->GetNumMaterials(); ++Slot)
		{
			BuildPreview->SetMaterial(Slot, GhostMaterial);
		}
	}
}

void AForestCharacter::SetGhostValid(bool bValid)
{
	if (GhostMaterial && bValid != bGhostValid)
	{
		GhostMaterial->SetVectorParameterValue(TEXT("GhostColor"), bValid ? FLinearColor(0.2f, 1.f, 0.3f) : FLinearColor(1.f, 0.15f, 0.1f));
	}
	bGhostValid = bValid;
}

void AForestCharacter::UpdateBuildPreview()
{
	if (IsWallTool())
	{
		UpdateWallPreview();
		return;
	}
	FTransform Transform;
	const UInstancedStaticMeshComponent* SnappedTo = nullptr;
	const bool bHasSpot = GetItemCount(SelectedBuildItem) > 0 && ComputeBuildTransform(Transform, &SnappedTo);
	bPlacementValid = bHasSpot && IsPlacementFree(Transform, SnappedTo);
	BuildPreview->SetVisibility(bHasSpot);
	if (bHasSpot)
	{
		BuildPreview->SetWorldTransform(Transform);
		SetGhostValid(bPlacementValid);
	}
}

bool AForestCharacter::ComputeBuildTransform(FTransform& OutTransform, const UInstancedStaticMeshComponent** OutSnappedTo) const
{
	const UStaticMesh* PieceMesh = BuildPreview->GetStaticMesh();
	if (!PieceMesh || !Structures)
	{
		return false;
	}

	FHitResult Hit;
	if (!TraceFromView(BuildDistance, Hit, /*bIgnorePlans*/ true))
	{
		return false;
	}

	const FVector PieceScale = ForestItems::GetItemScale(SelectedBuildItem);
	const FBox LocalBox = ScaledBox(PieceMesh, SelectedBuildItem);
	const FVector Size = LocalBox.GetSize();

	// Looking at a placed piece of the same kind (free placed or part of a planned wall): stack onto it.
	const UInstancedStaticMeshComponent* Pieces = Cast<UInstancedStaticMeshComponent>(Hit.GetComponent());
	const AActor* PiecesOwner = Pieces ? Pieces->GetOwner() : nullptr;
	if (Pieces && Hit.Item != INDEX_NONE && Pieces->GetStaticMesh() == PieceMesh
		&& (PiecesOwner == Structures || (PiecesOwner && PiecesOwner->IsA<ALogWallBlueprint>())))
	{
		FTransform Instance;
		Pieces->GetInstanceTransform(Hit.Item, Instance, /*bWorldSpace*/ true);

		// Work from the centre and a level frame along the length: wall logs may be
		// stretched or turned over, which must not leak into the new piece.
		const FVector InstanceCenter = PieceCenter(Instance, LocalBox);
		FVector Along = Instance.GetUnitAxis(EAxis::X);
		Along.Z = 0.f;
		const FQuat Frame = FRotator(0.f, Along.GetSafeNormal().Rotation().Yaw, 0.f).Quaternion();
		const FVector LocalHit = Frame.UnrotateVector(Hit.ImpactPoint - InstanceCenter);
		const FVector LocalNormal = Frame.UnrotateVector(Hit.ImpactNormal);

		// Logs nest into each other like the planned walls; square pieces sit flush.
		const bool bLog = ForestItems::IsLogItem(SelectedBuildItem);
		const float StackStep = bLog ? Size.Z * 0.74f : Size.Z;
		const float SideStep = bLog ? Size.Y * 0.95f : Size.Y;
		const float EndStep = bLog ? Size.X * 0.96f : Size.X;
		const bool bCrossing = FMath::Abs(FMath::Fmod(BuildYawOffset, 180.f)) > 1.f;

		FQuat Rotation = Frame;
		FVector Offset = FVector::ZeroVector;
		if (bCrossing && LocalNormal.Z > 0.3f)
		{
			// Turned 90 degrees on top: lay across the nearest end, half a row up, ends sticking out,
			// the way log cabin corners lock together.
			const float Overhang = Size.Y * 0.6f;
			const FVector LocalViewer = Frame.UnrotateVector(Camera->GetComponentLocation() - InstanceCenter);
			Rotation = Frame * FRotator(0.f, 90.f, 0.f).Quaternion();
			Offset.X = (LocalHit.X >= 0.f ? 1.f : -1.f) * (Size.X * 0.5f - Overhang);
			Offset.Y = (LocalViewer.Y >= 0.f ? 1.f : -1.f) * (Size.X * 0.5f - Overhang);
			Offset.Z = StackStep * 0.5f;
		}
		else if (LocalNormal.Z > 0.5f || (bLog && LocalHit.Z > Size.Z * 0.1f))
		{
			// Round logs: aiming anywhere on the upper half means "on top".
			Offset.Z = StackStep;
		}
		else if (LocalNormal.Z < -0.5f)
		{
			Offset.Z = -StackStep;
		}
		else if (FMath::Abs(LocalNormal.Y) >= FMath::Abs(LocalNormal.X))
		{
			Offset.Y = FMath::Sign(LocalNormal.Y) * SideStep;
		}
		else
		{
			Offset.X = FMath::Sign(LocalNormal.X) * EndStep;
		}

		// Aiming at a lower log of a stack: keep stepping the same way until a free slot,
		// so clicking anywhere on a pile adds to its top.
		FVector Center = InstanceCenter + Frame.RotateVector(Offset);
		for (int32 Step = 0; Step < 12 && IsSlotTaken(Pieces, Center, LocalBox); ++Step)
		{
			Center += Frame.RotateVector(bCrossing ? FVector(0.f, 0.f, StackStep) : Offset);
		}
		OutTransform = FTransform(Rotation, Center - Rotation.RotateVector(LocalBox.GetCenter()), PieceScale);
		if (OutSnappedTo)
		{
			*OutSnappedTo = Pieces;
		}
		return true;
	}

	// Anything else needs a surface to stand on (ground, top of a wall).
	if (Hit.ImpactNormal.Z < 0.6f)
	{
		return false;
	}

	const FVector BottomCenter(LocalBox.GetCenter().X, LocalBox.GetCenter().Y, LocalBox.Min.Z);

	// Near a placed piece of the same kind: take its rotation and line up on its grid,
	// so logs, beams and blocks form clean rows instead of loose scatter.
	if (const UInstancedStaticMeshComponent* Same = Structures->FindPieces(SelectedBuildItem))
	{
		const TArray<int32> Nearby = Same->GetInstancesOverlappingSphere(Hit.ImpactPoint, Size.GetMax() * 1.25f, /*bSphereInWorldSpace*/ true);
		float BestDistSq = MAX_flt;
		FTransform Neighbour;
		for (const int32 Index : Nearby)
		{
			FTransform Instance;
			Same->GetInstanceTransform(Index, Instance, /*bWorldSpace*/ true);
			const float DistSq = FVector::DistSquared2D(Instance.GetLocation(), Hit.ImpactPoint);
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Neighbour = Instance;
			}
		}
		if (BestDistSq < MAX_flt)
		{
			// Same spacing as stacking: logs nest slightly into each other so a row reads as one floor.
			const bool bLog = ForestItems::IsLogItem(SelectedBuildItem);
			const float SideStep = bLog ? Size.Y * 0.95f : Size.Y;
			const float EndStep = bLog ? Size.X * 0.96f : Size.X;
			const FQuat Frame = Neighbour.GetRotation();
			const FVector NeighbourCenter = PieceCenter(Neighbour, LocalBox);
			const FVector LocalAim = Frame.UnrotateVector(Hit.ImpactPoint - NeighbourCenter);
			const int32 BaseX = FMath::RoundToInt(LocalAim.X / EndStep);
			const int32 BaseY = FMath::RoundToInt(LocalAim.Y / SideStep);

			// Take the free grid cell closest to the aim, so aiming at a piece still snaps beside it.
			float BestScore = MAX_flt;
			FVector BestCenter = NeighbourCenter;
			for (int32 DX = -1; DX <= 1; ++DX)
			{
				for (int32 DY = -1; DY <= 1; ++DY)
				{
					const FVector Candidate = NeighbourCenter + Frame.RotateVector(FVector((BaseX + DX) * EndStep, (BaseY + DY) * SideStep, 0.f));
					const float Score = FVector::DistSquared2D(Candidate, Hit.ImpactPoint);
					if (Score < BestScore && !IsSlotTaken(Same, Candidate, LocalBox))
					{
						BestScore = Score;
						BestCenter = Candidate;
					}
				}
			}
			if (BestScore < MAX_flt)
			{
				OutTransform = FTransform(Frame, BestCenter - Frame.RotateVector(LocalBox.GetCenter()), PieceScale);
				return true;
			}
		}
	}

	// Free placement: grid snap, sit the bottom of the piece on the surface. Long pieces (logs, beams)
	// lie across the view so they form a wall in front of the player instead of pointing at them.
	const bool bLongPiece = Size.X > Size.Y * 2.f;
	const float Yaw = FMath::GridSnap(GetControlRotation().Yaw, 90.f) + (bLongPiece ? 90.f : 0.f) + BuildYawOffset;
	const FQuat Rotation = FRotator(0.f, Yaw, 0.f).Quaternion();
	FVector Location = Hit.ImpactPoint;
	Location.X = FMath::GridSnap(Location.X, 25.f);
	Location.Y = FMath::GridSnap(Location.Y, 25.f);
	Location -= Rotation.RotateVector(BottomCenter);
	OutTransform = FTransform(Rotation, Location, PieceScale);
	return true;
}

bool AForestCharacter::IsPlacementFree(const FTransform& Transform, const UInstancedStaticMeshComponent* SnappedTo) const
{
	const UStaticMesh* PieceMesh = BuildPreview->GetStaticMesh();
	if (!PieceMesh)
	{
		return false;
	}
	const FBox LocalBox = ScaledBox(PieceMesh, SelectedBuildItem);
	const FVector Center = PieceCenter(Transform, LocalBox);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(BuildOverlap), false);

	if (SnappedTo)
	{
		// Stacked pieces are meant to nest into their neighbours, so only refuse a slot that is
		// already taken or one that would trap the player.
		if (IsSlotTaken(SnappedTo, Center, LocalBox))
		{
			return false;
		}
		// Only pawns count here: the neighbouring logs themselves are supposed to overlap.
		const FVector Core = (LocalBox.GetExtent() * 0.8f).ComponentMax(FVector(1.f));
		return !GetWorld()->OverlapAnyTestByObjectType(Center, Transform.GetRotation(), FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeBox(Core), Params);
	}

	// Shrink a little so pieces may touch neighbours and the ground without counting as overlap.
	const FVector Extent = (LocalBox.GetExtent() - FVector(3.f)).ComponentMax(FVector(1.f));
	const FCollisionObjectQueryParams Objects(FCollisionObjectQueryParams::AllObjects);
	return !GetWorld()->OverlapAnyTestByObjectType(Center, Transform.GetRotation(), Objects, FCollisionShape::MakeBox(Extent), Params);
}

void AForestCharacter::PlaceSelected()
{
	if (GetItemCount(SelectedBuildItem) <= 0)
	{
		ShowMessage(TEXT("Yerleştirecek eşyan yok"));
		return;
	}
	FTransform Transform;
	const UInstancedStaticMeshComponent* SnappedTo = nullptr;
	if (!ComputeBuildTransform(Transform, &SnappedTo) || !IsPlacementFree(Transform, SnappedTo))
	{
		ShowMessage(TEXT("Buraya yerleştirilemez"));
		return;
	}
	if (SelectedBuildItem == EForestItem::StoneBlock)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStoneMasonryActor* StoneActor = GetWorld()->SpawnActor<AStoneMasonryActor>(AStoneMasonryActor::StaticClass(), Transform, SpawnParams);
		if (StoneActor)
		{
			StoneActor->InitializeStone(EMasonryType::DryStone);
			const bool bValid = StoneActor->ValidateDryStoneRules();
			ShowMessage(bValid ? TEXT("Kuru taş temel bloğu yerleştirildi (8 Kurala Uygun: Throughstones + Batter)") : TEXT("Taş temel bloğu yerleştirildi"));
		}
	}
	else if (SelectedBuildItem == EForestItem::WindowBuck || SelectedBuildItem == EForestItem::DoorBuck)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AOpeningBuckActor* BuckActor = GetWorld()->SpawnActor<AOpeningBuckActor>(AOpeningBuckActor::StaticClass(), Transform, SpawnParams);
		if (BuckActor)
		{
			const bool bIsWindow = (SelectedBuildItem == EForestItem::WindowBuck);
			BuckActor->InitializeBuck(bIsWindow ? EOpeningType::Window : EOpeningType::Door, 90.f, bIsWindow ? 120.f : 210.f, true);
			ShowMessage(FString::Printf(TEXT("%s yerleştirildi — 6%% Settling payı: %.1f cm"),
				*ForestItems::GetDisplayName(SelectedBuildItem), BuckActor->CalculateSettlingAllowance()));
		}
	}
	else
	{
		Structures->AddPiece(SelectedBuildItem, Transform);
	}
	RemoveItem(SelectedBuildItem, 1);
	if (GetItemCount(SelectedBuildItem) <= 0)
	{
		SelectNextAvailable(1);
	}
}


void AForestCharacter::ShowMessage(const FString& Text)
{
	Message = Text;
	MessageTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}

void AForestCharacter::ApplyShapeHit(ACarvableLog* Log, const FHitResult& Hit)
{
	if (Log->IsAnchored() || Log->IsCarried())
	{
		ShowMessage(TEXT("Yapıdaki parça işlenmez  —  önce [E] ile al, yere koy"));
		return;
	}
	const RawLogShapes::FShape Shape = RawLogShapes::FromLog(Log);
	const RawLogShapes::EAction Action = RawLogShapes::ClassifyHit(Log, Hit, bChannelMode);
	FString Why;
	if (!RawLogShapes::CanApply(Shape, Action, Why))
	{
		ShapeHits = 0;
		ShowMessage(Why);
		return;
	}
	if (ShapeTarget.Get() != Log || ShapeAction != static_cast<uint8>(Action))
	{
		ShapeTarget = Log;
		ShapeAction = static_cast<uint8>(Action);
		ShapeHits = 0;
	}
	AHarvestableActor::PlayFellingSound(GetWorld(), TEXT("SW_AxeChop"), Hit.ImpactPoint);
	const int32 Needed = RawLogShapes::HitsFor(Action);
	if (++ShapeHits < Needed)
	{
		ShowMessage(FString::Printf(TEXT("%s  %d/%d  (%s)"), *RawLogShapes::ActionName(Action), ShapeHits, Needed, *RawLogShapes::Describe(Shape)));
		return;
	}
	ShapeHits = 0;
	ShapeTarget = nullptr;
	FString Made;
	if (RawLogShapes::Apply(Log, Action, Made))
	{
		FocusedCarvable = nullptr;
		ShowMessage(Made);
	}
	else
	{
		ShowMessage(Made);
	}
}

void AForestCharacter::TestPressInteract()
{
	UpdateFocus();
	Interact();
}

void AForestCharacter::TestPressPrimary()
{
	UpdateFocus();
	PrimaryAction();
}

void AForestCharacter::TestShapeHit(ACarvableLog* Log, FVector LocalPoint, FVector LocalNormal, bool bChannel)
{
	if (!Log)
	{
		return;
	}
	const FTransform Frame = Log->GetActorTransform();
	FHitResult Hit(Log, nullptr, Frame.TransformPosition(LocalPoint), Frame.TransformVectorNoScale(LocalNormal).GetSafeNormal());
	bChannelMode = bChannel;
	ApplyShapeHit(Log, Hit);
}

FString AForestCharacter::TestCarriedItem() const
{
	return CarriedItem == EForestItem::None ? FString(TEXT("None")) : ForestItems::GetDisplayName(CarriedItem);
}

FString AForestCharacter::TestLastMessage() const
{
	return Message;
}

float AForestCharacter::GetMessageAge() const
{
	return GetWorld() ? static_cast<float>(GetWorld()->GetTimeSeconds() - MessageTime) : 100.f;
}

void AForestCharacter::UpdateCarveAim(const FHitResult& Hit, ACarvableLog* Log)
{
	const FTransform Frame = Log->GetActorTransform();
	const FVector Local = Frame.InverseTransformPosition(Hit.ImpactPoint);
	// Joints are as long as the log is thick (the crossing log is the same size) and opened on the top or the
	// bottom face only: the log is rolled over to work the other side, the axe does not cut sideways into it.
	CarveWidth = Log->GetDiameter();
	CarveDepth = Log->GetDiameter() * GetCarveDepthFraction(CarveDepthIndex);
	// "Top" is the face toward the sky whichever way the log has rolled, "bottom" the face toward the ground.
	const FVector Up = Frame.InverseTransformVectorNoScale(FVector::UpVector);
	const float TopSide = FMath::RadiansToDegrees(FMath::Atan2(Up.Y, Up.Z));
	CarveSide = Hit.ImpactNormal.Z >= 0.f ? TopSide : TopSide + 180.f;
	bCarveSpotValid = CarveTool == ECarveTool::Split || Log->FindSnapPoint(CarveTool, Local.X, CarveWidth, CarveX);
}

float AForestCharacter::GetCarveDepthFraction(int32 Index)
{
	static const float Fractions[] = { 1.f / 6.f, 1.f / 3.f, 0.5f };
	return Fractions[FMath::Clamp(Index, 0, 2)];
}

void AForestCharacter::UpdateCarvePreview()
{
	ACarvableLog* Log = FocusedCarvable.Get();
	if (!Log || bBuildMode || IsHandsBusy() || !CarveCylinder || !CarveCube || !bCarveSpotValid)
	{
		CarvePreview->SetVisibility(false);
		return;
	}

	// Basic shapes are 100 cm, centred.
	const float R = Log->GetRadius();
	UStaticMesh* Shape = CarveCylinder;
	FTransform Local;
	if (CarveTool == ECarveTool::Notch)
	{
		FVector Centre, Direction;
		Log->GetNotchFrame(CarveX, CarveSide, CarveAngle, R, R, Centre, Direction);
		Local = FTransform(FRotationMatrix::MakeFromZ(Direction).ToQuat(), Centre, FVector(R * 0.0204f, R * 0.0204f, R * 0.05f));
	}
	else if (IsProfileTool(CarveTool))
	{
		const bool bRound = CarveTool == ECarveTool::NotchRound;
		const float Depth = FMath::Min(CarveDepth, R);
		FVector Centre, Out, Across;
		Log->GetProfileFrame(CarveX, CarveSide, CarveWidth, Depth, bRound, Centre, Out, Across);
		if (bRound)
		{
			// The cutter cylinder, its axis across the log, a little longer than the log is wide.
			Local = FTransform(FRotationMatrix::MakeFromZ(Across).ToQuat(), Centre,
				FVector(CarveWidth * 0.01f, CarveWidth * 0.01f, R * 0.022f));
		}
		else
		{
			// Only the wood that comes out: Width along the log, Depth from the surface inward.
			Shape = CarveCube;
			const FVector Removed = FVector(CarveX, 0.f, 0.f) + Out * (R - Depth * 0.5f + 1.f);
			Local = FTransform(FRotationMatrix::MakeFromXZ(FVector::XAxisVector, Out).ToQuat(), Removed,
				FVector(CarveWidth * 0.01f, R * 0.022f, (Depth + 2.f) * 0.01f));
		}
	}
	else if (CarveTool == ECarveTool::CrossCut || IsEndTool(CarveTool))
	{
		Local = FTransform(FRotationMatrix::MakeFromZ(FVector::XAxisVector).ToQuat(), FVector(CarveX, 0.f, 0.f), FVector(R * 0.024f, R * 0.024f, 0.03f));
	}
	else
	{
		Shape = CarveCube;
		const float Angle = FMath::DegreesToRadians(CarveAngle);
		const FVector Normal(0.f, FMath::Cos(Angle), -FMath::Sin(Angle));
		Local = FTransform(FRotationMatrix::MakeFromZX(Normal, FVector::XAxisVector).ToQuat(), FVector(Log->GetCenterX(), 0.f, 0.f),
			FVector(Log->GetLength() * 0.0105f, R * 0.024f, 0.03f));
	}
	CarvePreview->SetStaticMesh(Shape);
	CarvePreview->SetWorldTransform(Local * Log->GetActorTransform());
	CarvePreview->SetVisibility(true);
}

void AForestCharacter::ApplyCarveHit(ACarvableLog* Log)
{
	if (!bCarveSpotValid)
	{
		ShowMessage(TEXT("Bu kütükte bu kesim için yer kalmadı"));
		return;
	}
	const float ShapeKey = IsProfileTool(CarveTool) ? CarveWidth * 1000.f + CarveDepth : CarveAngle;
	const FVector4 Key(CarveX, CarveSide, ShapeKey, static_cast<float>(static_cast<uint8>(CarveTool)));
	// Same cut if it is the same tool and shape at the same snap point and face. The face angle follows the log
	// as physics settles it, so it is compared loosely; an exact match would never let the second hit land.
	// A cross cut or split goes through the whole log: the face it was aimed from does not matter.
	const bool bFaceMatters = CarveTool != ECarveTool::CrossCut && CarveTool != ECarveTool::Split && !IsEndTool(CarveTool);
	const bool bSameCut = CarveTarget.Get() == Log
		&& Key.W == CarveKey.W
		&& FMath::IsNearlyEqual(Key.Z, CarveKey.Z, 0.5f)
		&& FMath::IsNearlyEqual(Key.X, CarveKey.X, 2.f)
		&& (!bFaceMatters || FMath::Abs(FMath::FindDeltaAngleDegrees(Key.Y, CarveKey.Y)) < 15.f);
	if (bSameCut)
	{
		++CarveHits;
	}
	else
	{
		CarveTarget = Log;
		CarveKey = Key;
		CarveHits = 1;
	}
	if (CarveHits < 2)
	{
		ShowMessage(TEXT("Kesim izi açıldı  —  aynı yere bir vuruş daha"));
		return;
	}
	CarveHits = 0;
	CarveTarget = nullptr;
	CarveKey = FVector4(0.f, 0.f, 0.f, -1.f);

	const float R = Log->GetRadius();
	switch (CarveTool)
	{
	case ECarveTool::Notch:
		// A square cup is half the crossing log deep; a lengthwise groove is b = 0.5 d wide.
		Log->Notch(CarveX, CarveSide, CarveAngle, R, CarveAngle < 1.f ? R * 0.134f : R);
		ShowMessage(CarveAngle < 1.f ? TEXT("Boyuna kanal açıldı") : TEXT("Çentik açıldı"));
		break;
	case ECarveTool::CrossCut:
		Log->CrossCut(CarveX);
		ShowMessage(TEXT("Kütük enine kesildi"));
		break;
	case ECarveTool::Split:
		Log->Split(CarveAngle);
		ShowMessage(TEXT("Kütük boyuna yarıldı"));
		break;
	case ECarveTool::Point:
	case ECarveTool::Chamfer:
	case ECarveTool::Trim:
		if (Log->ShapeEnd(CarveX > Log->GetCenterX(), CarveTool))
		{
			ShowMessage(CarveTool == ECarveTool::Point ? TEXT("Uç sivriltildi") : CarveTool == ECarveTool::Chamfer ? TEXT("Uç pahlandı") : TEXT("Uç düzeltildi"));
		}
		break;
	case ECarveTool::Groove:
		Log->CutGroove(CarveSide);
		ShowMessage(TEXT("Oluk açıldı"));
		break;
	case ECarveTool::Dovetail:
		Log->CutDovetail(CarveX, CarveSide, CarveWidth, FMath::Min(CarveDepth, R));
		ShowMessage(FString::Printf(TEXT("Kırlangıç kuyruğu açıldı: %.0f cm geniş, %.1f cm derin"), CarveWidth, FMath::Min(CarveDepth, R)));
		break;
	case ECarveTool::NotchRound:
	case ECarveTool::NotchFlat:
	{
		const float Depth = FMath::Min(CarveDepth, R);
		Log->CutProfile(CarveX, CarveSide, CarveWidth, Depth, CarveTool == ECarveTool::NotchRound);
		ShowMessage(FString::Printf(TEXT("%s açıldı: %.0f cm geniş, %.1f cm derin"), GetCarveToolName(CarveTool), CarveWidth, Depth));
		break;
	}
	default:
		break;
	}
}

const TCHAR* AForestCharacter::GetCarveToolName(ECarveTool Tool)
{
	switch (Tool)
	{
	case ECarveTool::Notch:      return TEXT("Çentik");
	case ECarveTool::CrossCut:   return TEXT("Enine kesim");
	case ECarveTool::Split:      return TEXT("Boyuna yarma");
	case ECarveTool::NotchRound: return TEXT("Oval çentik");
	case ECarveTool::NotchFlat:  return TEXT("Düz çentik");
	case ECarveTool::Point:      return TEXT("Sivri uç");
	case ECarveTool::Chamfer:    return TEXT("Pah");
	case ECarveTool::Trim:       return TEXT("Uç düzeltme");
	case ECarveTool::Dovetail:   return TEXT("Kırlangıç");
	case ECarveTool::Groove:     return TEXT("Oluk");
	default:                     return TEXT("");
	}
}

bool AForestCharacter::TestCarveHit(ACarvableLog* Log, ECarveTool Tool, const FVector& WorldPoint, const FVector& WorldNormal)
{
	CarveTool = Tool;
	FHitResult Hit;
	Hit.ImpactPoint = WorldPoint;
	Hit.ImpactNormal = WorldNormal;
	UpdateCarveAim(Hit, Log);
	const int32 TrisBefore = Log->GetTriangleCount();
	ApplyCarveHit(Log);
	return Log->GetTriangleCount() != TrisBefore;
}

void AForestCharacter::DebugDamageStructure()
{
	if (!RequireAdmin())
	{
		return;
	}
	if (ACarvableLog* Piece = FocusedCarvable.Get())
	{
		Piece->AddDamage(0.3f);
		ShowMessage(FString::Printf(TEXT("Hasar: %%%.0f"), Piece->GetDamage() * 100.f));
	}
}

void AForestCharacter::DebugMechanicsDemo()
{
	if (RequireAdmin())
	{
		AMechanicsDemo::Start(GetWorld(), /*bAuto*/ false);
	}
}

void AForestCharacter::MechanicsDemo()
{
	AMechanicsDemo::Start(GetWorld(), /*bAuto*/ false);
}

void AForestCharacter::DebugSpawnCarvableLogs()
{
	if (!RequireAdmin())
	{
		return;
	}
	const FVector Forward = FVector(GetActorForwardVector().X, GetActorForwardVector().Y, 0.f).GetSafeNormal();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);
	const TCHAR* Paths[] = {
		TEXT("/Game/Greybox/Meshes/SM_GB_RawLog_500.SM_GB_RawLog_500"),
		TEXT("/Game/Greybox/Meshes/SM_GB_RawLog_375.SM_GB_RawLog_375"),
		TEXT("/Game/Greybox/Meshes/SM_GB_RawLog_250.SM_GB_RawLog_250"),
	};
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const FVector Location = GetActorLocation() + Forward * (220.f + Index * 90.f) + FVector(0.f, 0.f, 30.f);
		ACarvableLog::SpawnFromStaticMesh(GetWorld(), LoadObject<UStaticMesh>(nullptr, Paths[Index]), FTransform(Right.Rotation(), Location));
	}
	ShowMessage(TEXT("Test: oyulabilir kütükler (A 500, B 375, C 250)  —  baltayla bak, [R] araç, [Tekerlek] açı, 2 vuruş"));
}

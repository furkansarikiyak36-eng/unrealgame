// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ForestTypes.generated.h"

class UStaticMesh;
class UMaterialInterface;

UENUM(BlueprintType)
enum class EForestItem : uint8
{
	None,
	Log,
	/** Stage 1 log: the whole trunk of a felled tree. */
	TrunkLog,
	/** Stage 2 log: half a trunk. */
	BigLog,
	/** A log split lengthwise, flat side up (bench seat, floor). */
	HalfLog,
	/** A short section of log with both ends sawn (bench leg, stump). */
	ShortLog,
	/** A log with its top carved out (trough, basin). */
	HollowLog,
	/** A half log split again lengthwise: a hewn plank. */
	Plank,
	Beam,
	Firewood,
	Stone,
	StoneBlock,
	WindowBuck UMETA(DisplayName = "Pencere Kasası (Window Buck)"),
	DoorBuck UMETA(DisplayName = "Kapı Kasası (Door Buck)"),
	/** Stackable small timber resource; distinct from physical ACarvableLog pieces. */
	Stick UMETA(DisplayName = "Çubuk"),
	/** Farming and animals: seeds to sow, grain and meat to eat, hide and wool to wear and twist into rope. */
	Seed UMETA(DisplayName = "Tohum"),
	Grain UMETA(DisplayName = "Tahıl"),
	Meat UMETA(DisplayName = "Et"),
	Hide UMETA(DisplayName = "Deri"),
	Wool UMETA(DisplayName = "Yün"),
	Egg UMETA(DisplayName = "Yumurta"),
	Milk UMETA(DisplayName = "Süt"),
	Rope UMETA(DisplayName = "İp"),
	/** Raw materials gathered by hand at ARawMaterialNode sources: they bind and seal pieces, they are not pieces. */
	PlantFiber UMETA(DisplayName = "Bitki Lifi"),
	Vine UMETA(DisplayName = "Asma"),
	Bark UMETA(DisplayName = "Ağaç Kabuğu"),
	Resin UMETA(DisplayName = "Reçine"),
	Clay UMETA(DisplayName = "Kil"),
	Flint UMETA(DisplayName = "Çakmak Taşı"),
	Bone UMETA(DisplayName = "Kemik")
};

UENUM(BlueprintType)
enum class EHarvestType : uint8
{
	Tree,
	Rock,
	/** A felled tree lying on the ground, cut into logs piece by piece. */
	Trunk
};

UENUM(BlueprintType)
enum class EForestTool : uint8
{
	Axe,
	Pickaxe,
	Trowel UMETA(DisplayName = "Mala (Chinking Trowel)"),
	Shovel UMETA(DisplayName = "Kürek")
};

namespace ForestItems
{
	/** Items in the order they are shown in the HUD. */
	MYPROJECT2_API const TArray<EForestItem>& GetAllItems();

	MYPROJECT2_API FString GetDisplayName(EForestItem Item);

	/** Stable item ID shared by the character inventory and modular InventoryCore. */
	MYPROJECT2_API FName GetInventoryID(EForestItem Item);

	/** Any of the three log stages (trunk, big log, log): they nest when stacked. */
	MYPROJECT2_API bool IsLogItem(EForestItem Item);

	/** Items that never enter the backpack: they are carried or dragged in the world (the three log stages). */
	MYPROJECT2_API bool IsCarriedItem(EForestItem Item);

	/** Mesh used when the item lies in the world. Variant picks between pack variations. */
	MYPROJECT2_API UStaticMesh* GetMesh(EForestItem Item, int32 Variant = 0);

	/**
	 * Scale the item is shown at in the world (pickups, placed pieces, wall plans).
	 * Lets a piece read bigger without touching its mesh asset.
	 */
	MYPROJECT2_API FVector GetItemScale(EForestItem Item);

	/** Approximate stack weight used by storage and transfer policies (kg per item). */
	MYPROJECT2_API float GetItemWeight(EForestItem Item);

	/**
	 * Big logs left by a felled tree. Stage 1 is the trunk, cutting it gives two stage 2 logs,
	 * cutting those gives the carryable Log item (stage 3).
	 */
	MYPROJECT2_API UStaticMesh* GetLogStageMesh(int32 Stage);

	/** Scale for a big log stage: each stage is twice as long as the next so halves line up exactly. */
	MYPROJECT2_API FVector GetLogStageScale(int32 Stage);

	/** Small cosmetic pieces thrown off when a tree or rock is hit. */
	MYPROJECT2_API UStaticMesh* GetDebrisMesh(EHarvestType Type, int32 Variant);

	/** Decal left behind by an axe (wood) or pickaxe (rock) hit. */
	MYPROJECT2_API UMaterialInterface* GetHitMarkMaterial(EHarvestType Type);

	/** Mesh of the tool held in first person. */
	MYPROJECT2_API UStaticMesh* GetToolMesh(EForestTool Tool);

	/** Translucent material for the build placement preview, tinted through the GhostColor parameter. */
	MYPROJECT2_API UMaterialInterface* GetBuildGhostMaterial();
}

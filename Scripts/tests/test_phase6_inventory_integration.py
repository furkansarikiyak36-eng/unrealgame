#!/usr/bin/env python3
"""Static integration contracts for Phase 6 stack inventory, sticks and save/load."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
CHARACTER_H = (ROOT / "Source/MyProject2/ForestCharacter.h").read_text(encoding="utf-8")
CHARACTER_CPP = (ROOT / "Source/MyProject2/ForestCharacter.cpp").read_text(encoding="utf-8")
FOREST_TYPES_H = (ROOT / "Source/MyProject2/ForestTypes.h").read_text(encoding="utf-8")
FOREST_TYPES_CPP = (ROOT / "Source/MyProject2/ForestTypes.cpp").read_text(encoding="utf-8")
GREYBOX = (ROOT / "Scripts/build_greybox.py").read_text(encoding="utf-8")
INVENTORY_H = (ROOT / "Source/MyProject2/InventoryCore/InventoryComponent.h").read_text(encoding="utf-8")
INVENTORY_CPP = (ROOT / "Source/MyProject2/InventoryCore/InventoryComponent.cpp").read_text(encoding="utf-8")
SAVE_H = (ROOT / "Source/MyProject2/WorldCore/WorldPersistenceSubsystem.h").read_text(encoding="utf-8")
SAVE_CPP = (ROOT / "Source/MyProject2/WorldCore/WorldPersistenceSubsystem.cpp").read_text(encoding="utf-8")
RUNNER = (ROOT / "Scripts/run_python_tests.sh").read_text(encoding="utf-8")
STORAGE_H = (ROOT / "Source/MyProject2/InventoryCore/InventoryStorageActor.h").read_text(encoding="utf-8")
STORAGE_CPP = (ROOT / "Source/MyProject2/InventoryCore/InventoryStorageActor.cpp").read_text(encoding="utf-8")


def section(source: str, start: str, end: str) -> str:
    begin = source.index(start)
    finish = source.index(end, begin + len(start))
    return source[begin:finish]


class Phase6InventoryIntegrationTests(unittest.TestCase):
    def test_character_has_one_canonical_stack_inventory(self):
        self.assertNotIn("TMap<EForestItem, int32> Inventory;", CHARACTER_H)
        self.assertIn("TObjectPtr<UInventoryComponent> CoreInventory", CHARACTER_H)
        self.assertIn('CoreInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("CoreInventory"))', CHARACTER_CPP)
        self.assertIn("InitializeCoreInventoryDefinitions()", CHARACTER_CPP)
        self.assertIn("CoreInventory->GetItemCount(ItemID)", CHARACTER_CPP)
        self.assertIn("CoreInventory->ExchangeItems", CHARACTER_CPP)
        self.assertNotIn("Inventory.Find(Item)", CHARACTER_CPP)

    def test_physical_logs_stay_world_owned_and_only_raw_loose_sticks_stack(self):
        self.assertIn("ACarvableLog", CHARACTER_H)
        self.assertIn("ForestItems::IsCarriedItem(Item)", CHARACTER_CPP)
        self.assertIn("Stick->GetStructuralState_Implementation() != EStructuralNodeState::Loose", CHARACTER_CPP)
        self.assertIn("StickDefinition->Shape != EStickShape::Raw", CHARACTER_CPP)
        self.assertIn("Stick->Destroy()", CHARACTER_CPP)

    def test_stick_has_a_stable_item_id_weight_and_mesh_asset(self):
        self.assertIn('Stick UMETA(DisplayName = "Çubuk")', FOREST_TYPES_H)
        self.assertIn("FName ForestItems::GetInventoryID(EForestItem Item)", FOREST_TYPES_CPP)
        self.assertIn("ForestItems::GetItemWeight", CHARACTER_CPP)
        self.assertIn('case EForestItem::Stick:      return LoadGreybox(TEXT("SM_GB_Stick"));', FOREST_TYPES_CPP)
        self.assertIn("made['Stick'] = save_mesh('SM_GB_Stick'", GREYBOX)

    def test_physical_drop_and_pickup_preserve_stock_on_failure(self):
        drop = section(CHARACTER_CPP, "AActor* AForestCharacter::DropInventoryItem", "int32 AForestCharacter::GetItemCount")
        self.assertLess(drop.index("CoreInventory->CanDropItem"), drop.index("CoreInventory->DropItem"))
        self.assertIn("Item == EForestItem::Stone", drop)
        self.assertIn("Item == EForestItem::Stick", drop)
        self.assertIn("RollbackSpawnedActors()", drop)
        self.assertIn("APickupItem::Spawn", drop)
        pickup = section(CHARACTER_CPP, "void AForestCharacter::Interact()", "int32 AForestCharacter::GetItemCount")
        self.assertIn("if (AddedCount != PickupCount)", pickup)
        self.assertLess(pickup.index("if (AddedCount != PickupCount)"), pickup.index("Pickup->Destroy()"))
        self.assertIn("bool UInventoryComponent::CanDropItem", INVENTORY_CPP)
        self.assertIn("Definition->bCanBeDropped", INVENTORY_CPP)

    def test_transfer_and_crafting_publish_only_after_transaction_commit(self):
        transfer = section(INVENTORY_CPP, "bool UInventoryComponent::TransferItem", "bool UInventoryComponent::ExchangeItems")
        self.assertIn("OriginalSourceSlots", transfer)
        self.assertIn("OriginalDestinationSlots", transfer)
        self.assertIn("Destination->Slots = OriginalDestinationSlots", transfer)
        self.assertIn("bSuppressInventoryEvents = true", transfer)
        exchange = section(INVENTORY_CPP, "bool UInventoryComponent::ExchangeItems", "int32 UInventoryComponent::GetItemCount")
        self.assertIn("Slots = OriginalSlots", exchange)
        self.assertIn("bSuppressInventoryEvents = true", exchange)
        self.assertIn("CoreInventory->ExchangeItems", CHARACTER_CPP)

    def test_player_inventory_is_versioned_and_preflighted_before_domain_import(self):
        self.assertIn("bool bHasPlayerInventory = false", SAVE_H)
        self.assertIn("TArray<FInventoryItem> PlayerInventoryItems", SAVE_H)
        self.assertIn("CurrentSaveSchemaVersion = 5", SAVE_H)
        self.assertIn("Manifest.SchemaVersion != 1 && Manifest.SchemaVersion != 2", SAVE_CPP)
        self.assertIn("SaveObject->PlayerInventoryItems.Add(Slot.Item)", SAVE_CPP)
        self.assertIn("PlayerInventory->CanRestoreFromSavedItems(LoadedSave->PlayerInventoryItems)", SAVE_CPP)
        self.assertLess(
            SAVE_CPP.index("PlayerInventory->CanRestoreFromSavedItems(LoadedSave->PlayerInventoryItems)"),
            SAVE_CPP.index("Excavation->ImportSnapshot(LoadedSave->Excavation, true)"),
        )
        self.assertIn("bool UInventoryComponent::CanRestoreFromSavedItems", INVENTORY_CPP)

    def test_storage_actor_uses_same_transfer_policy(self):
        self.assertIn("TObjectPtr<UInventoryComponent> Inventory", STORAGE_H)
        self.assertIn("UInventoryComponent::TransferItem(Source, Inventory, ItemID, Count)", STORAGE_CPP)
        self.assertIn("UInventoryComponent::TransferItem(Inventory, Destination, ItemID, Count)", STORAGE_CPP)
        self.assertIn("MaxSlots = 80", STORAGE_CPP)
        self.assertIn("MaxWeight = 2000.0f", STORAGE_CPP)

    def test_storage_snapshots_are_saved_preflighted_and_restored(self):
        self.assertIn("TArray<FInventoryStorageSnapshot> StorageInventories", SAVE_H)
        self.assertIn("SaveObject->bHasStorageInventories = true", SAVE_CPP)
        self.assertIn("StorageSnapshot.Items.Add(Slot.Item)", SAVE_CPP)
        self.assertIn("StorageActorsByKey", SAVE_CPP)
        self.assertIn("!(*FoundStorage)->GetInventory()->CanRestoreFromSavedItems(StorageSnapshot.Items)", SAVE_CPP)
        self.assertIn("Storage->GetInventory()->RestoreFromSavedItems(StorageSnapshot.Items)", SAVE_CPP)
        self.assertIn("CurrentSaveSchemaVersion = 5", SAVE_H)

    def test_phase6_contract_runs_in_the_python_ci_script(self):
        self.assertIn("test_inventory_transfer_contract.py", RUNNER)
        self.assertIn("test_phase6_inventory_integration.py", RUNNER)


if __name__ == "__main__":
    unittest.main(verbosity=2)

#!/usr/bin/env python3
"""Static contract checks for rollback-safe inventory transfers."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
HEADER = (ROOT / "Source/MyProject2/InventoryCore/InventoryComponent.h").read_text(encoding="utf-8")
CPP = (ROOT / "Source/MyProject2/InventoryCore/InventoryComponent.cpp").read_text(encoding="utf-8")


class InventoryTransferContractTests(unittest.TestCase):
    def test_public_transfer_api_is_declared(self):
        self.assertIn("static bool TransferItem(UInventoryComponent* Source", HEADER)

    def test_rejects_invalid_or_same_inventory_transfer(self):
        self.assertIn("!IsValid(Source) || !IsValid(Destination) || Source == Destination", CPP)
        self.assertIn("ItemID.IsNone() || Count <= 0 || !Source->HasItem(ItemID, Count)", CPP)

    def test_destination_capacity_is_checked_before_source_debit(self):
        self.assertLess(CPP.index("if (!Destination->CanAddItem(ItemID, Count))"), CPP.index("Source->RemoveItem(ItemID, Count)"))

    def test_source_payload_is_preserved(self):
        self.assertIn("Payload = Slot.Item", CPP)
        self.assertIn("Destination->AddItemData(Payload)", CPP)

    def test_failed_destination_add_restores_both_exact_slot_snapshots(self):
        transfer = CPP[CPP.index("bool UInventoryComponent::TransferItem"):CPP.index("bool UInventoryComponent::ExchangeItems")]
        self.assertIn("OriginalSourceSlots", transfer)
        self.assertIn("OriginalDestinationSlots", transfer)
        self.assertIn("Source->Slots = OriginalSourceSlots", transfer)
        self.assertIn("Destination->Slots = OriginalDestinationSlots", transfer)
        self.assertIn("Source->bSuppressInventoryEvents = true", transfer)
        self.assertIn("Destination->bSuppressInventoryEvents = true", transfer)
        self.assertNotIn("Source->AddItemData(Payload)", transfer)

    def test_item_exchange_is_transactional_for_processing_recipes(self):
        exchange = CPP[CPP.index("bool UInventoryComponent::ExchangeItems"):CPP.index("int32 UInventoryComponent::GetItemCount")]
        self.assertIn("OriginalSlots", exchange)
        self.assertIn("RemoveItem(InputID, InputCount)", exchange)
        self.assertIn("AddItem(OutputID, OutputCount)", exchange)
        self.assertIn("Slots = OriginalSlots", exchange)
        self.assertIn("bSuppressInventoryEvents = true", exchange)

    def test_does_not_create_resources_or_allow_drop_bypass(self):
        self.assertIn("Source->HasItem(ItemID, Count)", CPP)
        self.assertIn("Definition->bCanBeDropped", CPP)
        self.assertIn("CanDropItem(ItemID, Count)", CPP)


if __name__ == "__main__":
    unittest.main(verbosity=2)

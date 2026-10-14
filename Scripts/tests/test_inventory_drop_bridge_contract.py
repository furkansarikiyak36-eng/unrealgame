#!/usr/bin/env python3
"""Static checks for the inventory-to-world drop contract."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
BRIDGE = (ROOT / "Source/MyProject2/InventoryBridge/InventoryToPhysicalBridge.cpp").read_text(encoding="utf-8")
INVENTORY = (ROOT / "Source/MyProject2/InventoryCore/InventoryComponent.cpp").read_text(encoding="utf-8")


class InventoryDropBridgeContractTests(unittest.TestCase):
    def test_bridge_checks_world_inventory_and_item(self):
        self.assertIn("if (!World || !Inventory || !Inventory->HasItem(ItemID, 1))", BRIDGE)
        self.assertIn("if (!Self || ItemID.IsNone() || !ActorClass)", BRIDGE)

    def test_bridge_uses_drop_policy(self):
        self.assertIn("Inventory->DropItem(ItemID, 1)", BRIDGE)
        self.assertNotIn("Inventory->RemoveItem(ItemID, 1)", BRIDGE)
        self.assertIn("!D->bCanBeDropped", INVENTORY)


if __name__ == "__main__":
    unittest.main(verbosity=2)

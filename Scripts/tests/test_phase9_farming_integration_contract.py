#!/usr/bin/env python3
"""Static contracts for Phase 9 crop/feed/animal/product/manure integration."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
CROP_H = (ROOT / "Source/MyProject2/PhaseT2_Plant/CropFieldActor.h").read_text(encoding="utf-8")
CROP_CPP = (ROOT / "Source/MyProject2/PhaseT2_Plant/CropFieldActor.cpp").read_text(encoding="utf-8")
FEED_H = (ROOT / "Source/MyProject2/PhaseT6_Animal/FeedStorageComponent.h").read_text(encoding="utf-8")
FEED_CPP = (ROOT / "Source/MyProject2/PhaseT6_Animal/FeedStorageComponent.cpp").read_text(encoding="utf-8")
ANIMAL_H = (ROOT / "Source/MyProject2/PhaseT6_Animal/AnimalActor.h").read_text(encoding="utf-8")
ANIMAL_CPP = (ROOT / "Source/MyProject2/PhaseT6_Animal/AnimalActor.cpp").read_text(encoding="utf-8")
INVENTORY_CPP = (ROOT / "Source/MyProject2/InventoryCore/InventoryComponent.cpp").read_text(encoding="utf-8")
WORLD_H = (ROOT / "Source/MyProject2/WorldCore/WorldStateSubsystem.h").read_text(encoding="utf-8")
RUNNER = (ROOT / "Scripts/run_python_tests.sh").read_text(encoding="utf-8")


def section(source: str, start: str, end: str) -> str:
    begin = source.index(start)
    finish = source.index(end, begin + len(start))
    return source[begin:finish]


class FarmingIntegrationTests(unittest.TestCase):
    def test_seed_planting_debits_the_existing_inventory_and_reuses_plant_component(self):
        self.assertIn("PlantCropFromInventory", CROP_H)
        planting = section(CROP_CPP, "bool ACropFieldActor::PlantCropFromInventory", "float ACropFieldActor::HarvestToFeedStorage")
        self.assertIn("Inventory->HasItem(SeedItemID,SeedCount)", planting)
        self.assertIn("Inventory->RemoveItem(SeedItemID,SeedCount)", planting)
        self.assertIn("Def->ValidateDefinition(Error)", planting)
        self.assertIn("GrowthComponent->SetDefinition(Def)", CROP_CPP)
        self.assertIn("USimulationSubsystem", CROP_CPP)

    def test_crops_read_and_write_the_canonical_world_cell_without_double_rainfall(self):
        self.assertIn("UWorldStateSubsystem", CROP_CPP)
        self.assertIn("WorldState->GetCellAtWorld(GetActorLocation())", CROP_CPP)
        self.assertIn("SharedCell->SoilMoisture", CROP_CPP)
        self.assertIn("SharedCell->SoilNitrogen", CROP_CPP)
        self.assertIn("SharedCell->SoilPhosphorus", CROP_CPP)
        self.assertIn("SharedCell->SoilPotassium", CROP_CPP)
        daily = section(CROP_CPP, "void ACropFieldActor::UpdateDaily", "void ACropFieldActor::HandleStageChanged")
        self.assertIn("if(!SharedCell){SoilComponent->UpdateDaily", daily)
        self.assertIn("BiomassGain/Def->MaxBiomass", daily)
        self.assertIn("Chunk->Metadata.ActivationMode=EChunkActivationMode::Scheduled", CROP_CPP)

    def test_harvest_is_preflighted_before_feed_stock_is_committed(self):
        harvest = section(CROP_CPP, "float ACropFieldActor::HarvestToFeedStorage", "void ACropFieldActor::HandleSimulationDay")
        self.assertIn("HarvestComponent->CanHarvest()", harvest)
        self.assertIn("Destination->CanAddFeed(FeedID,ExpectedYield)", harvest)
        self.assertIn("Destination->TryAddFeed(FeedID,ExpectedYield,Feed)", harvest)
        self.assertIn("HarvestComponent->Harvest()", harvest)
        self.assertIn("Destination->ConsumeFeed(FeedID,ExpectedYield)", harvest)

    def test_feed_storage_is_capacity_bounded_and_rejects_invalid_values(self):
        self.assertIn("CanAddFeed", FEED_H)
        self.assertIn("TryAddFeed", FEED_H)
        self.assertIn("StorageCapacity=1000.f", FEED_H)
        self.assertIn("GetTotalFeedAmount()+A<=StorageCapacity", FEED_CPP)
        self.assertIn("FMath::IsFinite(A)", FEED_CPP)
        self.assertIn("FeedDefinitions.Add(ID,F)", FEED_CPP)

    def test_animals_accumulate_manure_from_consumed_feed(self):
        self.assertIn("UManureComponent", ANIMAL_H)
        self.assertIn('CreateDefaultSubobject<UManureComponent>(TEXT("Manure"))', ANIMAL_CPP)
        self.assertIn("ManureComponent->AccumulateManure(FeedAmount*0.25f,ProteinIntake*0.65f)", ANIMAL_CPP)
        self.assertIn("NutritionComponent->ComputeProteinIntake(FeedAmount,Feed)", ANIMAL_CPP)

    def test_milk_eggs_and_wool_enter_the_canonical_inventory_before_debit(self):
        self.assertIn("CollectProductsToInventory", ANIMAL_H)
        collect = section(ANIMAL_CPP, "int32 AAnimalActor::CollectProductsToInventory", "bool AAnimalActor::ApplyManureToSoil")
        for source in ("MilkProduction->Milking", "EggProduction->CollectEggs", "WoolProduction->AccumulatedWool"):
            self.assertIn(source, collect)
        self.assertLess(collect.index("Destination->AddItem(MilkItemID"), collect.index("MilkProduction->Milking"))
        self.assertLess(collect.index("Destination->AddItem(EggItemID"), collect.index("EggProduction->CollectEggs"))
        self.assertIn("Destination->CanAddItem", collect)
        self.assertIn("EInventoryResult::Success", INVENTORY_CPP)

    def test_manure_is_debited_once_and_nutrients_return_to_the_world_cell(self):
        manure = section(ANIMAL_CPP, "bool AAnimalActor::ApplyManureToSoil", "}")
        self.assertIn("ManureComponent->CollectManure(Amount)", manure)
        self.assertIn("WorldState->GetCellAtWorld(SoilWorldLocation)", manure)
        self.assertIn("Cell->SoilNitrogen", manure)
        self.assertIn("Cell->SoilPhosphorus", manure)
        self.assertIn("Cell->SoilPotassium", manure)
        self.assertIn("Chunk->MarkDirty()", manure)
        self.assertIn("FChunkCell* GetCellAtWorld", WORLD_H)

    def test_phase9_contract_is_included_in_the_shared_python_runner(self):
        self.assertIn("test_phase9_farming_integration_contract.py", RUNNER)


if __name__ == "__main__":
    unittest.main(verbosity=2)

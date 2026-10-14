#!/usr/bin/env python3
"""Static source-contract checks for Phase 11 production queues and resource accounting."""

from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "Source" / "MyProject2"
INV_H = (SRC / "InventoryCore" / "InventoryComponent.h").read_text(encoding="utf-8")
INV_CPP = (SRC / "InventoryCore" / "InventoryComponent.cpp").read_text(encoding="utf-8")
RECIPE_H = (SRC / "Phase11_Production" / "ProductionRecipeAsset.h").read_text(encoding="utf-8")
RECIPE_CPP = (SRC / "Phase11_Production" / "ProductionRecipeAsset.cpp").read_text(encoding="utf-8")
CHAIN_H = (SRC / "Phase11_Production" / "ProductionChainComponent.h").read_text(encoding="utf-8")
CHAIN_CPP = (SRC / "Phase11_Production" / "ProductionChainComponent.cpp").read_text(encoding="utf-8")
TYPES = (SRC / "Phase11_Production" / "ProductionChainTypes.h").read_text(encoding="utf-8")
SAVE_H = (SRC / "WorldCore" / "WorldPersistenceSubsystem.h").read_text(encoding="utf-8")
SAVE_CPP = (SRC / "WorldCore" / "WorldPersistenceSubsystem.cpp").read_text(encoding="utf-8")
BENCH_H = (SRC / "Phase6_Utility" / "CraftingBench.h").read_text(encoding="utf-8")
BENCH_CPP = (SRC / "Phase6_Utility" / "CraftingBench.cpp").read_text(encoding="utf-8")
RUNNER = (ROOT / "Scripts" / "run_python_tests.sh").read_text(encoding="utf-8")


def between(source: str, start: str, end: str) -> str:
    a = source.find(start)
    if a < 0:
        raise AssertionError(f"missing start anchor: {start}")
    b = source.find(end, a + len(start))
    if b < 0:
        raise AssertionError(f"missing end anchor after: {start}")
    return source[a:b]


class Phase11ProductionChainContract(unittest.TestCase):
    def test_inventory_batch_transaction_preflights_and_restores_exact_slots(self):
        self.assertIn("ExchangeItemBatches", INV_H)
        body = between(INV_CPP, "bool UInventoryComponent::ExchangeItemBatches(", "int32 UInventoryComponent::GetItemCount(")
        self.assertIn("RequiredInputs", body)
        self.assertIn("Source->HasItem(Pair.Key", body)
        self.assertIn("const TArray<FInventorySlot> OriginalSourceSlots", body)
        self.assertIn("const TArray<FInventorySlot> OriginalDestinationSlots", body)
        self.assertIn("if (!bCommitted)", body)
        self.assertIn("Source->Slots = OriginalSourceSlots", body)
        self.assertIn("Destination->Slots = OriginalDestinationSlots", body)
        self.assertIn("bSuppressInventoryEvents = true", body)
        self.assertIn("BroadcastChanged()", body)

    def test_recipe_asset_checks_inputs_outputs_duration_power_and_worker_bounds(self):
        self.assertIn("TArray<FProductionIngredient> Inputs", RECIPE_H)
        self.assertIn("TArray<FProductionOutput> Outputs", RECIPE_H)
        self.assertIn("RequiredPowerWatts", RECIPE_H)
        self.assertIn("RequiredWorkers", RECIPE_H)
        self.assertIn("Inputs.IsEmpty() || Outputs.IsEmpty()", RECIPE_CPP)
        self.assertIn("FMath::IsFinite(BaseDurationSeconds)", RECIPE_CPP)
        self.assertIn("FMath::IsFinite(RequiredPowerWatts)", RECIPE_CPP)
        self.assertIn("Input.Count <= 0", RECIPE_CPP)
        self.assertIn("Output.Count <= 0", RECIPE_CPP)

    def test_enqueue_reserves_inputs_atomically_before_recording_the_job(self):
        enqueue = between(CHAIN_CPP, "bool UProductionChainComponent::EnqueuePlan(", "void UProductionChainComponent::TrimCompletedHistory(")
        debit = enqueue.find("ExchangeItemBatches(InputStore, nullptr, ScaledInputs, {})")
        job_append = enqueue.find("Jobs.Add(MoveTemp(NewJob))")
        self.assertGreaterEqual(debit, 0)
        self.assertGreater(job_append, debit)
        self.assertIn("GetActiveJobCount()", enqueue)
        self.assertIn("MaxBatchCount", enqueue)
        self.assertIn("ScaleAmount", enqueue)
        self.assertIn("NewJob.ReservedInputs", enqueue)
        self.assertIn("NewJob.Outputs", enqueue)

    def test_sawmill_example_uses_existing_half_log_and_plank_item_ids(self):
        self.assertIn("QueueStandardPlankBatch", CHAIN_H)
        example = between(CHAIN_CPP, "bool UProductionChainComponent::QueueStandardPlankBatch(", "bool UProductionChainComponent::EnqueuePlan(")
        self.assertIn('FName(TEXT("HalfLog"))', example)
        self.assertIn('FName(TEXT("Plank"))', example)
        self.assertIn("5.0f, 100.0f, 1", example)

    def test_power_workers_and_pause_state_gate_progress(self):
        tick = between(CHAIN_CPP, "void UProductionChainComponent::TickComponent(", "bool UProductionChainComponent::IsValidSavedJob(")
        self.assertIn("if (bQueuePaused)", tick)
        self.assertIn("BlockedPower", tick)
        self.assertIn("AvailablePowerWatts + KINDA_SMALL_NUMBER < Job.RequiredPowerWatts", tick)
        self.assertIn("BlockedWorkers", tick)
        self.assertIn("AvailableWorkers < Job.RequiredWorkers", tick)
        self.assertIn("SafeDelta", tick)
        self.assertIn("return; // One output commit per tick", tick)

    def test_output_capacity_failure_blocks_without_losing_or_completing_product(self):
        tick = between(CHAIN_CPP, "void UProductionChainComponent::TickComponent(", "bool UProductionChainComponent::IsValidSavedJob(")
        self.assertIn("ExchangeItemBatches(nullptr, OutputStore, {}, Job.Outputs)", tick)
        self.assertIn("Job.Status = EProductionJobStatus::BlockedOutput", tick)
        self.assertIn("Job.Status = EProductionJobStatus::Completed", tick)
        self.assertLess(tick.find("BlockedOutput"), tick.find("Job.Status = EProductionJobStatus::Completed"))

    def test_cancel_refunds_reserved_inputs_through_the_same_inventory_owner(self):
        cancel = between(CHAIN_CPP, "bool UProductionChainComponent::CancelJob(", "void UProductionChainComponent::PauseQueue(")
        self.assertIn("ExchangeItemBatches(nullptr, InputStore, {}, Job.ReservedInputs)", cancel)
        self.assertIn("return false", cancel)
        self.assertIn("Job.ReservedInputs.Reset()", cancel)
        self.assertIn("EProductionJobStatus::Cancelled", cancel)

    def test_queue_and_snapshot_are_bounded_idempotent_and_validated(self):
        self.assertIn("MaxBatchCount = 100", CHAIN_H)
        self.assertIn("QueueCapacity = 8", CHAIN_H)
        self.assertIn("MaxRetainedJobs = 32", CHAIN_H)
        self.assertIn("TSet<FGuid> JobIds", CHAIN_CPP)
        self.assertIn("JobIds.Contains(Job.JobId)", CHAIN_CPP)
        self.assertIn("Snapshot.Jobs.Num() >", CHAIN_CPP)
        self.assertIn("Snapshot.StationKey != GetStableStationKey()", CHAIN_CPP)
        self.assertIn("if (!ValidateSnapshot(Snapshot))", CHAIN_CPP)
        self.assertIn("Jobs = MoveTemp(NewJobs)", CHAIN_CPP)

    def test_active_queue_requires_inventory_owners_already_saved_by_core_envelope(self):
        ownership = between(CHAIN_CPP, "bool UProductionChainComponent::UsesCorePersistedInventories(", "bool UProductionChainComponent::ValidateSnapshot(")
        self.assertIn("Owner->IsA<AForestCharacter>()", ownership)
        self.assertIn("Owner->IsA<AInventoryStorageActor>()", ownership)
        self.assertIn("!UsesCorePersistedInventories()", CHAIN_CPP)

    def test_schema_six_persists_station_jobs_and_keeps_legacy_schema_one_through_five(self):
        self.assertIn("CurrentSaveSchemaVersion = 6", SAVE_H)
        self.assertIn("bHasProductionChains", SAVE_H)
        self.assertIn("TArray<FProductionChainSnapshot> ProductionChains", SAVE_H)
        self.assertIn("SaveObject->bHasProductionChains = true", SAVE_CPP)
        self.assertIn("SaveObject->ProductionChains.Add", SAVE_CPP)
        self.assertIn("Manifest.SchemaVersion != 5 && Manifest.SchemaVersion != CurrentSaveSchemaVersion", SAVE_CPP)
        self.assertIn("LoadedSave->bHasProductionChains", SAVE_CPP)
        self.assertIn("(*Found)->ValidateSnapshot(ProductionSnapshot)", SAVE_CPP)
        self.assertIn("(*Found)->ImportSnapshot(ProductionSnapshot)", SAVE_CPP)
        self.assertIn("PreviousProductionSnapshots", SAVE_CPP)
        self.assertIn("NotifyProductionChanged", SAVE_CPP)

    def test_existing_crafting_bench_owns_the_new_queue_component(self):
        self.assertIn("UProductionChainComponent", BENCH_H)
        self.assertIn("ProductionChain", BENCH_H)
        self.assertIn('CreateDefaultSubobject<UProductionChainComponent>(TEXT("ProductionChain"))', BENCH_CPP)
        self.assertIn("ProductionChain->QueueRecipeByID", BENCH_CPP)

    def test_contract_is_registered_in_python_runner(self):
        path = "Scripts/tests/test_phase11_production_chain_contract.py"
        self.assertIn(path, RUNNER)
        self.assertIn("PHASE 11 PRODUCTION CHAIN CONTRACT", RUNNER)


if __name__ == "__main__":
    unittest.main(verbosity=2)

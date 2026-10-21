#!/usr/bin/env python3
"""Static source-contract checks for Phase 10 region and logistics integration."""

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
WORLD_CORE = ROOT / "Source" / "MyProject2" / "WorldCore"
TYPES = (WORLD_CORE / "RegionLogisticsTypes.h").read_text(encoding="utf-8")
SUBSYSTEM_H = (WORLD_CORE / "RegionLogisticsSubsystem.h").read_text(encoding="utf-8")
SUBSYSTEM_CPP = (WORLD_CORE / "RegionLogisticsSubsystem.cpp").read_text(encoding="utf-8")
PERSIST_H = (WORLD_CORE / "WorldPersistenceSubsystem.h").read_text(encoding="utf-8")
PERSIST_CPP = (WORLD_CORE / "WorldPersistenceSubsystem.cpp").read_text(encoding="utf-8")
BRIDGE_CPP = (ROOT / "Source" / "MyProject2" / "Phase4_Construction" / "BridgeComponent.cpp").read_text(encoding="utf-8")
ZIPLINE_CPP = (ROOT / "Source" / "MyProject2" / "Phase2_Transport" / "ZiplineActor.cpp").read_text(encoding="utf-8")
RUNNER = (ROOT / "Scripts" / "run_python_tests.sh").read_text(encoding="utf-8")


def body_between(source: str, start: str, end: str) -> str:
    start_at = source.find(start)
    if start_at < 0:
        raise AssertionError(f"missing function anchor: {start}")
    end_at = source.find(end, start_at + len(start))
    if end_at < 0:
        raise AssertionError(f"missing end anchor after: {start}")
    return source[start_at:end_at]


class Phase10RegionLogisticsContract(unittest.TestCase):
    def test_link_types_and_persistent_value_contract_exist(self):
        for link_type in ("Trail", "Road", "Bridge", "Ramp", "SledRoute", "Ropeway"):
            self.assertIn(link_type, TYPES)
        for field in ("NodeId", "StableKey", "WorldLocation", "FromNodeId", "ToNodeId",
                      "MaxPayloadKg", "Condition01", "Risk01", "bOperational", "bBidirectional"):
            self.assertIn(field, TYPES)
        self.assertIn("FRegionLogisticsSnapshot", TYPES)
        self.assertIn("SaveGame", TYPES)

    def test_stable_ids_and_negative_region_coordinates_use_floor_division(self):
        self.assertIn("MakeStableNodeId", SUBSYSTEM_CPP)
        self.assertIn("MakeStableLinkId", SUBSYSTEM_CPP)
        self.assertIn("if (Remainder < 0)", SUBSYSTEM_CPP)
        self.assertIn("FloorDivide(ChunkCoord.X, RegionSize)", SUBSYSTEM_CPP)
        self.assertIn("FloorDivide(ChunkCoord.Y, RegionSize)", SUBSYSTEM_CPP)

    def test_resource_and_risk_survey_reads_the_canonical_world_state(self):
        survey = body_between(
            SUBSYSTEM_CPP,
            "bool URegionLogisticsSubsystem::QueryRegionSurvey(",
            "bool URegionLogisticsSubsystem::ExportSnapshot(",
        )
        self.assertIn("GetSubsystem<UWorldStateSubsystem>()", survey)
        self.assertIn("WorldState->StateStore->GetAllChunks()", survey)
        self.assertIn("Cell.Biomass.Biomass", survey)
        self.assertIn("Cell.SoilNitrogen", survey)
        self.assertIn("Cell.Water.Volume", survey)
        self.assertIn("Cell.Chemistry.ToxinConcentration", survey)
        self.assertIn("Cell.bHasFire", survey)
        self.assertNotIn("TMap<FIntPoint, FRegionSurvey>", SUBSYSTEM_H)

    def test_transport_cost_accounts_for_payload_speed_condition_and_regional_risk(self):
        cost = body_between(
            SUBSYSTEM_CPP,
            "bool URegionLogisticsSubsystem::ComputeTraversalCost(",
            "bool URegionLogisticsSubsystem::FindLowestCostRouteByKey(",
        )
        for required in ("PayloadMassKg > Link.MaxPayloadKg", "GetLinkBaseSpeedMetersPerSecond",
                         "ConditionPenalty", "RiskPenalty", "GetEnvironmentalRiskCached",
                         "FVector::Dist", "OutSeconds"):
            self.assertIn(required, cost)
        for required_type in ("ELogisticsLinkType::Road", "ELogisticsLinkType::Bridge",
                              "ELogisticsLinkType::Ramp", "ELogisticsLinkType::SledRoute",
                              "ELogisticsLinkType::Ropeway"):
            self.assertIn(required_type, SUBSYSTEM_CPP)

    def test_route_search_is_bounded_and_respects_disabled_or_over_capacity_links(self):
        self.assertIn("MaxNodes = 2048", SUBSYSTEM_H)
        self.assertIn("MaxLinks = 8192", SUBSYSTEM_H)
        self.assertIn("TArray<uint8> Visited", SUBSYSTEM_CPP)
        self.assertIn("BestSeconds.Init(Infinity, Nodes.Num())", SUBSYSTEM_CPP)
        self.assertIn("Candidate < BestSeconds[OtherIndex]", SUBSYSTEM_CPP)
        self.assertIn("!Link.bOperational", SUBSYSTEM_CPP)
        self.assertIn("!FromNode.bEnabled || !ToNode.bEnabled", SUBSYSTEM_CPP)

    def test_snapshot_validation_rejects_duplicates_bad_values_and_dangling_edges(self):
        validation = body_between(
            SUBSYSTEM_CPP,
            "bool URegionLogisticsSubsystem::ValidateSnapshot(",
            "bool URegionLogisticsSubsystem::ImportSnapshot(",
        )
        for required in ("Snapshot.Nodes.Num() > MaxNodes", "Snapshot.Links.Num() > MaxLinks",
                         "SeenNodeIds.Contains", "SeenNodeKeys.Contains", "SeenLinkIds.Contains",
                         "SeenLinkKeys.Contains", "SeenNodeIds.Contains(Link.FromNodeId)",
                         "SeenNodeIds.Contains(Link.ToNodeId)", "IsFiniteInRange(Link.Risk01"):
            self.assertIn(required, validation)

    def test_import_validates_before_replacing_the_live_route_graph(self):
        importer = body_between(
            SUBSYSTEM_CPP,
            "bool URegionLogisticsSubsystem::ImportSnapshot(",
            "\n}",
        )
        self.assertIn("if (!ValidateSnapshot(Snapshot))", importer)
        self.assertLess(importer.find("if (!ValidateSnapshot(Snapshot))"), importer.find("Nodes = MoveTemp(NewNodes)"))
        self.assertLess(importer.find("if (!ValidateSnapshot(Snapshot))"), importer.find("Links = MoveTemp(NewLinks)"))

    def test_schema_six_saves_and_loads_routes_while_accepting_schemas_one_through_five(self):
        self.assertIn("CurrentSaveSchemaVersion = 6", PERSIST_H)
        self.assertIn("bool bHasRegionLogistics = false", PERSIST_H)
        self.assertIn("FRegionLogisticsSnapshot RegionLogistics", PERSIST_H)
        self.assertIn("SaveObject->bHasRegionLogistics = true", PERSIST_CPP)
        self.assertIn("SaveObject->RegionLogistics = MoveTemp(RegionLogisticsSnapshot)", PERSIST_CPP)
        self.assertIn("Manifest.SchemaVersion != 4", PERSIST_CPP)
        self.assertIn("LoadedSave->bHasRegionLogistics && (!Logistics || !Logistics->ValidateSnapshot", PERSIST_CPP)
        self.assertIn("Logistics->ImportSnapshot(LoadedSave->RegionLogistics)", PERSIST_CPP)
        self.assertIn("OnLogisticsChanged.AddDynamic", PERSIST_CPP)
        self.assertIn("OnLogisticsChanged.RemoveDynamic", PERSIST_CPP)

    def test_existing_bridge_and_zipline_can_register_their_infrastructure(self):
        self.assertIn("RegisterInfrastructureSegment", BRIDGE_CPP)
        self.assertIn("ELogisticsLinkType::Bridge", BRIDGE_CPP)
        self.assertIn("UnregisterInfrastructureSegment", BRIDGE_CPP)
        self.assertIn("RegisterInfrastructureSegment", ZIPLINE_CPP)
        self.assertIn("ELogisticsLinkType::Ropeway", ZIPLINE_CPP)
        self.assertIn("UnregisterInfrastructureSegment", ZIPLINE_CPP)

    def test_contract_is_wired_into_the_repository_runner(self):
        test_path = "Scripts/tests/test_phase10_region_logistics_contract.py"
        self.assertIn(test_path, RUNNER)
        self.assertIn("test_phase10_region_logistics_contract.py", RUNNER)
        self.assertIn("PHASE 10 REGION/LOGISTICS CONTRACT", RUNNER)


if __name__ == "__main__":
    unittest.main(verbosity=2)

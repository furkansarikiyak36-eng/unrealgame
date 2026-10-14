#!/usr/bin/env python3
"""Static contract checks for Phase 3 terrain mesh/collision adapters."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
WORLD_CORE = ROOT / "Source" / "MyProject2" / "WorldCore"
ACTOR_H = (WORLD_CORE / "ExcavationTerrainChunkActor.h").read_text(encoding="utf-8")
ACTOR_CPP = (WORLD_CORE / "ExcavationTerrainChunkActor.cpp").read_text(encoding="utf-8")
SUBSYSTEM_H = (WORLD_CORE / "ExcavationTerrainChunkSubsystem.h").read_text(encoding="utf-8")
SUBSYSTEM_CPP = (WORLD_CORE / "ExcavationTerrainChunkSubsystem.cpp").read_text(encoding="utf-8")
BASE_SOURCE = (WORLD_CORE / "ExcavationFlatBaseDensitySource.cpp").read_text(encoding="utf-8")
LANDSCAPE_SOURCE = (WORLD_CORE / "ExcavationLandscapeDensitySource.cpp").read_text(encoding="utf-8")
BUILD_SPEC = (ROOT / "Source" / "MyProject2" / "MyProject2.Build.cs").read_text(encoding="utf-8")
VOLUME_H = (WORLD_CORE / "ExcavationVolumeSubsystem.h").read_text(encoding="utf-8")
VOLUME_CPP = (WORLD_CORE / "ExcavationVolumeSubsystem.cpp").read_text(encoding="utf-8")


class ExcavationTerrainAdapterContractTests(unittest.TestCase):
    def test_dynamic_mesh_is_the_render_and_collision_adapter(self):
        self.assertIn("UDynamicMeshComponent", ACTOR_H)
        self.assertIn("FDynamicMesh3", ACTOR_CPP)
        self.assertIn("SetMesh(MoveTemp(CandidateMesh))", ACTOR_CPP)
        self.assertIn("SetComplexAsSimpleCollisionEnabled(true, false)", ACTOR_CPP)
        self.assertIn("SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics)", ACTOR_CPP)
        self.assertIn("UpdateCollision(false)", ACTOR_CPP)

    def test_volume_samples_generate_surface_with_bounded_triangle_budget(self):
        self.assertIn("TrySampleDensityAtWorld(WorldSample", ACTOR_CPP)
        self.assertIn("SurfaceIsoLevel = 0.5f", ACTOR_CPP)
        self.assertIn("CubeTetrahedra[6][4]", ACTOR_CPP)
        self.assertIn("MaxGeneratedTriangles", ACTOR_H)
        self.assertIn("bExceededTriangleLimit", ACTOR_CPP)
        self.assertIn("ClearMeshAndCollision();\n        return false;", ACTOR_CPP)

    def test_changes_and_load_reset_rebuild_only_registered_affected_chunks(self):
        self.assertIn("OnVolumeChanged.AddDynamic", SUBSYSTEM_CPP)
        self.assertIn("OnVolumeReset.AddDynamic", SUBSYSTEM_CPP)
        self.assertIn("GetAffectedChunksForEdit(Record)", SUBSYSTEM_CPP)
        self.assertIn("HandleVolumeReset()", SUBSYSTEM_CPP)
        self.assertIn("Actor->RebuildTerrainChunk()", SUBSYSTEM_CPP)

    def test_density_owner_prefers_active_provider_then_fallback_then_scalar(self):
        self.assertIn("RegisterFallbackBaseDensitySource", VOLUME_H)
        self.assertIn("FallbackBaseDensitySourceObject = Source", VOLUME_CPP)
        self.assertIn("IsValid(BaseDensitySourceObject)", VOLUME_CPP)
        self.assertIn("FallbackBaseDensitySourceObject.Get()", VOLUME_CPP)
        self.assertIn("return WorldPositionCm.Z <= SurfaceHeightCm ? 1.0f : 0.0f", BASE_SOURCE)

    def test_streaming_is_bounded_and_sets_coordinate_before_begin_play(self):
        self.assertIn("UTickableWorldSubsystem", SUBSYSTEM_H)
        self.assertIn("MaxChunksSpawnedPerTick", SUBSYSTEM_H)
        self.assertIn("HorizontalChunkRadius", SUBSYSTEM_H)
        self.assertIn("UndergroundChunkLayers", SUBSYSTEM_H)
        self.assertIn("SpawnActorDeferred<AExcavationTerrainChunkActor>", SUBSYSTEM_CPP)
        self.assertLess(SUBSYSTEM_CPP.find("Actor->ChunkCoordinate = Coordinate"),
                        SUBSYSTEM_CPP.find("Actor->FinishSpawning(FTransform::Identity)"))
        self.assertIn("ManagedChunkActors", SUBSYSTEM_H)

    def test_loaded_landscape_proxies_can_feed_the_base_density_layer(self):
        self.assertIn('"Landscape"', BUILD_SPEC)
        self.assertIn("ALandscapeProxy", LANDSCAPE_SOURCE)
        self.assertIn("GetHeightAtLocation", LANDSCAPE_SOURCE)
        self.assertIn("EHeightfieldSource::Simple", LANDSCAPE_SOURCE)
        self.assertIn("return WorldPositionCm.Z <= SurfaceHeight.GetValue() ? 1.0f : 0.0f", LANDSCAPE_SOURCE)
        self.assertIn("TActorIterator<ALandscapeProxy>", SUBSYSTEM_CPP)
        self.assertIn("RegisterFallbackBaseDensitySource(LandscapeFallbackDensitySource)", SUBSYSTEM_CPP)

    def test_mesh_adapter_does_not_own_a_second_density_store(self):
        self.assertNotIn("CellOverrides", ACTOR_H + ACTOR_CPP)
        self.assertNotIn("TMap<FIntVector, float>", ACTOR_H + ACTOR_CPP)
        self.assertIn("UExcavationVolumeSubsystem", ACTOR_CPP)


if __name__ == "__main__":
    unittest.main(verbosity=2)

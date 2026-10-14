#!/usr/bin/env python3
"""Static regression contracts for the contextual demo and shape-dependent wood actions."""
from __future__ import annotations

import ast
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def source(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


class ContextualDemoBuilderTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.builder = source("Scripts/build_greybox.py")
        cls.log_cpp = source("Source/MyProject2/CarvableLog.cpp")
        cls.character_cpp = source("Source/MyProject2/ForestCharacter.cpp")
        cls.structure_cpp = source("Source/MyProject2/StructureLink.cpp")
        cls.mechanics_cpp = source("Source/MyProject2/MechanicsDemo.cpp")
        cls.demo_doc = source("Docs/DEMO_CONTEXTUAL_01.md")

    def test_builder_has_valid_python_syntax(self) -> None:
        ast.parse(self.builder, filename="Scripts/build_greybox.py")

    def test_contextual_map_is_separate_from_test_fixture(self) -> None:
        self.assertIn("CONTEXTUAL_DEMO_MAP_PATH = '/Game/Maps/Demo_Contextual_01'", self.builder)
        self.assertIn("DEMO_MAP_PATH = '/Game/Maps/Demo_WoodChain'", self.builder)
        self.assertIn("def build_contextual_demo_map(made, m):", self.builder)
        self.assertIn("build_contextual_demo_map(made, m)", self.builder)
        self.assertIn("mode == 'contextual'", self.builder)

    def test_interactive_trees_and_rocks_use_harvestable_actors(self) -> None:
        self.assertIn("def spawn_harvestable(", self.builder)
        self.assertIn("unreal.EHarvestType.TREE", self.builder)
        self.assertIn("unreal.EHarvestType.ROCK", self.builder)
        self.assertIn("actor.init_from(source.static_mesh_component, harvest_type)", self.builder)
        self.assertNotIn("Example_Plank_Not_Placed", self.builder)

    def test_groove_requires_one_longitudinal_split_and_is_real_geometry(self) -> None:
        groove_start = self.log_cpp.index("bool ACarvableLog::CutGroove(")
        groove_end = self.log_cpp.index("\nvoid ACarvableLog::LetFall(", groove_start)
        groove = self.log_cpp[groove_start:groove_end]
        self.assertIn("if (!bSplitPiece || SplitCount != 1)", groove)
        self.assertIn("JointCuts.ContainsByPredicate", groove)
        self.assertIn("AppendCylinder", groove)
        self.assertIn("SubtractTool(Mesh, Cutter)", groove)
        self.assertIn("Mesh->SetMesh(Backup->GetMeshRef())", groove)
        self.assertIn("OnLogCarved.Broadcast(this, ECarveTool::Groove)", groove)

    def test_groove_snap_and_preview_are_context_gated(self) -> None:
        self.assertGreaterEqual(self.log_cpp.count("if (!bSplitPiece || SplitCount != 1"), 2)
        self.assertIn("GetLength() + 2.f * ModuleLength", self.log_cpp)

    def test_player_only_reports_a_groove_after_success(self) -> None:
        self.assertIn("if (Log->CutGroove(CarveSide))", self.character_cpp)
        self.assertIn("Yarım kütük boyunca oluk açıldı", self.character_cpp)
        self.assertIn("Oluk için önce kütüğü boyuna yarıp yarım kütük oluştur", self.character_cpp)

    def test_pointed_logs_need_ground_and_upward_point_to_be_stakes(self) -> None:
        self.assertIn("Node.Role == EWoodRole::Stake", self.structure_cpp)
        self.assertIn("!Node.bGrounded || !bPointedEndIsUp", self.structure_cpp)
        self.assertIn("Kazık bağlamı: sivri uç yukarıda", self.mechanics_cpp)
        self.assertIn("Kazık bağlamı: ucu aşağı bakan", self.mechanics_cpp)

    def test_demo_includes_valid_and_invalid_context_regressions(self) -> None:
        self.assertIn("Oluk önkoşulu: yuvarlak/boyuna yarılmamış kütükte", self.mechanics_cpp)
        self.assertIn("Oluk önkoşulu: ikinci kez boyuna yarılıp çeyrek", self.mechanics_cpp)
        self.assertIn("Oluk önizlemesi: iki kılavuz çizgisi", self.mechanics_cpp)
        self.assertIn("Demo_WoodChain", self.demo_doc)
        self.assertIn("Demo_Contextual_01", self.demo_doc)


if __name__ == "__main__":
    unittest.main()

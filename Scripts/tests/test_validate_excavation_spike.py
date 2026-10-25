import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from validate_excavation_spike import REQUIRED_COLUMNS, audit_spike_rows  # noqa: E402


def make_row(candidate, resolution, decision="Bekliyor", **overrides):
    row = {column: "" for column in REQUIRED_COLUMNS}
    row.update({
        "Aday": candidate,
        "Çözünürlük": resolution,
        "Senaryo": "Kuyu+Tünel+Oda",
        "Karar": decision,
    })
    row.update(overrides)
    return row


def valid_pending_rows():
    return [
        make_row("A_Sparse_3D_SDF", "25cm"),
        make_row("A_Sparse_3D_SDF", "50cm"),
        make_row("B_DynamicMesh_CSG", "25cm"),
        make_row("B_DynamicMesh_CSG", "50cm"),
    ]


class ExcavationSpikeAuditTests(unittest.TestCase):
    def test_expected_pending_matrix_is_valid(self):
        self.assertEqual(audit_spike_rows(valid_pending_rows(), list(REQUIRED_COLUMNS)), [])

    def test_missing_candidate_resolution_pair_is_reported(self):
        rows = valid_pending_rows()[:-1]
        errors = audit_spike_rows(rows, list(REQUIRED_COLUMNS))
        self.assertTrue(any("expected exactly 4" in error for error in errors))
        self.assertTrue(any("missing candidate/resolution" in error for error in errors))

    def test_duplicate_pair_is_rejected(self):
        rows = valid_pending_rows()
        rows[-1] = make_row("B_DynamicMesh_CSG", "25cm")
        errors = audit_spike_rows(rows, list(REQUIRED_COLUMNS))
        self.assertTrue(any("duplicate candidate/resolution" in error for error in errors))
        self.assertTrue(any("missing candidate/resolution" in error for error in errors))

    def test_unexpected_candidate_is_rejected(self):
        rows = valid_pending_rows()
        rows[0] = make_row("HeightmapOnly", "25cm")
        errors = audit_spike_rows(rows, list(REQUIRED_COLUMNS))
        self.assertTrue(any("unexpected candidate" in error for error in errors))

    def test_completed_row_requires_all_measurement_evidence(self):
        rows = valid_pending_rows()
        rows[0]["Karar"] = "Seçildi"
        errors = audit_spike_rows(rows, list(REQUIRED_COLUMNS))
        self.assertTrue(any("completed measurement row has blank evidence fields" in error for error in errors))

    def test_completed_row_passes_only_when_evidence_fields_filled(self):
        rows = valid_pending_rows()
        for column in (
            "UE_Sürümü", "Commit_SHA", "Build_Config", "CPU_GPU_RAM", "Seed",
            "Volume_Chunk_Sayısı", "Vertex_Sayısı", "Triangle_Sayısı",
            "Kazı_GameThread_ms", "Mesh_Update_ms", "Collision_Update_ms",
            "Worst_Frame_Spike_ms", "Delta_or_Snapshot_Bytes", "Reload_ms",
            "Seam_Test", "Physics_Nav_Test", "Save_Load_Hash",
            "Kısmi_İşlem_Rollback", "Kanıt_Log_Link", "Açık_Risk",
        ):
            rows[0][column] = "recorded"
        rows[0]["Karar"] = "Seçildi"
        self.assertEqual(audit_spike_rows(rows, list(REQUIRED_COLUMNS)), [])

    def test_missing_required_column_is_rejected(self):
        headers = [column for column in REQUIRED_COLUMNS if column != "Commit_SHA"]
        errors = audit_spike_rows(valid_pending_rows(), headers)
        self.assertTrue(any("missing columns" in error and "Commit_SHA" in error for error in errors))


if __name__ == "__main__":
    unittest.main(verbosity=2)

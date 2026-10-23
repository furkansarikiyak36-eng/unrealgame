#pragma once
#include "world_mechanics/core.hpp"
#include "world_mechanics/runtime_profile.hpp"
#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace wm {
struct CellCoord {
  std::int64_t x{0}, y{0}, z{0};
  bool operator==(const CellCoord& b) const noexcept { return x==b.x&&y==b.y&&z==b.z; }
  bool operator<(const CellCoord& b) const noexcept {
    if(x!=b.x) return x<b.x; if(y!=b.y) return y<b.y; return z<b.z;
  }
};
struct CellCoordHash {
  std::size_t operator()(const CellCoord& c) const noexcept;
};
struct CellValue {
  double density{1.0};
  std::uint16_t materialId{0};
};
enum class EditShapeKind { Sphere, Box, VerticalCapsule };
struct EditShape {
  EditShapeKind kind{EditShapeKind::Sphere};
  Vec3 center{};
  Vec3 halfExtents{0.5,0.5,0.5};
  double radius{0.5};
  double halfHeight{0.5};
};
struct EditRequest {
  std::string operationId;
  Revision expectedRevision{0};
  EditShape shape{};
  double densityDelta{-1.0}; // negative carves, positive fills
  std::uint16_t materialId{0}; // 0 means preserve current material; nonzero sets for changed cells
  std::uint64_t maxAffectedCells{0}; // 0 uses profile cap
};
struct PreviousCell {
  CellCoord coord{};
  bool hadOverride{false};
  CellValue oldValue{};
};
struct EditReceipt {
  std::string operationId;
  Revision oldRevision{0}, newRevision{0};
  std::vector<PreviousCell> previous;
  std::vector<ChunkCoord> dirtyChunks;
  bool changed{false};
};
struct DensityRecord { CellCoord coord{}; CellValue value{}; };
struct DensitySnapshot {
  std::uint32_t schemaVersion{1};
  Revision revision{1};
  double spacingM{0.25};
  std::uint32_t cellsPerAxis{32};
  std::vector<DensityRecord> overrides;
  std::vector<std::string> appliedOperationIds;
};

class DensityVolume {
 public:
  using BaseSource = std::function<CellValue(const Vec3&)>;
  explicit DensityVolume(RuntimeProfile profile=RuntimeProfile::LowMid(), BaseSource source={});
  Revision GetRevision() const noexcept { return revision_; }
  double SpacingM() const noexcept { return profile_.densitySpacingM; }
  std::uint32_t CellsPerAxis() const noexcept { return profile_.cellsPerAxis; }
  CellValue Sample(const CellCoord& cell) const;
  Vec3 CellPosition(const CellCoord& cell) const;
  ChunkCoord ChunkForCell(const CellCoord& cell) const;
  Result<EditReceipt> ApplyEdit(const EditRequest& request);
  Result<EditReceipt> Rollback(const EditReceipt& receipt);
  Result<DensitySnapshot> ExportSnapshot() const;
  Result<bool> ImportSnapshot(const DensitySnapshot& snapshot);
  std::size_t OverrideCount() const noexcept { return overrides_.size(); }
 private:
  Result<CellCoord> ToCell(const Vec3& p) const;
  bool InShape(const Vec3& p,const EditShape& shape) const;
  std::vector<ChunkCoord> DirtyChunksFor(const std::vector<CellCoord>& cells) const;
  RuntimeProfile profile_;
  BaseSource baseSource_;
  Revision revision_{1};
  std::unordered_map<CellCoord,CellValue,CellCoordHash> overrides_;
  std::unordered_set<std::string> appliedOperations_;
};
} // namespace wm

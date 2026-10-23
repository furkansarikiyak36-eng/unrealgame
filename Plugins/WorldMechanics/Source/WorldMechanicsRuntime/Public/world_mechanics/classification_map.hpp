#pragma once
#include "world_mechanics/world_classification.hpp"
#include <unordered_map>

namespace wm {
struct RegionCellCoord {
  std::int64_t x{0},y{0},z{0};
  bool operator==(const RegionCellCoord& b) const noexcept { return x==b.x&&y==b.y&&z==b.z; }
  bool operator<(const RegionCellCoord& b) const noexcept {
    if(x!=b.x) return x<b.x; if(y!=b.y) return y<b.y; return z<b.z;
  }
};
struct RegionCellCoordHash {
  std::size_t operator()(const RegionCellCoord& c) const noexcept;
};
struct RegionCellSample {
  RegionCellCoord cell{};
  std::vector<ClassificationEvidence> evidence;
};
struct ManualRegionOverride {
  Id overrideId{0};
  Aabb bounds{};
  ClassificationEvidence evidence{};
};
class ClassificationMap {
 public:
  ClassificationMap(Vec3 origin={},double cellSizeM=8.0);
  Revision GetRevision() const noexcept { return revision_; }
  Result<bool> SetCellEvidence(const RegionCellCoord& cell,std::vector<ClassificationEvidence> evidence);
  Result<bool> RebuildCells(const std::vector<RegionCellSample>& samples,std::size_t maxCellCount=1000000);
  Result<bool> UpsertCells(const std::vector<RegionCellSample>& samples,std::size_t maxCellCount=1000000);
  Vec3 CellCenterFor(const RegionCellCoord& cell) const { return CellCenter(cell); }
  Result<bool> AddOverride(ManualRegionOverride overrideData);
  Result<bool> RemoveOverride(Id overrideId);
  Result<ClassificationResult> ClassifyAt(const Vec3& point,
        const std::vector<ClassificationEvidence>& liveEvidence={}) const;
  std::size_t CellCount() const noexcept { return cells_.size(); }
  std::size_t OverrideCount() const noexcept { return overrides_.size(); }
 private:
  Result<RegionCellCoord> ToCell(const Vec3& point) const;
  Vec3 CellCenter(const RegionCellCoord& cell) const;
  Vec3 origin_{};
  double cellSizeM_{8};
  Revision revision_{1};
  std::unordered_map<RegionCellCoord,std::vector<ClassificationEvidence>,RegionCellCoordHash> cells_;
  std::vector<ManualRegionOverride> overrides_;
};
} // namespace wm

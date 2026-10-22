#pragma once
#include "world_mechanics/classification_map.hpp"

namespace wm {
class IClassificationProbeProvider {
 public:
  virtual ~IClassificationProbeProvider()=default;
  // A provider may use actor/component tags, Landscape layers, physical materials,
  // bounded rays, and local surface samples. Unreal calls belong in the future adapter.
  virtual Result<std::vector<ClassificationEvidence>> ProbeCell(
      const RegionCellCoord& cell,const Vec3& center) const=0;
};
struct LandscapeBuildReport {
  Revision previousRevision{0};
  Revision newRevision{0};
  std::size_t cellsProbed{0};
  std::size_t cellsMarkedUnknown{0};
};
class LandscapeBuilder {
 public:
  // Updates only the supplied dirty cells. All probes complete before the map is mutated.
  static Result<LandscapeBuildReport> BuildDirtyCells(
      ClassificationMap& map,const std::vector<RegionCellCoord>& dirtyCells,
      const IClassificationProbeProvider& provider,std::size_t maxCellsPerBatch=4096);
};
} // namespace wm

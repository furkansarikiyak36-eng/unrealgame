#include "world_mechanics/landscape_builder.hpp"
#include <unordered_set>

namespace wm {
Result<LandscapeBuildReport> LandscapeBuilder::BuildDirtyCells(
    ClassificationMap& map,const std::vector<RegionCellCoord>& dirtyCells,
    const IClassificationProbeProvider& provider,std::size_t maxCellsPerBatch) {
  if(maxCellsPerBatch==0||dirtyCells.size()>maxCellsPerBatch)
    return Result<LandscapeBuildReport>::Failure(ErrorCode::BudgetExceeded,"dirty-cell batch exceeds Landscape Builder budget");
  std::unordered_set<RegionCellCoord,RegionCellCoordHash> unique;
  unique.reserve(dirtyCells.size());
  std::vector<RegionCellSample> staged;
  staged.reserve(dirtyCells.size());
  std::size_t unknown=0;
  for(const auto& cell:dirtyCells) {
    if(!unique.insert(cell).second)
      return Result<LandscapeBuildReport>::Failure(ErrorCode::Conflict,"dirty-cell batch contains duplicate cell keys");
    auto evidence=provider.ProbeCell(cell,map.CellCenterFor(cell));
    if(!evidence) return Result<LandscapeBuildReport>::Failure(evidence.error,"classification probe failed: "+evidence.message);
    if(evidence.value.empty()) {
      evidence.value.push_back({RegionKind::Unknown,EvidenceSource::GeometryFallback,false,0,"probe had no reliable classification evidence"});
      ++unknown;
    }
    staged.push_back({cell,std::move(evidence.value)});
  }
  const Revision before=map.GetRevision();
  auto update=map.UpsertCells(staged,maxCellsPerBatch);
  if(!update) return Result<LandscapeBuildReport>::Failure(update.error,update.message);
  return Result<LandscapeBuildReport>::Success({before,map.GetRevision(),staged.size(),unknown});
}
} // namespace wm

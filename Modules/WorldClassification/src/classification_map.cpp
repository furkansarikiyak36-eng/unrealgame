#include "world_mechanics/classification_map.hpp"
#include <algorithm>
#include <limits>

namespace wm {
std::size_t RegionCellCoordHash::operator()(const RegionCellCoord& c) const noexcept {
  auto mix=[](std::uint64_t x){x^=x>>30;x*=0xbf58476d1ce4e5b9ULL;x^=x>>27;x*=0x94d049bb133111ebULL;x^=x>>31;return x;};
  return static_cast<std::size_t>(mix(static_cast<std::uint64_t>(c.x)) ^
    (mix(static_cast<std::uint64_t>(c.y)+0x9e3779b97f4a7c15ULL)<<1) ^
    (mix(static_cast<std::uint64_t>(c.z)+0x243f6a8885a308d3ULL)<<7));
}
ClassificationMap::ClassificationMap(Vec3 origin,double cellSizeM):origin_(origin),cellSizeM_(cellSizeM) {
  if(!origin_.IsFinite()) origin_={};
  if(!std::isfinite(cellSizeM_)||cellSizeM_<=0) cellSizeM_=8.0;
}
Result<RegionCellCoord> ClassificationMap::ToCell(const Vec3& p) const {
  if(!p.IsFinite()) return Result<RegionCellCoord>::Failure(ErrorCode::InvalidArgument,"classification point is not finite");
  const long double x=std::floor((static_cast<long double>(p.x)-origin_.x)/cellSizeM_);
  const long double y=std::floor((static_cast<long double>(p.y)-origin_.y)/cellSizeM_);
  const long double z=std::floor((static_cast<long double>(p.z)-origin_.z)/cellSizeM_);
  const auto lo=static_cast<long double>(std::numeric_limits<std::int64_t>::min()+1);
  const auto hi=static_cast<long double>(std::numeric_limits<std::int64_t>::max()-1);
  if(x<lo||x>hi||y<lo||y>hi||z<lo||z>hi)
    return Result<RegionCellCoord>::Failure(ErrorCode::OutOfRange,"classification position exceeds cell range");
  return Result<RegionCellCoord>::Success({static_cast<std::int64_t>(x),static_cast<std::int64_t>(y),static_cast<std::int64_t>(z)});
}
Vec3 ClassificationMap::CellCenter(const RegionCellCoord& c) const {
  return origin_+Vec3{(static_cast<double>(c.x)+0.5)*cellSizeM_,
                      (static_cast<double>(c.y)+0.5)*cellSizeM_,
                      (static_cast<double>(c.z)+0.5)*cellSizeM_};
}
Result<bool> ClassificationMap::SetCellEvidence(const RegionCellCoord& c,std::vector<ClassificationEvidence> evidence) {
  if(evidence.empty()) return Result<bool>::Failure(ErrorCode::InvalidArgument,"cell evidence cannot be empty; Unknown is represented explicitly");
  for(const auto& e:evidence) {
    if(static_cast<int>(e.source)<1||static_cast<int>(e.source)>5||
       static_cast<int>(e.kind)<0||static_cast<int>(e.kind)>static_cast<int>(RegionKind::Water))
      return Result<bool>::Failure(ErrorCode::InvalidArgument,"cell has invalid classification enum");
  }
  cells_[c]=std::move(evidence); ++revision_; return Result<bool>::Success(true);
}
Result<bool> ClassificationMap::RebuildCells(const std::vector<RegionCellSample>& samples,std::size_t maxCellCount) {
  if(maxCellCount==0||samples.size()>maxCellCount)
    return Result<bool>::Failure(ErrorCode::BudgetExceeded,"classification rebuild exceeds cell budget");
  std::unordered_map<RegionCellCoord,std::vector<ClassificationEvidence>,RegionCellCoordHash> staged;
  staged.reserve(samples.size());
  for(const auto& sample:samples) {
    if(sample.evidence.empty()) return Result<bool>::Failure(ErrorCode::InvalidArgument,"classification rebuild contains an evidence-free cell");
    for(const auto& e:sample.evidence) {
      if(static_cast<int>(e.source)<1||static_cast<int>(e.source)>5||
         static_cast<int>(e.kind)<0||static_cast<int>(e.kind)>static_cast<int>(RegionKind::Water))
        return Result<bool>::Failure(ErrorCode::InvalidArgument,"classification rebuild contains invalid enum");
    }
    if(!staged.emplace(sample.cell,sample.evidence).second)
      return Result<bool>::Failure(ErrorCode::Conflict,"classification rebuild contains duplicate cell key");
  }
  cells_.swap(staged); ++revision_;
  return Result<bool>::Success(true);
}
Result<bool> ClassificationMap::UpsertCells(const std::vector<RegionCellSample>& samples,std::size_t maxCellCount) {
  if(maxCellCount==0||samples.size()>maxCellCount)
    return Result<bool>::Failure(ErrorCode::BudgetExceeded,"classification update exceeds cell budget");

  // Validate and stage only the dirty batch. Local work should scale with
  // changed cells rather than copying the complete world classification map.
  std::unordered_map<RegionCellCoord,std::vector<ClassificationEvidence>,RegionCellCoordHash> changes;
  changes.reserve(samples.size());
  for(const auto& sample:samples) {
    if(sample.evidence.empty())
      return Result<bool>::Failure(ErrorCode::InvalidArgument,"classification update contains an evidence-free cell");
    for(const auto& e:sample.evidence) {
      if(static_cast<int>(e.source)<1||static_cast<int>(e.source)>5||
         static_cast<int>(e.kind)<0||static_cast<int>(e.kind)>static_cast<int>(RegionKind::Water))
        return Result<bool>::Failure(ErrorCode::InvalidArgument,"classification update contains invalid enum");
    }
    if(!changes.emplace(sample.cell,sample.evidence).second)
      return Result<bool>::Failure(ErrorCode::Conflict,"classification update contains duplicate cell key");
  }

  std::size_t newCellCount=0;
  for(const auto& pair:changes) {
    if(cells_.find(pair.first)==cells_.end()) ++newCellCount;
  }
  if(cells_.size()>maxCellCount||newCellCount>maxCellCount-cells_.size())
    return Result<bool>::Failure(ErrorCode::BudgetExceeded,"updated classification map exceeds total cell cap");

  struct RollbackEntry {
    RegionCellCoord cell{};
    std::vector<ClassificationEvidence> previousEvidence;
    bool inserted{false};
  };
  std::vector<RollbackEntry> rollback;
  try {
    rollback.reserve(changes.size());
    cells_.reserve(cells_.size()+newCellCount);
  } catch(const std::bad_alloc&) {
    return Result<bool>::Failure(ErrorCode::BudgetExceeded,"unable to reserve classification update memory");
  }

  auto Rollback = [&]() noexcept {
    for(auto it=rollback.rbegin();it!=rollback.rend();++it) {
      if(it->inserted) {
        cells_.erase(it->cell);
      } else {
        auto found=cells_.find(it->cell);
        if(found!=cells_.end()) found->second=std::move(it->previousEvidence);
      }
    }
  };

  try {
    for(auto& pair:changes) {
      auto existing=cells_.find(pair.first);
      if(existing!=cells_.end()) {
        rollback.push_back({pair.first,std::move(existing->second),false});
        existing->second=std::move(pair.second);
      } else {
        const auto inserted=cells_.emplace(pair.first,std::move(pair.second));
        if(!inserted.second) {
          Rollback();
          return Result<bool>::Failure(ErrorCode::Conflict,"classification cell changed during staged update");
        }
        rollback.push_back({pair.first,{},true});
      }
    }
  } catch(const std::bad_alloc&) {
    Rollback();
    return Result<bool>::Failure(ErrorCode::BudgetExceeded,"classification update ran out of memory; prior state restored");
  } catch(...) {
    Rollback();
    return Result<bool>::Failure(ErrorCode::Failed,"classification update failed; prior state restored");
  }

  ++revision_;
  return Result<bool>::Success(true);
}

Result<bool> ClassificationMap::AddOverride(ManualRegionOverride o) {
  if(o.overrideId==0||!o.bounds.IsValid()||
     static_cast<int>(o.evidence.kind)<0||static_cast<int>(o.evidence.kind)>static_cast<int>(RegionKind::Water))
    return Result<bool>::Failure(ErrorCode::InvalidArgument,"invalid manual region override");
  if(std::any_of(overrides_.begin(),overrides_.end(),[&](const auto& current){return current.overrideId==o.overrideId;}))
    return Result<bool>::Failure(ErrorCode::Conflict,"duplicate override ID");
  o.evidence.source=EvidenceSource::ManualOverride;
  o.evidence.sourceId=o.overrideId;
  overrides_.push_back(std::move(o));
  std::stable_sort(overrides_.begin(),overrides_.end(),[](const auto& a,const auto& b){return a.overrideId<b.overrideId;});
  ++revision_; return Result<bool>::Success(true);
}
Result<bool> ClassificationMap::RemoveOverride(Id id) {
  const auto old=overrides_.size();
  overrides_.erase(std::remove_if(overrides_.begin(),overrides_.end(),[&](const auto& o){return o.overrideId==id;}),overrides_.end());
  if(old==overrides_.size()) return Result<bool>::Failure(ErrorCode::NotFound,"override ID not found");
  ++revision_; return Result<bool>::Success(true);
}
Result<ClassificationResult> ClassificationMap::ClassifyAt(const Vec3& point,
        const std::vector<ClassificationEvidence>& liveEvidence) const {
  auto cell=ToCell(point); if(!cell) return Result<ClassificationResult>::Failure(cell.error,cell.message);
  std::vector<ClassificationEvidence> candidates=liveEvidence;
  const auto found=cells_.find(cell.value);
  if(found!=cells_.end()) {
    for(auto e:found->second) {
      if(e.source==EvidenceSource::GeometryFallback) e.source=EvidenceSource::ExistingCell;
      candidates.push_back(std::move(e));
    }
  }
  for(const auto& o:overrides_) {
    if(o.bounds.Contains(point)) candidates.push_back(o.evidence);
  }
  return Result<ClassificationResult>::Success(RegionClassifier::Classify(std::move(candidates),revision_));
}
} // namespace wm

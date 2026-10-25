#pragma once
#include "world_mechanics/core.hpp"
#include <string>

namespace wm {
enum class RegionKind { Unknown, EditableVoxel, StaticGeometry, Transition, Water };
enum class EvidenceSource { GeometryFallback=1, ExistingCell=2, LayerMarker=3, ManualOverride=4, ExplicitTag=5 };
struct ClassificationEvidence {
  RegionKind kind{RegionKind::Unknown};
  EvidenceSource source{EvidenceSource::GeometryFallback};
  bool explicitDeny{false};
  Id sourceId{0};
  std::string reason;
};
struct ClassificationResult {
  RegionKind kind{RegionKind::Unknown};
  bool canDig{false}, canSnap{false}, canSupport{false}, isWater{false};
  Revision mapRevision{0};
  Id winningSource{0};
  std::string reason{"no evidence"};
};

class RegionClassifier {
 public:
  static ClassificationResult Classify(std::vector<ClassificationEvidence> evidence, Revision revision) {
    if(evidence.empty()) return {RegionKind::Unknown,false,false,false,false,revision,0,"no evidence"};
    std::stable_sort(evidence.begin(),evidence.end(),[](const auto& a,const auto& b){
      if(a.source!=b.source) return static_cast<int>(a.source)>static_cast<int>(b.source);
      if(a.explicitDeny!=b.explicitDeny) return a.explicitDeny>b.explicitDeny;
      if(a.sourceId!=b.sourceId) return a.sourceId<b.sourceId;
      return static_cast<int>(a.kind)<static_cast<int>(b.kind);
    });
    const auto& winner=evidence.front();
    ClassificationResult r;
    r.kind=winner.kind; r.mapRevision=revision; r.winningSource=winner.sourceId;
    r.reason=winner.reason.empty()?"classification evidence selected by precedence":winner.reason;
    r.canDig=winner.kind==RegionKind::EditableVoxel&&!winner.explicitDeny;
    r.canSnap=winner.kind!=RegionKind::Unknown&&winner.kind!=RegionKind::Water&&!winner.explicitDeny;
    r.canSupport=winner.kind==RegionKind::EditableVoxel||winner.kind==RegionKind::StaticGeometry;
    r.isWater=winner.kind==RegionKind::Water;
    return r;
  }
};
} // namespace wm

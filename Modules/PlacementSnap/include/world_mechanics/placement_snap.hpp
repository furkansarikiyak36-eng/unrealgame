#pragma once
#include "world_mechanics/core.hpp"
#include <vector>

namespace wm {
struct SnapCandidate {
  Id candidateId{0};
  Transform transform{};
  Vec3 surfaceNormal{0,0,1};
  double distanceM{0};
  bool explicitSocket{false};
  bool regionAllowed{false};
  bool overlapFree{false};
  Revision surfaceRevision{0};
  Revision classificationRevision{0};
};
struct SnapRequest {
  Vec3 desiredNormal{0,0,1};
  Vec3 desiredAxis{1,0,0};
  double maxDistanceM{0.5};
  double minimumNormalDot{0.8660254038}; // cosine of 30 degrees
  double minimumAxisDot{0.8660254038};
  Revision expectedSurfaceRevision{0};
  Revision expectedClassificationRevision{0};
};
class PlacementSnapResolver {
 public:
  static Result<SnapCandidate> Resolve(const SnapRequest& req,const std::vector<SnapCandidate>& candidates) {
    if(!req.desiredNormal.IsFinite()||!req.desiredAxis.IsFinite()||
       LengthSquared(req.desiredNormal)<1e-12||LengthSquared(req.desiredAxis)<1e-12||
       !std::isfinite(req.maxDistanceM)||req.maxDistanceM<=0||
       !std::isfinite(req.minimumNormalDot)||!std::isfinite(req.minimumAxisDot))
      return Result<SnapCandidate>::Failure(ErrorCode::InvalidArgument,"invalid snap request");
    std::vector<SnapCandidate> eligible;
    const Vec3 normal=Normalize(req.desiredNormal), axis=Normalize(req.desiredAxis);
    for(const auto& c:candidates) {
      if(c.candidateId==0||!c.transform.IsFinite()||!c.surfaceNormal.IsFinite()||
         !std::isfinite(c.distanceM)||c.distanceM<0||c.distanceM>req.maxDistanceM||
         !c.regionAllowed||!c.overlapFree) continue;
      if(req.expectedSurfaceRevision && c.surfaceRevision!=req.expectedSurfaceRevision) continue;
      if(req.expectedClassificationRevision && c.classificationRevision!=req.expectedClassificationRevision) continue;
      const Vec3 n=Normalize(c.surfaceNormal), f=Normalize(c.transform.forward);
      if(LengthSquared(n)<1e-12||LengthSquared(f)<1e-12) continue;
      if(Dot(n,normal)<req.minimumNormalDot||std::abs(Dot(f,axis))<req.minimumAxisDot) continue;
      eligible.push_back(c);
    }
    if(eligible.empty()) return Result<SnapCandidate>::Failure(ErrorCode::NotFound,"no candidate passed snap filters");
    std::sort(eligible.begin(),eligible.end(),[](const auto& a,const auto& b){
      if(a.explicitSocket!=b.explicitSocket) return a.explicitSocket>b.explicitSocket;
      if(std::abs(a.distanceM-b.distanceM)>1e-9) return a.distanceM<b.distanceM;
      return a.candidateId<b.candidateId;
    });
    return Result<SnapCandidate>::Success(eligible.front());
  }
  static Result<bool> ValidateCommit(const SnapCandidate& c,Revision currentSurfaceRevision,
                                     Revision currentClassificationRevision,bool overlapStillFree) {
    if(c.candidateId==0) return Result<bool>::Failure(ErrorCode::InvalidArgument,"candidate ID is missing");
    if(c.surfaceRevision!=currentSurfaceRevision||c.classificationRevision!=currentClassificationRevision)
      return Result<bool>::Failure(ErrorCode::StaleRevision,"surface or classification changed after preview");
    if(!overlapStillFree) return Result<bool>::Failure(ErrorCode::Conflict,"candidate volume is now obstructed");
    return Result<bool>::Success(true);
  }
};
} // namespace wm

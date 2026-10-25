#pragma once
#include "world_mechanics/core.hpp"
#include <cmath>
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
  bool targetCompatible{true};
  bool supportValid{true};
  bool socketOccupied{false};
};
struct SnapRequest {
  Vec3 desiredNormal{0,0,1};
  Vec3 desiredAxis{1,0,0};
  double maxDistanceM{0.5};
  double minimumNormalDot{0.8660254038}; // cosine of 30 degrees
  double minimumAxisDot{0.8660254038};
  Revision expectedSurfaceRevision{0};
  Revision expectedClassificationRevision{0};
  std::size_t maxCandidates{512};
  bool requireExplicitSocket{false};
};
class PlacementSnapResolver {
 public:
  // Plank-only metric grid helper. Log sockets and terrain tools must not use
  // this policy implicitly; callers opt into the 20 cm plank grid explicitly.
  static Result<Vec3> SnapPlankPositionToGrid(const Vec3& desiredPosition,
                                             const Vec3& gridOrigin,
                                             double spacingM=0.20) {
    if(!desiredPosition.IsFinite()||!gridOrigin.IsFinite()||
       !std::isfinite(spacingM)||spacingM<=0.0)
      return Result<Vec3>::Failure(ErrorCode::InvalidArgument,"invalid plank grid position, origin or spacing");
    const Vec3 relative=desiredPosition-gridOrigin;
    const Vec3 cells{relative.x/spacingM,relative.y/spacingM,relative.z/spacingM};
    if(!cells.IsFinite())
      return Result<Vec3>::Failure(ErrorCode::OutOfRange,"plank grid coordinate exceeds numeric range");
    const Vec3 snapped=gridOrigin+Vec3{
      std::round(cells.x)*spacingM,
      std::round(cells.y)*spacingM,
      std::round(cells.z)*spacingM
    };
    if(!snapped.IsFinite())
      return Result<Vec3>::Failure(ErrorCode::OutOfRange,"snapped plank position is not finite");
    return Result<Vec3>::Success(snapped);
  }
  static Result<SnapCandidate> Resolve(const SnapRequest& req,const std::vector<SnapCandidate>& candidates) {
    if(!req.desiredNormal.IsFinite()||!req.desiredAxis.IsFinite()||
       LengthSquared(req.desiredNormal)<1e-12||LengthSquared(req.desiredAxis)<1e-12||
       !std::isfinite(req.maxDistanceM)||req.maxDistanceM<=0||
       !std::isfinite(req.minimumNormalDot)||req.minimumNormalDot<0||req.minimumNormalDot>1||
       !std::isfinite(req.minimumAxisDot)||req.minimumAxisDot<0||req.minimumAxisDot>1||
       req.maxCandidates==0)
      return Result<SnapCandidate>::Failure(ErrorCode::InvalidArgument,"invalid snap request");
    if(candidates.size()>req.maxCandidates)
      return Result<SnapCandidate>::Failure(ErrorCode::BudgetExceeded,"snap candidate set exceeds configured budget");
    std::vector<SnapCandidate> eligible;
    const Vec3 normal=Normalize(req.desiredNormal), axis=Normalize(req.desiredAxis);
    for(const auto& c:candidates) {
      if(c.candidateId==0||!c.transform.IsFinite()||!c.surfaceNormal.IsFinite()||
         !std::isfinite(c.distanceM)||c.distanceM<0||c.distanceM>req.maxDistanceM||
         !c.regionAllowed||!c.overlapFree||!c.targetCompatible||!c.supportValid||
         (c.explicitSocket&&c.socketOccupied)||(req.requireExplicitSocket&&!c.explicitSocket)) continue;
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
      // Exact total ordering avoids epsilon-based comparators that can violate
      // strict-weak-ordering for three close distances.
      if(a.distanceM!=b.distanceM) return a.distanceM<b.distanceM;
      return a.candidateId<b.candidateId;
    });
    return Result<SnapCandidate>::Success(eligible.front());
  }
  static Result<bool> ValidateCommit(const SnapCandidate& c,Revision currentSurfaceRevision,
                                     Revision currentClassificationRevision,bool regionStillAllowed,
                                     bool overlapStillFree,
                                     bool targetStillCompatible=true,
                                     bool socketStillAvailable=true,
                                     bool supportStillValid=true) {
    if(c.candidateId==0||!c.transform.IsFinite()||!c.regionAllowed||!c.overlapFree||
       !c.targetCompatible||!c.supportValid||(c.explicitSocket&&c.socketOccupied))
      return Result<bool>::Failure(ErrorCode::InvalidArgument,"candidate is missing or failed preview authorization");
    if(c.surfaceRevision!=currentSurfaceRevision||c.classificationRevision!=currentClassificationRevision)
      return Result<bool>::Failure(ErrorCode::StaleRevision,"surface or classification changed after preview");
    if(!regionStillAllowed)
      return Result<bool>::Failure(ErrorCode::Unsupported,"placement region is no longer allowed");
    if(!targetStillCompatible)
      return Result<bool>::Failure(ErrorCode::Unsupported,"target no longer accepts this piece kind");
    if(!supportStillValid)
      return Result<bool>::Failure(ErrorCode::MissingDependency,"support path is no longer valid");
    if(c.explicitSocket&&!socketStillAvailable)
      return Result<bool>::Failure(ErrorCode::Conflict,"snap socket became occupied after preview");
    if(!overlapStillFree)
      return Result<bool>::Failure(ErrorCode::Conflict,"candidate volume is now obstructed");
    return Result<bool>::Success(true);
  }
};
} // namespace wm

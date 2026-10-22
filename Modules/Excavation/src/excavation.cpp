#include "world_mechanics/excavation.hpp"
#include <cmath>

namespace wm {
Result<ExcavationCommit> ExcavationService::Execute(const ExcavationRequest& req) {
  if(!classify_) return Result<ExcavationCommit>::Failure(ErrorCode::MissingDependency,"classification provider is missing");
  if(req.operationId.empty()||req.intendedTargetId==0||!req.observation.hit||
     req.observation.targetId!=req.intendedTargetId||!req.observation.point.IsFinite()||
     !req.observation.normal.IsFinite()||!std::isfinite(req.observation.distanceM)||
     !std::isfinite(req.maxTargetDistanceM)||req.maxTargetDistanceM<=0||
     !std::isfinite(req.allowedEditOffsetM)||req.allowedEditOffsetM<0)
    return Result<ExcavationCommit>::Failure(ErrorCode::InvalidArgument,"invalid excavation request or stale/mismatched hit");
  if(req.observation.distanceM>req.maxTargetDistanceM)
    return Result<ExcavationCommit>::Failure(ErrorCode::OutOfRange,"target is beyond allowed dig distance");
  if(Length(req.edit.shape.center-req.observation.point)>req.allowedEditOffsetM)
    return Result<ExcavationCommit>::Failure(ErrorCode::OutOfRange,"edit volume is too far from observed target");
  const auto cls=classify_(req.observation.point);
  if(cls.mapRevision!=req.expectedClassificationRevision)
    return Result<ExcavationCommit>::Failure(ErrorCode::StaleRevision,"classification map changed after scan");
  if(cls.kind!=RegionKind::EditableVoxel||!cls.canDig)
    return Result<ExcavationCommit>::Failure(ErrorCode::Unsupported,"target region is not permitted to be excavated");
  EditRequest edit=req.edit;
  edit.operationId=req.operationId;
  auto result=terrain_.ApplyEdit(edit);
  if(!result) return Result<ExcavationCommit>::Failure(result.error,result.message);
  return Result<ExcavationCommit>::Success({std::move(result.value),cls});
}
} // namespace wm

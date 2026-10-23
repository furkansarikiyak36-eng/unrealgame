#include "world_mechanics/excavation.hpp"
#include <cmath>

namespace wm {
Result<ExcavationCommit> ExcavationService::Execute(const ExcavationRequest& req) {
  if(!classify_)
    return Result<ExcavationCommit>::Failure(ErrorCode::MissingDependency,"classification provider is missing");
  if(!validateObservation_)
    return Result<ExcavationCommit>::Failure(ErrorCode::MissingDependency,"live observation validator is missing");

  if(req.operationId.empty()||req.intendedTargetId==0||!req.observation.hit||
     req.observation.targetId!=req.intendedTargetId||req.observation.sourceRevision==0||
     !req.observation.point.IsFinite()||!req.observation.normal.IsFinite()||
     LengthSquared(req.observation.normal)<1e-12||
     !std::isfinite(req.observation.distanceM)||req.observation.distanceM<0||
     !std::isfinite(req.maxTargetDistanceM)||req.maxTargetDistanceM<=0||
     !std::isfinite(req.allowedEditOffsetM)||req.allowedEditOffsetM<0||
     !std::isfinite(req.maxObservationDriftM)||req.maxObservationDriftM<0)
    return Result<ExcavationCommit>::Failure(ErrorCode::InvalidArgument,"invalid excavation request or observation");

  if(req.observation.distanceM>req.maxTargetDistanceM)
    return Result<ExcavationCommit>::Failure(ErrorCode::OutOfRange,"target is beyond allowed dig distance");

  // A scan hit is evidence from a moment in time, not a durable permission.
  // Query the source again immediately before classification/edit and require
  // the same stable target and source revision.
  auto currentHit=validateObservation_(req.observation);
  if(!currentHit)
    return Result<ExcavationCommit>::Failure(currentHit.error,currentHit.message.empty()
      ? "live observation validation failed" : currentHit.message);

  if(!currentHit.value.hit||currentHit.value.targetId!=req.intendedTargetId||
     currentHit.value.sourceRevision==0||
     currentHit.value.sourceRevision!=req.observation.sourceRevision)
    return Result<ExcavationCommit>::Failure(ErrorCode::StaleRevision,"observed target no longer matches the live target revision");

  if(!currentHit.value.point.IsFinite()||!currentHit.value.normal.IsFinite()||
     LengthSquared(currentHit.value.normal)<1e-12||
     !std::isfinite(currentHit.value.distanceM)||currentHit.value.distanceM<0)
    return Result<ExcavationCommit>::Failure(ErrorCode::CorruptData,"live query returned an invalid target observation");
  if(currentHit.value.distanceM>req.maxTargetDistanceM)
    return Result<ExcavationCommit>::Failure(ErrorCode::OutOfRange,"live target is beyond allowed dig distance");
  if(Length(currentHit.value.point-req.observation.point)>req.maxObservationDriftM)
    return Result<ExcavationCommit>::Failure(ErrorCode::StaleRevision,"target moved beyond the allowed observation drift");

  if(Length(req.edit.shape.center-currentHit.value.point)>req.allowedEditOffsetM)
    return Result<ExcavationCommit>::Failure(ErrorCode::OutOfRange,"edit volume is too far from the live target point");

  const auto cls=classify_(currentHit.value.point);
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

#include "world_mechanics/shoring.hpp"
#include <cmath>
#include <limits>

namespace wm {
Result<ShoringPlan> ShoringPlanner::Build(const TrenchGeometry& g,Revision sourceRevision) {
  if(g.planId==0||!g.start.IsFinite()||!g.end.IsFinite()||
     !std::isfinite(g.widthM)||g.widthM<=0||!std::isfinite(g.depthM)||g.depthM<=0||
     !std::isfinite(g.stationSpacingM)||g.stationSpacingM<=0||
     !std::isfinite(g.boardHeightM)||g.boardHeightM<=0||g.maxPieces==0)
    return Result<ShoringPlan>::Failure(ErrorCode::InvalidArgument,"invalid trench geometry or planner budget");
  const Vec3 delta=g.end-g.start; const double length=Length(delta);
  if(!std::isfinite(length)||length<1e-4)
    return Result<ShoringPlan>::Failure(ErrorCode::InvalidArgument,"trench axis must have nonzero length");
  const Vec3 axis=Normalize(delta);
  const Vec3 lateral=Normalize(Vec3{-axis.y,axis.x,0});
  if(LengthSquared(lateral)<1e-12)
    return Result<ShoringPlan>::Failure(ErrorCode::InvalidArgument,"vertical trench axis has no horizontal side vector");
  const double rawIntervals=std::ceil(length/g.stationSpacingM);
  if(rawIntervals>static_cast<double>(g.maxPieces))
    return Result<ShoringPlan>::Failure(ErrorCode::BudgetExceeded,"station count exceeds planner budget");
  const auto intervals=std::max<std::uint64_t>(1,static_cast<std::uint64_t>(rawIntervals));
  const auto stations=intervals+1;
  const auto rows=static_cast<std::uint64_t>(std::ceil(g.depthM/g.boardHeightM));
  const long double expected=static_cast<long double>(stations)*3.0L+
    static_cast<long double>(intervals)*2.0L*static_cast<long double>(rows);
  if(rows==0||expected>static_cast<long double>(g.maxPieces))
    return Result<ShoringPlan>::Failure(ErrorCode::BudgetExceeded,"planned support-piece count exceeds budget");

  ShoringPlan plan; plan.planId=g.planId; plan.sourceRevision=sourceRevision;
  plan.stationSpacingUsedM=length/static_cast<double>(intervals);
  std::uint64_t ordinal=1;
  auto push=[&](ShoringPieceKind kind,const Vec3& p,const Vec3& pieceAxis,int side,
                std::uint32_t station,std::uint32_t row) {
    plan.pieces.push_back({ordinal++,kind,p,pieceAxis,side,station,row});
  };
  for(std::uint64_t i=0;i<stations;++i) {
    const double t=static_cast<double>(i)/static_cast<double>(intervals);
    const Vec3 top=g.start+delta*t;
    for(int side : {-1,1}) {
      const Vec3 p=top+lateral*(static_cast<double>(side)*g.widthM*0.5)+Vec3{0,0,-g.depthM*0.5};
      push(ShoringPieceKind::Upright,p,{0,0,1},side,static_cast<std::uint32_t>(i),0);
    }
    const Vec3 strut=top+Vec3{0,0,-std::min(g.depthM*0.35,g.depthM-0.01)};
    push(ShoringPieceKind::CrossStrut,strut,lateral,0,static_cast<std::uint32_t>(i),0);
  }
  for(std::uint64_t i=0;i<intervals;++i) {
    const Vec3 mid=g.start+delta*((static_cast<double>(i)+0.5)/static_cast<double>(intervals));
    for(int side : {-1,1}) {
      for(std::uint64_t row=0;row<rows;++row) {
        const double zOffset=std::min(g.depthM,(static_cast<double>(row)+0.5)*g.boardHeightM);
        const Vec3 p=mid+lateral*(static_cast<double>(side)*g.widthM*0.5)+Vec3{0,0,-zOffset};
        push(ShoringPieceKind::WallBoard,p,axis,side,static_cast<std::uint32_t>(i),static_cast<std::uint32_t>(row));
      }
    }
  }
  plan.complete=plan.pieces.size()==static_cast<std::size_t>(expected);
  if(!plan.complete) plan.issues.push_back("planner output count did not match preflight estimate");
  return Result<ShoringPlan>::Success(std::move(plan));
}

Result<bool> ShoringAssembly::RegisterPlan(const ShoringPlan& plan,Id firstPieceId,double maxHealth) {
  if(plan.planId==0||plan.sourceRevision==0||!plan.complete||plan.pieces.empty()||
     firstPieceId==0||!std::isfinite(maxHealth)||maxHealth<=0)
    return Result<bool>::Failure(ErrorCode::InvalidArgument,"shoring plan is incomplete or has invalid identity/health");
  if(plans_.count(plan.planId)) return Result<bool>::Failure(ErrorCode::Conflict,"shoring plan ID already exists");
  if(plan.pieces.size()-1>std::numeric_limits<Id>::max()-firstPieceId)
    return Result<bool>::Failure(ErrorCode::OutOfRange,"piece ID range overflows");
  for(std::size_t i=0;i<plan.pieces.size();++i) {
    const Id id=firstPieceId+static_cast<Id>(i);
    if(pieces_.count(id)) return Result<bool>::Failure(ErrorCode::Conflict,"piece ID range conflicts with existing assembly");
    if(!plan.pieces[i].position.IsFinite()||!plan.pieces[i].axis.IsFinite())
      return Result<bool>::Failure(ErrorCode::CorruptData,"plan contains non-finite piece transform");
  }
  ShoringAssemblyPlan stagedPlan; stagedPlan.planId=plan.planId;
  stagedPlan.terrainRevision=plan.sourceRevision; stagedPlan.state=ShoringPlanState::Proposed;
  stagedPlan.pieceIds.reserve(plan.pieces.size());
  for(std::size_t i=0;i<plan.pieces.size();++i) {
    const auto& source=plan.pieces[i];
    const Id id=firstPieceId+static_cast<Id>(i);
    stagedPlan.pieceIds.push_back(id);
    pieces_.emplace(id,ShoringPieceInstance{id,plan.planId,source.ordinal,source.kind,
      source.position,source.axis,source.side,source.station,source.row,maxHealth,maxHealth,0,
      ShoringPieceState::Planned});
  }
  plans_.emplace(plan.planId,std::move(stagedPlan));
  ++revision_; return Result<bool>::Success(true);
}
Result<bool> ShoringAssembly::ValidatePlan(Id planId,Revision currentTerrainRevision,bool allPlacementsValid) {
  auto it=plans_.find(planId);
  if(it==plans_.end()) return Result<bool>::Failure(ErrorCode::NotFound,"shoring plan does not exist");
  auto& plan=it->second;
  if(plan.state!=ShoringPlanState::Proposed)
    return Result<bool>::Failure(ErrorCode::Conflict,"only a proposed plan can be validated");
  if(currentTerrainRevision==0||currentTerrainRevision!=plan.terrainRevision) {
    plan.state=ShoringPlanState::NeedsRevalidation; ++revision_;
    return Result<bool>::Failure(ErrorCode::StaleRevision,"terrain changed after the shoring plan was created");
  }
  if(!allPlacementsValid) {
    plan.state=ShoringPlanState::Rejected; ++revision_;
    return Result<bool>::Success(false);
  }
  plan.state=ShoringPlanState::Validated; ++revision_;
  return Result<bool>::Success(true);
}
Result<bool> ShoringAssembly::InstallPlan(Id planId,Revision currentTerrainRevision) {
  auto it=plans_.find(planId);
  if(it==plans_.end()) return Result<bool>::Failure(ErrorCode::NotFound,"shoring plan does not exist");
  auto& plan=it->second;
  if(plan.state!=ShoringPlanState::Validated)
    return Result<bool>::Failure(ErrorCode::Conflict,"plan must be validated before installation");
  if(currentTerrainRevision==0||currentTerrainRevision!=plan.terrainRevision) {
    plan.state=ShoringPlanState::NeedsRevalidation; ++revision_;
    return Result<bool>::Failure(ErrorCode::StaleRevision,"terrain changed before install commit");
  }
  for(Id id:plan.pieceIds) {
    auto piece=pieces_.find(id);
    if(piece==pieces_.end()) return Result<bool>::Failure(ErrorCode::CorruptData,"plan references missing piece record");
    piece->second.state=ShoringPieceState::Installed;
    piece->second.anchorRevision=currentTerrainRevision;
  }
  plan.state=ShoringPlanState::Installed; plan.physicallyInstalled=true; ++revision_;
  return Result<bool>::Success(true);
}
void ShoringAssembly::NotifyTerrainRevision(Revision newTerrainRevision) {
  if(newTerrainRevision==0) return;
  bool changed=false;
  for(auto& pair:plans_) {
    auto& plan=pair.second;
    if(plan.terrainRevision==newTerrainRevision) continue;
    if(plan.state==ShoringPlanState::Installed||plan.state==ShoringPlanState::Validated) {
      plan.state=ShoringPlanState::NeedsRevalidation;
      for(Id id:plan.pieceIds) {
        auto piece=pieces_.find(id);
        if(piece!=pieces_.end()&&(piece->second.state==ShoringPieceState::Installed||
           piece->second.state==ShoringPieceState::Damaged))
          piece->second.state=ShoringPieceState::NeedsRevalidation;
      }
      changed=true;
    }
  }
  if(changed) ++revision_;
}
Result<bool> ShoringAssembly::RevalidatePlan(Id planId,Revision currentTerrainRevision,bool allAnchorsValid) {
  auto it=plans_.find(planId);
  if(it==plans_.end()) return Result<bool>::Failure(ErrorCode::NotFound,"shoring plan does not exist");
  auto& plan=it->second;
  if(plan.state!=ShoringPlanState::NeedsRevalidation)
    return Result<bool>::Failure(ErrorCode::Conflict,"plan is not awaiting revalidation");
  if(currentTerrainRevision==0)
    return Result<bool>::Failure(ErrorCode::InvalidArgument,"terrain revision must be nonzero");
  if(!allAnchorsValid) {
    plan.state=ShoringPlanState::Rejected; ++revision_;
    return Result<bool>::Success(false);
  }
  for(Id id:plan.pieceIds) {
    auto piece=pieces_.find(id);
    if(piece==pieces_.end()) return Result<bool>::Failure(ErrorCode::CorruptData,"plan references missing piece record");
    if(piece->second.state==ShoringPieceState::Removed||piece->second.state==ShoringPieceState::Failed) {
      plan.state=ShoringPlanState::Rejected; ++revision_;
      return Result<bool>::Failure(ErrorCode::Conflict,"removed or failed piece requires replacement");
    }
  }
  plan.terrainRevision=currentTerrainRevision;
  if(!plan.physicallyInstalled) {
    plan.state=ShoringPlanState::Validated; ++revision_;
    return Result<bool>::Success(true);
  }
  for(Id id:plan.pieceIds) {
    auto& piece=pieces_.at(id);
    piece.anchorRevision=currentTerrainRevision;
    piece.state=piece.health+1e-9<piece.maxHealth?ShoringPieceState::Damaged:ShoringPieceState::Installed;
  }
  plan.state=ShoringPlanState::Installed; ++revision_;
  return Result<bool>::Success(true);
}
Result<bool> ShoringAssembly::DamagePiece(Id pieceId,double damage) {
  if(!std::isfinite(damage)||damage<=0)
    return Result<bool>::Failure(ErrorCode::InvalidArgument,"damage must be positive and finite");
  auto it=pieces_.find(pieceId);
  if(it==pieces_.end()) return Result<bool>::Failure(ErrorCode::NotFound,"shoring piece does not exist");
  auto& piece=it->second;
  if(piece.state!=ShoringPieceState::Installed&&piece.state!=ShoringPieceState::Damaged)
    return Result<bool>::Failure(ErrorCode::Conflict,"only a validated installed piece can take gameplay damage");
  piece.health=std::max(0.0,piece.health-damage);
  piece.state=piece.health<=0?ShoringPieceState::Failed:ShoringPieceState::Damaged;
  ++revision_; return Result<bool>::Success(true);
}
Result<bool> ShoringAssembly::RepairPiece(Id pieceId,double amount) {
  if(!std::isfinite(amount)||amount<=0)
    return Result<bool>::Failure(ErrorCode::InvalidArgument,"repair amount must be positive and finite");
  auto it=pieces_.find(pieceId);
  if(it==pieces_.end()) return Result<bool>::Failure(ErrorCode::NotFound,"shoring piece does not exist");
  auto& piece=it->second;
  if(piece.state!=ShoringPieceState::Damaged)
    return Result<bool>::Failure(ErrorCode::Conflict,"only a damaged, non-failed piece can be repaired");
  piece.health=std::min(piece.maxHealth,piece.health+amount);
  piece.state=piece.health+1e-9<piece.maxHealth?ShoringPieceState::Damaged:ShoringPieceState::Installed;
  ++revision_; return Result<bool>::Success(true);
}
Result<bool> ShoringAssembly::RemovePiece(Id pieceId) {
  auto it=pieces_.find(pieceId);
  if(it==pieces_.end()) return Result<bool>::Failure(ErrorCode::NotFound,"shoring piece does not exist");
  if(it->second.state==ShoringPieceState::Removed)
    return Result<bool>::Success(false);
  it->second.state=ShoringPieceState::Removed;
  auto plan=plans_.find(it->second.planId);
  if(plan!=plans_.end()&&plan->second.state!=ShoringPlanState::Rejected)
    plan->second.state=ShoringPlanState::NeedsRevalidation;
  ++revision_; return Result<bool>::Success(true);
}
Result<ShoringAssemblyPlan> ShoringAssembly::GetPlan(Id planId) const {
  const auto it=plans_.find(planId);
  if(it==plans_.end()) return Result<ShoringAssemblyPlan>::Failure(ErrorCode::NotFound,"shoring plan does not exist");
  return Result<ShoringAssemblyPlan>::Success(it->second);
}
Result<ShoringPieceInstance> ShoringAssembly::GetPiece(Id pieceId) const {
  const auto it=pieces_.find(pieceId);
  if(it==pieces_.end()) return Result<ShoringPieceInstance>::Failure(ErrorCode::NotFound,"shoring piece does not exist");
  return Result<ShoringPieceInstance>::Success(it->second);
}

} // namespace wm

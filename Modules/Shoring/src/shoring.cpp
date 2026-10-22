#include "world_mechanics/shoring.hpp"
#include <cmath>

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
} // namespace wm

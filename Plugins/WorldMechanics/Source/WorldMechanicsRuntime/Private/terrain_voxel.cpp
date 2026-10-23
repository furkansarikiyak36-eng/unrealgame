#include "world_mechanics/terrain_voxel.hpp"
#include <cmath>
#include <limits>
#include <set>

namespace wm {
std::size_t CellCoordHash::operator()(const CellCoord& c) const noexcept {
  auto mix=[](std::uint64_t x){ x^=x>>30; x*=0xbf58476d1ce4e5b9ULL; x^=x>>27; x*=0x94d049bb133111ebULL; x^=x>>31; return x; };
  const auto a=mix(static_cast<std::uint64_t>(c.x));
  const auto b=mix(static_cast<std::uint64_t>(c.y)+0x9e3779b97f4a7c15ULL);
  const auto d=mix(static_cast<std::uint64_t>(c.z)+0x243f6a8885a308d3ULL);
  return static_cast<std::size_t>(a^(b<<1)^(d<<7));
}

DensityVolume::DensityVolume(RuntimeProfile profile, BaseSource source)
  : profile_(profile), baseSource_(std::move(source)) {
  if(!profile_.Validate()) profile_=RuntimeProfile::LowMid();
  if(!baseSource_) baseSource_=[](const Vec3&){ return CellValue{1.0,0}; };
}
CellValue DensityVolume::Sample(const CellCoord& cell) const {
  const auto it=overrides_.find(cell);
  if(it!=overrides_.end()) return it->second;
  const auto v=baseSource_(CellPosition(cell));
  return CellValue{Clamp01(std::isfinite(v.density)?v.density:1.0),v.materialId};
}
Vec3 DensityVolume::CellPosition(const CellCoord& c) const {
  const double s=profile_.densitySpacingM;
  return {static_cast<double>(c.x)*s,static_cast<double>(c.y)*s,static_cast<double>(c.z)*s};
}
ChunkCoord DensityVolume::ChunkForCell(const CellCoord& c) const {
  const auto n=static_cast<std::int64_t>(profile_.cellsPerAxis);
  return {FloorDiv(c.x,n),FloorDiv(c.y,n),FloorDiv(c.z,n)};
}
Result<CellCoord> DensityVolume::ToCell(const Vec3& p) const {
  if(!p.IsFinite()) return Result<CellCoord>::Failure(ErrorCode::InvalidArgument,"non-finite world position");
  const long double x=std::floor(static_cast<long double>(p.x)/profile_.densitySpacingM);
  const long double y=std::floor(static_cast<long double>(p.y)/profile_.densitySpacingM);
  const long double z=std::floor(static_cast<long double>(p.z)/profile_.densitySpacingM);
  const auto lo=static_cast<long double>(std::numeric_limits<std::int64_t>::min()+1);
  const auto hi=static_cast<long double>(std::numeric_limits<std::int64_t>::max()-1);
  if(x<lo||x>hi||y<lo||y>hi||z<lo||z>hi)
    return Result<CellCoord>::Failure(ErrorCode::OutOfRange,"world position exceeds supported cell range");
  return Result<CellCoord>::Success({static_cast<std::int64_t>(x),static_cast<std::int64_t>(y),static_cast<std::int64_t>(z)});
}
bool DensityVolume::InShape(const Vec3& p,const EditShape& s) const {
  if(s.kind==EditShapeKind::Box)
    return std::abs(p.x-s.center.x)<=s.halfExtents.x &&
           std::abs(p.y-s.center.y)<=s.halfExtents.y &&
           std::abs(p.z-s.center.z)<=s.halfExtents.z;
  if(s.kind==EditShapeKind::Sphere)
    return LengthSquared(p-s.center)<=s.radius*s.radius;
  const double z0=s.center.z-s.halfHeight, z1=s.center.z+s.halfHeight;
  const double nearestZ=std::max(z0,std::min(z1,p.z));
  const Vec3 nearest{s.center.x,s.center.y,nearestZ};
  return LengthSquared(p-nearest)<=s.radius*s.radius;
}
std::vector<ChunkCoord> DensityVolume::DirtyChunksFor(const std::vector<CellCoord>& cells) const {
  std::set<ChunkCoord> dirty;
  const auto n=static_cast<std::int64_t>(profile_.cellsPerAxis);
  for(const auto& c:cells) {
    const auto chunk=ChunkForCell(c);
    const auto lx=c.x-chunk.x*n, ly=c.y-chunk.y*n, lz=c.z-chunk.z*n;
    std::vector<std::int64_t> xs{chunk.x},ys{chunk.y},zs{chunk.z};
    if(lx==0) xs.push_back(chunk.x-1); if(lx==n-1) xs.push_back(chunk.x+1);
    if(ly==0) ys.push_back(chunk.y-1); if(ly==n-1) ys.push_back(chunk.y+1);
    if(lz==0) zs.push_back(chunk.z-1); if(lz==n-1) zs.push_back(chunk.z+1);
    for(auto x:xs) for(auto y:ys) for(auto z:zs) dirty.insert({x,y,z});
  }
  return {dirty.begin(),dirty.end()};
}
Result<EditReceipt> DensityVolume::ApplyEdit(const EditRequest& req) {
  if(req.operationId.empty()) return Result<EditReceipt>::Failure(ErrorCode::InvalidArgument,"operationId is required");
  if(appliedOperations_.count(req.operationId))
    return Result<EditReceipt>::Failure(ErrorCode::Conflict,"operationId has already been applied");
  if(req.expectedRevision!=revision_)
    return Result<EditReceipt>::Failure(ErrorCode::StaleRevision,"density revision changed before edit");
  if(!req.shape.center.IsFinite()||!std::isfinite(req.densityDelta)||req.densityDelta==0||
     std::abs(req.densityDelta)>1.0)
    return Result<EditReceipt>::Failure(ErrorCode::InvalidArgument,"invalid edit center or density delta");
  if(req.shape.kind==EditShapeKind::Box &&
     (!req.shape.halfExtents.IsFinite()||req.shape.halfExtents.x<=0||req.shape.halfExtents.y<=0||req.shape.halfExtents.z<=0))
    return Result<EditReceipt>::Failure(ErrorCode::InvalidArgument,"box half extents must be positive");
  if((req.shape.kind==EditShapeKind::Sphere||req.shape.kind==EditShapeKind::VerticalCapsule) &&
     (!std::isfinite(req.shape.radius)||req.shape.radius<=0))
    return Result<EditReceipt>::Failure(ErrorCode::InvalidArgument,"shape radius must be positive");
  if(req.shape.kind==EditShapeKind::VerticalCapsule &&
     (!std::isfinite(req.shape.halfHeight)||req.shape.halfHeight<0))
    return Result<EditReceipt>::Failure(ErrorCode::InvalidArgument,"capsule half-height is invalid");

  Vec3 ext{};
  if(req.shape.kind==EditShapeKind::Box) ext=req.shape.halfExtents;
  else {
    ext={req.shape.radius,req.shape.radius,req.shape.radius+
      (req.shape.kind==EditShapeKind::VerticalCapsule?req.shape.halfHeight:0)};
  }
  auto minC=ToCell(req.shape.center-ext), maxC=ToCell(req.shape.center+ext);
  if(!minC||!maxC) return Result<EditReceipt>::Failure(ErrorCode::OutOfRange,"edit bounds exceed cell range");
  const auto min=minC.value, max=maxC.value;
  const long double nx=static_cast<long double>(max.x)-min.x+1;
  const long double ny=static_cast<long double>(max.y)-min.y+1;
  const long double nz=static_cast<long double>(max.z)-min.z+1;
  const long double bound=nx*ny*nz;
  const std::uint64_t budget=req.maxAffectedCells?std::min(req.maxAffectedCells,profile_.maxEditCells):profile_.maxEditCells;
  if(nx<=0||ny<=0||nz<=0||bound>static_cast<long double>(budget)*8.0L)
    return Result<EditReceipt>::Failure(ErrorCode::BudgetExceeded,"edit bounding volume exceeds preflight budget");

  struct Pending { CellCoord c; bool had; CellValue oldValue; CellValue newValue; };
  std::vector<Pending> pending;
  pending.reserve(static_cast<std::size_t>(std::min<long double>(bound,budget)));
  for(std::int64_t x=min.x;;++x) {
    for(std::int64_t y=min.y;;++y) {
      for(std::int64_t z=min.z;;++z) {
        const CellCoord c{x,y,z}; const Vec3 p=CellPosition(c);
        if(InShape(p,req.shape)) {
          const auto it=overrides_.find(c);
          const bool had=it!=overrides_.end();
          const CellValue old=had?it->second:baseSource_(p);
          CellValue next=old;
          next.density=Clamp01(old.density+req.densityDelta);
          if(req.materialId!=0) next.materialId=req.materialId;
          if(!std::isfinite(old.density)||old.density<0||old.density>1)
            return Result<EditReceipt>::Failure(ErrorCode::CorruptData,"base density returned invalid value");
          if(std::abs(next.density-old.density)>1e-12||next.materialId!=old.materialId) {
            pending.push_back({c,had,old,next});
            if(pending.size()>budget)
              return Result<EditReceipt>::Failure(ErrorCode::BudgetExceeded,"edit affected more cells than allowed");
          }
        }
        if(z==max.z) break;
      }
      if(y==max.y) break;
    }
    if(x==max.x) break;
  }

  EditReceipt receipt; receipt.operationId=req.operationId; receipt.oldRevision=revision_;
  std::vector<CellCoord> changed; changed.reserve(pending.size());
  for(const auto& p:pending) {
    receipt.previous.push_back({p.c,p.had,p.oldValue});
    changed.push_back(p.c);
  }
  for(const auto& p:pending) overrides_[p.c]=p.newValue;
  appliedOperations_.insert(req.operationId);
  receipt.changed=!pending.empty();
  if(receipt.changed) ++revision_;
  receipt.newRevision=revision_;
  receipt.dirtyChunks=DirtyChunksFor(changed);
  return Result<EditReceipt>::Success(std::move(receipt));
}
Result<EditReceipt> DensityVolume::Rollback(const EditReceipt& receipt) {
  if(receipt.operationId.empty()||!appliedOperations_.count(receipt.operationId))
    return Result<EditReceipt>::Failure(ErrorCode::NotFound,"operation receipt is not active");
  if(receipt.newRevision!=revision_)
    return Result<EditReceipt>::Failure(ErrorCode::Conflict,"cannot rollback across later density edits");
  for(const auto& p:receipt.previous) {
    if(p.hadOverride) overrides_[p.coord]=p.oldValue;
    else overrides_.erase(p.coord);
  }
  appliedOperations_.erase(receipt.operationId);
  EditReceipt rollback; rollback.operationId="rollback:"+receipt.operationId;
  rollback.oldRevision=revision_; rollback.changed=!receipt.previous.empty();
  if(rollback.changed) ++revision_;
  rollback.newRevision=revision_;
  std::vector<CellCoord> cells; for(const auto& p:receipt.previous) cells.push_back(p.coord);
  rollback.dirtyChunks=DirtyChunksFor(cells);
  return Result<EditReceipt>::Success(std::move(rollback));
}
Result<DensitySnapshot> DensityVolume::ExportSnapshot() const {
  if(overrides_.size()>profile_.maxEditCells*8ULL)
    return Result<DensitySnapshot>::Failure(ErrorCode::BudgetExceeded,"snapshot override count exceeds limit");
  DensitySnapshot s; s.revision=revision_; s.spacingM=profile_.densitySpacingM;
  s.cellsPerAxis=profile_.cellsPerAxis; s.overrides.reserve(overrides_.size());
  for(const auto& p:overrides_) s.overrides.push_back({p.first,p.second});
  std::sort(s.overrides.begin(),s.overrides.end(),[](const auto& a,const auto& b){return a.coord<b.coord;});
  s.appliedOperationIds.assign(appliedOperations_.begin(),appliedOperations_.end());
  std::sort(s.appliedOperationIds.begin(),s.appliedOperationIds.end());
  return Result<DensitySnapshot>::Success(std::move(s));
}
Result<bool> DensityVolume::ImportSnapshot(const DensitySnapshot& s) {
  if(s.schemaVersion!=1||s.revision==0||!std::isfinite(s.spacingM)||s.spacingM<=0||s.cellsPerAxis<2)
    return Result<bool>::Failure(ErrorCode::CorruptData,"invalid density snapshot header");
  if(std::abs(s.spacingM-profile_.densitySpacingM)>1e-9||s.cellsPerAxis!=profile_.cellsPerAxis)
    return Result<bool>::Failure(ErrorCode::Conflict,"snapshot grid does not match active profile");
  if(s.overrides.size()>profile_.maxEditCells*8ULL||s.appliedOperationIds.size()>1000000)
    return Result<bool>::Failure(ErrorCode::BudgetExceeded,"snapshot exceeds configured limits");
  std::unordered_map<CellCoord,CellValue,CellCoordHash> staged;
  for(const auto& r:s.overrides) {
    if(!std::isfinite(r.value.density)||r.value.density<0||r.value.density>1||!staged.emplace(r.coord,r.value).second)
      return Result<bool>::Failure(ErrorCode::CorruptData,"duplicate cell or invalid density in snapshot");
  }
  std::unordered_set<std::string> ops;
  for(const auto& id:s.appliedOperationIds) {
    if(id.empty()||!ops.insert(id).second)
      return Result<bool>::Failure(ErrorCode::CorruptData,"empty or duplicate operation ID in snapshot");
  }
  overrides_.swap(staged); appliedOperations_.swap(ops); revision_=s.revision;
  return Result<bool>::Success(true);
}
} // namespace wm

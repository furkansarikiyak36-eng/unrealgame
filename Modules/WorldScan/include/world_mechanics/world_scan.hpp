#pragma once
#include "world_mechanics/core.hpp"
#include "world_mechanics/runtime_profile.hpp"

namespace wm {
struct RayRequest { Vec3 origin{}, direction{1,0,0}; double maxDistanceM{5}; std::vector<Id> ignored; };
struct ShapeSweepRequest { Aabb bounds{}; Vec3 start{}, end{}; double maxDistanceM{5}; };
struct OverlapRequest { Aabb bounds{}; std::uint32_t maxResults{32}; };
struct RawHit { bool hit{false}; Id targetId{0}; Vec3 point{}, normal{}; double distanceM{0}; Revision sourceRevision{0}; };
struct SurfaceSample { Vec3 point{}, normal{}; double slopeDegrees{0}; bool valid{false}; };

class IWorldQueryProvider {
 public:
  virtual ~IWorldQueryProvider()=default;
  virtual Result<RawHit> Raycast(const RayRequest&) const {
    return Result<RawHit>::Failure(ErrorCode::Unsupported,"raycast provider is not implemented");
  }
  virtual Result<std::vector<RawHit>> ShapeSweep(const ShapeSweepRequest&) const {
    return Result<std::vector<RawHit>>::Failure(ErrorCode::Unsupported,"shape sweep provider is not implemented");
  }
  virtual Result<std::vector<RawHit>> Overlap(const OverlapRequest&) const {
    return Result<std::vector<RawHit>>::Failure(ErrorCode::Unsupported,"overlap provider is not implemented");
  }
  virtual Result<std::vector<SurfaceSample>> SampleSurface(const Aabb&,std::uint32_t) const {
    return Result<std::vector<SurfaceSample>>::Failure(ErrorCode::Unsupported,"surface sampler is not implemented");
  }
};

class WorldScan {
 public:
  WorldScan(const IWorldQueryProvider& provider, RuntimeProfile profile)
    : provider_(provider), profile_(profile) {}
  Result<RawHit> Ray(const RayRequest& request) const {
    if(!profile_.Validate()) return Result<RawHit>::Failure(ErrorCode::InvalidArgument,"invalid runtime profile");
    if(!request.origin.IsFinite()||!request.direction.IsFinite()||
       !std::isfinite(request.maxDistanceM)||request.maxDistanceM<=0)
      return Result<RawHit>::Failure(ErrorCode::InvalidArgument,"invalid ray request");
    if(LengthSquared(request.direction)<1e-12)
      return Result<RawHit>::Failure(ErrorCode::InvalidArgument,"zero ray direction");
    RayRequest normalized=request; normalized.direction=Normalize(request.direction);
    auto result=provider_.Raycast(normalized);
    if(!result) return result;
    if(result.value.hit && (!result.value.point.IsFinite()||!result.value.normal.IsFinite()||
       !std::isfinite(result.value.distanceM)||result.value.distanceM<0||
       result.value.distanceM>request.maxDistanceM+1e-6))
      return Result<RawHit>::Failure(ErrorCode::CorruptData,"provider returned invalid hit");
    if(result.value.hit && std::find(request.ignored.begin(),request.ignored.end(),result.value.targetId)!=request.ignored.end())
      return Result<RawHit>::Success(RawHit{});
    return result;
  }
  Result<std::vector<RawHit>> Overlap(const OverlapRequest& request) const {
    if(!request.bounds.IsValid()||request.maxResults==0)
      return Result<std::vector<RawHit>>::Failure(ErrorCode::InvalidArgument,"invalid overlap request");
    auto result=provider_.Overlap(request); if(!result) return result;
    if(result.value.size()>request.maxResults)
      result.value.resize(request.maxResults);
    for(const auto& hit:result.value) {
      if(hit.hit && (!hit.point.IsFinite()||!hit.normal.IsFinite()||!std::isfinite(hit.distanceM)))
        return Result<std::vector<RawHit>>::Failure(ErrorCode::CorruptData,"invalid overlap hit");
    }
    return result;
  }
 private:
  const IWorldQueryProvider& provider_;
  RuntimeProfile profile_;
};
} // namespace wm

#pragma once
#include "world_mechanics/core.hpp"

namespace wm {
enum class ProfileTier { LowMid, HighEnd };

struct RuntimeProfile {
  ProfileTier tier{ProfileTier::LowMid};
  double densitySpacingM{0.25};
  double chunkEdgeM{8.0};
  std::uint32_t cellsPerAxis{32};
  std::uint32_t maxConcurrentMeshJobs{1};
  std::uint32_t maxQueuedChunks{8};
  std::uint32_t maxMeshPublishesPerFrame{1};
  std::uint32_t maxCollisionUpdatesPerFrame{1};
  std::uint32_t maxSurfaceSamplesPerAxis{5};
  std::uint32_t maxActiveEditableChunks{32};
  std::uint64_t maxEditCells{32768};

  static RuntimeProfile LowMid() { return {}; }
  static RuntimeProfile HighEnd() {
    RuntimeProfile p; p.tier=ProfileTier::HighEnd; p.densitySpacingM=0.125;
    p.cellsPerAxis=64; p.maxConcurrentMeshJobs=3; p.maxQueuedChunks=32;
    p.maxMeshPublishesPerFrame=2; p.maxCollisionUpdatesPerFrame=2;
    p.maxSurfaceSamplesPerAxis=9; p.maxActiveEditableChunks=96;
    p.maxEditCells=262144; return p;
  }
  bool Validate(std::string* reason=nullptr) const {
    auto fail=[&](const char* msg){ if(reason) *reason=msg; return false; };
    if(!std::isfinite(densitySpacingM)||densitySpacingM<=0) return fail("densitySpacingM must be finite and positive");
    if(!std::isfinite(chunkEdgeM)||chunkEdgeM<=0) return fail("chunkEdgeM must be finite and positive");
    if(cellsPerAxis<2||maxConcurrentMeshJobs==0||maxQueuedChunks==0||
       maxMeshPublishesPerFrame==0||maxCollisionUpdatesPerFrame==0||
       maxSurfaceSamplesPerAxis<2||maxActiveEditableChunks==0||maxEditCells==0)
      return fail("profile budget or dimension is invalid");
    const double expected=densitySpacingM*static_cast<double>(cellsPerAxis);
    if(std::abs(expected-chunkEdgeM)>std::max(1e-6,chunkEdgeM*0.02))
      return fail("chunkEdgeM is inconsistent with spacing and cellsPerAxis");
    return true;
  }
};

class ProfileSessionLock {
 public:
  bool Lock(const RuntimeProfile& profile) {
    if(!profile.Validate()) return false;
    if(locked_) return profile.tier==profile_.tier &&
      profile.densitySpacingM==profile_.densitySpacingM &&
      profile.chunkEdgeM==profile_.chunkEdgeM && profile.cellsPerAxis==profile_.cellsPerAxis;
    profile_=profile; locked_=true; return true;
  }
  bool IsLocked() const noexcept { return locked_; }
  const RuntimeProfile& Get() const noexcept { return profile_; }
 private:
  bool locked_{false};
  RuntimeProfile profile_{};
};
} // namespace wm

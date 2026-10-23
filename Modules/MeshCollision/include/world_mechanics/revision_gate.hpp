#pragma once
#include "world_mechanics/core.hpp"

namespace wm {
struct DerivedJobStamp {
  ChunkCoord chunk{};
  Revision densityRevision{0};
  std::uint64_t profileFingerprint{0};
};
class DerivedRevisionGate {
 public:
  static bool MayPublish(const DerivedJobStamp& job, Revision currentDensityRevision,
                         std::uint64_t currentProfileFingerprint) noexcept {
    return job.densityRevision!=0 && job.densityRevision==currentDensityRevision &&
      job.profileFingerprint==currentProfileFingerprint;
  }
};
} // namespace wm

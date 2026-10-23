#pragma once
#include "world_mechanics/terrain_voxel.hpp"

namespace wm {
struct SnapshotCodecLimits {
  std::uint64_t maxOverrides{1'000'000};
  std::uint64_t maxOperationIds{500'000};
  std::uint32_t maxOperationIdBytes{512};
  std::uint64_t maxEncodedBytes{128ULL*1024ULL*1024ULL};
};
class DensitySnapshotCodec {
 public:
  static Result<std::vector<std::uint8_t>> Encode(
      const DensitySnapshot& snapshot,const SnapshotCodecLimits& limits={});
  static Result<DensitySnapshot> Decode(
      const std::vector<std::uint8_t>& bytes,const SnapshotCodecLimits& limits={});
};
} // namespace wm

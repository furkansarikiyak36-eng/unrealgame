#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace wm {
struct LocalStoneSample {
  std::uint64_t id{0};
  double x{0.0}, y{0.0}, z{0.0};
  bool active{true};
};
struct StoneRingConfig {
  std::size_t minStones{6};
  std::size_t maxStones{32};
  double minRadius{25.0};
  double maxRadius{110.0};
  double maxRadiusDeviation{22.0};
  double maxHeightSpan{28.0};
  double maxAngularGapDegrees{105.0};
  double maxCenterOccupancyRadius{18.0};
};
struct StoneRingAssessment {
  bool recognized{false};
  std::size_t contributingStones{0};
  double meanRadius{0.0};
  double radiusDeviation{0.0};
  double heightSpan{0.0};
  double largestAngularGapDegrees{360.0};
  std::string reason;
};
class LocalStoneRingRecognizer {
 public:
  static StoneRingAssessment Assess(const std::vector<LocalStoneSample>& samples,
                                    double centerX, double centerY, double baseZ,
                                    const StoneRingConfig& config = {});
};
} // namespace wm

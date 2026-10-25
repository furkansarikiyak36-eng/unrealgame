#include "world_mechanics/local_stone_ring_recognition.hpp"
#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace wm {
namespace {
constexpr double kPi = 3.14159265358979323846;
bool Finite(double v) { return std::isfinite(v); }
}
StoneRingAssessment LocalStoneRingRecognizer::Assess(
    const std::vector<LocalStoneSample>& samples, double centerX, double centerY,
    double baseZ, const StoneRingConfig& cfg) {
  StoneRingAssessment out;
  if (!Finite(centerX) || !Finite(centerY) || !Finite(baseZ) ||
      cfg.minStones < 3 || cfg.maxStones < cfg.minStones ||
      !Finite(cfg.minRadius) || !Finite(cfg.maxRadius) ||
      !Finite(cfg.maxRadiusDeviation) || !Finite(cfg.maxHeightSpan) ||
      !Finite(cfg.maxAngularGapDegrees) || !Finite(cfg.maxCenterOccupancyRadius) ||
      cfg.minRadius <= 0 || cfg.maxRadius <= cfg.minRadius ||
      cfg.maxRadiusDeviation < 0 || cfg.maxHeightSpan < 0 ||
      cfg.maxAngularGapDegrees <= 0 || cfg.maxAngularGapDegrees >= 360 ||
      cfg.maxCenterOccupancyRadius < 0) {
    out.reason = "invalid configuration or center";
    return out;
  }

  struct Candidate { std::uint64_t id; double radius; double z; double angle; };
  std::vector<Candidate> candidates;
  std::unordered_set<std::uint64_t> seen;
  for (const auto& s : samples) {
    if (!s.active || s.id == 0 || !seen.insert(s.id).second ||
        !Finite(s.x) || !Finite(s.y) || !Finite(s.z)) continue;
    const double dx = s.x - centerX, dy = s.y - centerY;
    const double radius = std::sqrt(dx*dx + dy*dy);
    if (radius < cfg.maxCenterOccupancyRadius) {
      out.reason = "ring center is obstructed by a local stone";
      return out;
    }
    if (radius < cfg.minRadius || radius > cfg.maxRadius ||
        std::abs(s.z - baseZ) > cfg.maxHeightSpan) continue;
    candidates.push_back({s.id, radius, s.z, std::atan2(dy, dx)});
  }
  if (candidates.size() < cfg.minStones) {
    out.reason = "too few unique active stones in the local ring band";
    return out;
  }
  if (candidates.size() > cfg.maxStones) {
    out.reason = "candidate budget exceeded";
    return out;
  }
  std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
    return a.angle == b.angle ? a.id < b.id : a.angle < b.angle;
  });
  double radiusSum = 0.0, minZ = candidates.front().z, maxZ = candidates.front().z;
  for (const auto& c : candidates) {
    radiusSum += c.radius; minZ = std::min(minZ, c.z); maxZ = std::max(maxZ, c.z);
  }
  out.contributingStones = candidates.size();
  out.meanRadius = radiusSum / static_cast<double>(candidates.size());
  for (const auto& c : candidates) out.radiusDeviation += std::abs(c.radius - out.meanRadius);
  out.radiusDeviation /= static_cast<double>(candidates.size());
  out.heightSpan = maxZ - minZ;
  out.largestAngularGapDegrees = 0.0;
  for (std::size_t i = 0; i < candidates.size(); ++i) {
    double gap = candidates[(i + 1) % candidates.size()].angle - candidates[i].angle;
    if (i + 1 == candidates.size()) gap += 2.0 * kPi;
    out.largestAngularGapDegrees = std::max(out.largestAngularGapDegrees, gap * 180.0 / kPi);
  }
  if (out.radiusDeviation > cfg.maxRadiusDeviation) {
    out.reason = "ring radius is too inconsistent"; return out;
  }
  if (out.heightSpan > cfg.maxHeightSpan) {
    out.reason = "stone heights are inconsistent"; return out;
  }
  if (out.largestAngularGapDegrees > cfg.maxAngularGapDegrees) {
    out.reason = "ring is too open; angular coverage failed"; return out;
  }
  out.recognized = true;
  out.reason = "local stone ring geometry accepted";
  return out;
}
} // namespace wm

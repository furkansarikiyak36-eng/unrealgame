#pragma once
#include "world_mechanics/core.hpp"
#include "world_mechanics/runtime_profile.hpp"
#include <algorithm>
#include <cmath>
#include <unordered_set>
#include <vector>

namespace wm {

struct RayRequest {
  Vec3 origin{}, direction{1, 0, 0};
  double maxDistanceM{5.0};
  std::vector<Id> ignored;
};

struct ShapeSweepRequest {
  Aabb bounds{};
  Vec3 start{}, end{};
  double maxDistanceM{5.0};
  std::uint32_t maxResults{64};
  std::vector<Id> ignored;
};

struct OverlapRequest {
  Aabb bounds{};
  std::uint32_t maxResults{32};
  std::vector<Id> ignored;
};

struct RawHit {
  bool hit{false};
  Id targetId{0};
  Vec3 point{}, normal{};
  double distanceM{0.0};
  Revision sourceRevision{0};
};

struct SurfaceSample {
  Vec3 point{}, normal{};
  double slopeDegrees{0.0};
  bool valid{false};
};

class IWorldQueryProvider {
 public:
  virtual ~IWorldQueryProvider() = default;

  virtual Result<RawHit> Raycast(const RayRequest&) const {
    return Result<RawHit>::Failure(ErrorCode::Unsupported, "raycast provider is not implemented");
  }
  virtual Result<std::vector<RawHit>> ShapeSweep(const ShapeSweepRequest&) const {
    return Result<std::vector<RawHit>>::Failure(ErrorCode::Unsupported, "shape sweep provider is not implemented");
  }
  virtual Result<std::vector<RawHit>> Overlap(const OverlapRequest&) const {
    return Result<std::vector<RawHit>>::Failure(ErrorCode::Unsupported, "overlap provider is not implemented");
  }
  virtual Result<std::vector<SurfaceSample>> SampleSurface(const Aabb&, std::uint32_t) const {
    return Result<std::vector<SurfaceSample>>::Failure(ErrorCode::Unsupported, "surface sampler is not implemented");
  }
};

class WorldScan {
 public:
  WorldScan(const IWorldQueryProvider& provider, RuntimeProfile profile)
      : provider_(provider), profile_(std::move(profile)) {}

  Result<RawHit> Ray(const RayRequest& request) const {
    if (!profile_.Validate()) {
      return Result<RawHit>::Failure(ErrorCode::InvalidArgument, "invalid runtime profile");
    }
    if (!request.origin.IsFinite() || !request.direction.IsFinite() ||
        !std::isfinite(request.maxDistanceM) || request.maxDistanceM <= 0.0) {
      return Result<RawHit>::Failure(ErrorCode::InvalidArgument, "invalid ray request");
    }
    if (LengthSquared(request.direction) < 1e-12) {
      return Result<RawHit>::Failure(ErrorCode::InvalidArgument, "zero ray direction");
    }
    const auto ignoredValidation = ValidateIgnored(request.ignored);
    if (!ignoredValidation) {
      return Result<RawHit>::Failure(ignoredValidation.error, ignoredValidation.message);
    }

    RayRequest normalized = request;
    normalized.direction = Normalize(request.direction);
    auto result = provider_.Raycast(normalized);
    if (!result) {
      return result;
    }
    if (!result.value.hit) {
      return Result<RawHit>::Success(RawHit{});
    }
    if (!ValidHit(result.value) || result.value.distanceM > request.maxDistanceM + 1e-6) {
      return Result<RawHit>::Failure(ErrorCode::CorruptData, "provider returned invalid/out-of-range hit");
    }
    if (IsIgnored(request.ignored, result.value.targetId)) {
      return Result<RawHit>::Success(RawHit{});
    }
    result.value.normal = Normalize(result.value.normal);
    return result;
  }

  Result<std::vector<RawHit>> ShapeSweep(const ShapeSweepRequest& request) const {
    if (!profile_.Validate() || !request.bounds.IsValid() ||
        !request.start.IsFinite() || !request.end.IsFinite() ||
        !std::isfinite(request.maxDistanceM) || request.maxDistanceM <= 0.0 ||
        request.maxResults == 0) {
      return Result<std::vector<RawHit>>::Failure(ErrorCode::InvalidArgument, "invalid shape sweep request");
    }
    if (request.maxResults > profile_.maxQueryResults) {
      return Result<std::vector<RawHit>>::Failure(ErrorCode::BudgetExceeded, "shape sweep result cap exceeds runtime profile");
    }
    const auto ignoredValidation = ValidateIgnored(request.ignored);
    if (!ignoredValidation) {
      return Result<std::vector<RawHit>>::Failure(ignoredValidation.error, ignoredValidation.message);
    }
    if (Length(request.end - request.start) > request.maxDistanceM + 1e-6) {
      return Result<std::vector<RawHit>>::Failure(ErrorCode::OutOfRange, "shape sweep path exceeds requested distance budget");
    }

    auto result = provider_.ShapeSweep(request);
    if (!result) {
      return result;
    }
    if (result.value.size() > profile_.maxQueryResults) {
      return Result<std::vector<RawHit>>::Failure(ErrorCode::BudgetExceeded, "shape sweep provider exceeded runtime result budget");
    }
    for (const auto& hit : result.value) {
      if (hit.hit && (!ValidHit(hit) || hit.distanceM > request.maxDistanceM + 1e-6)) {
        return Result<std::vector<RawHit>>::Failure(ErrorCode::CorruptData, "shape sweep provider returned an invalid hit");
      }
    }

    result.value.erase(std::remove_if(result.value.begin(), result.value.end(),
      [&](const RawHit& hit) { return !hit.hit || IsIgnored(request.ignored, hit.targetId); }),
      result.value.end());
    NormalizeHitNormals(result.value);
    SortHitsDeterministically(result.value);
    if (result.value.size() > request.maxResults) {
      result.value.resize(request.maxResults);
    }
    return result;
  }

  Result<std::vector<RawHit>> Overlap(const OverlapRequest& request) const {
    if (!profile_.Validate() || !request.bounds.IsValid() || request.maxResults == 0) {
      return Result<std::vector<RawHit>>::Failure(ErrorCode::InvalidArgument, "invalid overlap request");
    }
    if (request.maxResults > profile_.maxQueryResults) {
      return Result<std::vector<RawHit>>::Failure(ErrorCode::BudgetExceeded, "overlap result cap exceeds runtime profile");
    }
    const auto ignoredValidation = ValidateIgnored(request.ignored);
    if (!ignoredValidation) {
      return Result<std::vector<RawHit>>::Failure(ignoredValidation.error, ignoredValidation.message);
    }

    auto result = provider_.Overlap(request);
    if (!result) {
      return result;
    }
    if (result.value.size() > profile_.maxQueryResults) {
      return Result<std::vector<RawHit>>::Failure(ErrorCode::BudgetExceeded, "overlap provider exceeded runtime result budget");
    }
    for (const auto& hit : result.value) {
      if (hit.hit && !ValidHit(hit)) {
        return Result<std::vector<RawHit>>::Failure(ErrorCode::CorruptData, "overlap provider returned an invalid hit");
      }
    }

    result.value.erase(std::remove_if(result.value.begin(), result.value.end(),
      [&](const RawHit& hit) { return !hit.hit || IsIgnored(request.ignored, hit.targetId); }),
      result.value.end());
    NormalizeHitNormals(result.value);
    SortHitsDeterministically(result.value);
    if (result.value.size() > request.maxResults) {
      result.value.resize(request.maxResults);
    }
    return result;
  }

  Result<std::vector<SurfaceSample>> SampleSurface(const Aabb& bounds, std::uint32_t samplesPerAxis) const {
    if (!profile_.Validate() || !bounds.IsValid() || samplesPerAxis < 2 ||
        samplesPerAxis > profile_.maxSurfaceSamplesPerAxis) {
      return Result<std::vector<SurfaceSample>>::Failure(
        ErrorCode::InvalidArgument, "invalid surface sample request/profile budget");
    }

    const std::uint64_t requested = static_cast<std::uint64_t>(samplesPerAxis) * samplesPerAxis;
    const std::uint64_t maxAllowed =
      static_cast<std::uint64_t>(profile_.maxSurfaceSamplesPerAxis) * profile_.maxSurfaceSamplesPerAxis;
    if (requested > maxAllowed) {
      return Result<std::vector<SurfaceSample>>::Failure(ErrorCode::BudgetExceeded, "surface sample count exceeds profile budget");
    }

    auto result = provider_.SampleSurface(bounds, samplesPerAxis);
    if (!result) {
      return result;
    }
    if (result.value.size() > requested) {
      return Result<std::vector<SurfaceSample>>::Failure(ErrorCode::BudgetExceeded, "provider exceeded requested sample count");
    }
    for (auto& sample : result.value) {
      if (!sample.valid) {
        continue;
      }
      if (!sample.point.IsFinite() || !sample.normal.IsFinite() ||
          LengthSquared(sample.normal) < 1e-12 ||
          !std::isfinite(sample.slopeDegrees) || sample.slopeDegrees < 0.0 || sample.slopeDegrees > 90.0) {
        return Result<std::vector<SurfaceSample>>::Failure(ErrorCode::CorruptData, "provider returned invalid surface sample");
      }
      sample.normal = Normalize(sample.normal);
    }
    return result;
  }

 private:
  Result<bool> ValidateIgnored(const std::vector<Id>& ignored) const {
    if (ignored.size() > profile_.maxIgnoredTargets) {
      return Result<bool>::Failure(ErrorCode::BudgetExceeded, "ignored-target list exceeds runtime profile");
    }
    std::unordered_set<Id> unique;
    unique.reserve(ignored.size());
    for (const Id id : ignored) {
      if (id == 0) {
        return Result<bool>::Failure(ErrorCode::InvalidArgument, "ignored-target list contains invalid ID");
      }
      if (!unique.insert(id).second) {
        return Result<bool>::Failure(ErrorCode::Conflict, "ignored-target list contains duplicate ID");
      }
    }
    return Result<bool>::Success(true);
  }

  static bool IsIgnored(const std::vector<Id>& ignored, Id targetId) {
    return std::find(ignored.begin(), ignored.end(), targetId) != ignored.end();
  }

  static bool ValidHit(const RawHit& hit) {
    return hit.targetId != 0 && hit.sourceRevision != 0 &&
      hit.point.IsFinite() && hit.normal.IsFinite() && LengthSquared(hit.normal) >= 1e-12 &&
      std::isfinite(hit.distanceM) && hit.distanceM >= 0.0;
  }

  static void NormalizeHitNormals(std::vector<RawHit>& hits) {
    for (auto& hit : hits) {
      hit.normal = Normalize(hit.normal);
    }
  }

  static void SortHitsDeterministically(std::vector<RawHit>& hits) {
    std::stable_sort(hits.begin(), hits.end(), [](const RawHit& a, const RawHit& b) {
      if (a.distanceM != b.distanceM) return a.distanceM < b.distanceM;
      if (a.targetId != b.targetId) return a.targetId < b.targetId;
      if (a.point.x != b.point.x) return a.point.x < b.point.x;
      if (a.point.y != b.point.y) return a.point.y < b.point.y;
      if (a.point.z != b.point.z) return a.point.z < b.point.z;
      return false;
    });
  }

  const IWorldQueryProvider& provider_;
  RuntimeProfile profile_;
};

} // namespace wm

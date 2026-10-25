#include "world_mechanics/world_snap_two_anchor_solver.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace wm {
namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kGeometryEpsilon = 1e-6;

bool IsFiniteRequest(const TwoAnchorSnapRequest& r) {
  return r.pieceId != 0 &&
      std::isfinite(r.localEndAOffsetM) &&
      std::isfinite(r.localEndBOffsetM) &&
      r.localEndBOffsetM > r.localEndAOffsetM &&
      std::isfinite(r.maxLengthErrorM) && r.maxLengthErrorM >= 0.0 &&
      std::isfinite(r.maxEndPenetrationM) && r.maxEndPenetrationM >= 0.0 &&
      std::isfinite(r.maxGapM) && r.maxGapM >= 0.0 &&
      std::isfinite(r.minPitchDegrees) && std::isfinite(r.maxPitchDegrees) &&
      r.minPitchDegrees >= -90.0 && r.maxPitchDegrees <= 90.0 &&
      r.minPitchDegrees <= r.maxPitchDegrees &&
      std::isfinite(r.minimumAnchorAxisDot) &&
      r.minimumAnchorAxisDot >= 0.0 && r.minimumAnchorAxisDot <= 1.0 &&
      std::isfinite(r.requiredLoad) && r.requiredLoad >= 0.0 &&
      (!r.useDesiredCenter || r.desiredCenterM.IsFinite()) &&
      r.expectedSurfaceRevision != 0 &&
      r.expectedClassificationRevision != 0 &&
      r.expectedSupportRevision != 0 &&
      r.maxCandidateAnchors >= 2 && r.maxPairsToEvaluate > 0;
}

bool IsEligible(const TwoAnchorSnapRequest& r,
                const TwoAnchorSnapAnchor& a) {
  if (a.anchorId == 0 || !a.positionM.IsFinite() ||
      !a.longitudinalAxis.IsFinite() ||
      LengthSquared(a.longitudinalAxis) < 1e-12 ||
      !std::isfinite(a.maxSupportedLoad) || a.maxSupportedLoad < 0.0 ||
      !a.available || !a.targetCompatible || !a.supportValid ||
      a.socketOccupied || a.maxSupportedLoad < r.requiredLoad) {
    return false;
  }
  return a.surfaceRevision == r.expectedSurfaceRevision &&
         a.classificationRevision == r.expectedClassificationRevision &&
         a.supportRevision == r.expectedSupportRevision;
}

Result<TwoAnchorPlacementDecision> EvaluatePair(
    const TwoAnchorSnapRequest& r,
    const TwoAnchorSnapAnchor& a,
    const TwoAnchorSnapAnchor& b) {
  if (a.anchorId == b.anchorId) {
    return Result<TwoAnchorPlacementDecision>::Failure(
        ErrorCode::InvalidArgument, "two-anchor placement requires distinct anchor IDs");
  }
  const Vec3 segment = b.positionM - a.positionM;
  const double distance = Length(segment);
  const double requiredLength = r.localEndBOffsetM - r.localEndAOffsetM;
  if (!std::isfinite(distance) || distance <= kGeometryEpsilon) {
    return Result<TwoAnchorPlacementDecision>::Failure(
        ErrorCode::OutOfRange, "anchor separation is zero or non-finite");
  }

  const double signedLengthError = distance - requiredLength;
  const double maxGap = r.maxGapM + r.maxLengthErrorM;
  const double maxCompression = 2.0 * r.maxEndPenetrationM + r.maxLengthErrorM;
  if (signedLengthError > maxGap || signedLengthError < -maxCompression) {
    return Result<TwoAnchorPlacementDecision>::Failure(
        ErrorCode::OutOfRange, "anchor separation does not fit the piece length/penetration budget");
  }

  const Vec3 forward = segment / distance;
  const Vec3 axisA = Normalize(a.longitudinalAxis);
  const Vec3 axisB = Normalize(b.longitudinalAxis);
  if (std::abs(Dot(axisA, forward)) < r.minimumAnchorAxisDot ||
      std::abs(Dot(axisB, forward)) < r.minimumAnchorAxisDot) {
    return Result<TwoAnchorPlacementDecision>::Failure(
        ErrorCode::Unsupported, "anchor axes are incompatible with the piece direction");
  }

  const double horizontalLength = std::sqrt(forward.x * forward.x + forward.y * forward.y);
  const double yaw = std::atan2(forward.y, forward.x) * 180.0 / kPi;
  const double pitch = std::atan2(forward.z, horizontalLength) * 180.0 / kPi;
  if (pitch < r.minPitchDegrees || pitch > r.maxPitchDegrees) {
    return Result<TwoAnchorPlacementDecision>::Failure(
        ErrorCode::Unsupported, "endpoint-derived pitch is outside the allowed angle band");
  }

  // The portable Transform contains only position + forward. Actor adapters use
  // forward to derive their full Unreal rotation and preserve asset-specific roll.
  const Vec3 worldOrigin = a.positionM - forward * r.localEndAOffsetM;
  if (!worldOrigin.IsFinite()) {
    return Result<TwoAnchorPlacementDecision>::Failure(
        ErrorCode::OutOfRange, "solved actor origin is not finite");
  }

  TwoAnchorPlacementDecision d;
  d.pieceId = r.pieceId;
  d.anchorAId = a.anchorId;
  d.anchorBId = b.anchorId;
  d.anchorAOwnerId = a.ownerPieceId;
  d.anchorBOwnerId = b.ownerPieceId;
  d.transform = {worldOrigin, forward};
  d.worldEndA = a.positionM;
  d.worldEndB = b.positionM;
  d.requiredLengthM = requiredLength;
  d.actualAnchorDistanceM = distance;
  d.signedLengthErrorM = signedLengthError;
  d.absoluteLengthErrorM = std::abs(signedLengthError);
  d.yawDegrees = yaw;
  d.pitchDegrees = pitch;
  d.centerDistanceM = r.useDesiredCenter
      ? Length((a.positionM + b.positionM) * 0.5 - r.desiredCenterM)
      : 0.0;
  d.minimumAnchorCapacity = std::min(a.maxSupportedLoad, b.maxSupportedLoad);
  d.surfaceRevision = r.expectedSurfaceRevision;
  d.classificationRevision = r.expectedClassificationRevision;
  d.supportRevision = r.expectedSupportRevision;
  return Result<TwoAnchorPlacementDecision>::Success(d);
}

bool Better(const TwoAnchorSnapRequest& r,
            const TwoAnchorPlacementDecision& a,
            const TwoAnchorPlacementDecision& b) {
  if (r.useDesiredCenter && a.centerDistanceM != b.centerDistanceM)
    return a.centerDistanceM < b.centerDistanceM;
  if (a.absoluteLengthErrorM != b.absoluteLengthErrorM)
    return a.absoluteLengthErrorM < b.absoluteLengthErrorM;
  if (a.anchorAId != b.anchorAId) return a.anchorAId < b.anchorAId;
  return a.anchorBId < b.anchorBId;
}
}  // namespace

Result<TwoAnchorPlacementDecision> WorldSnapTwoAnchorSolver::Resolve(
    const TwoAnchorSnapRequest& request,
    const std::vector<TwoAnchorSnapAnchor>& anchors) {
  if (!IsFiniteRequest(request)) {
    return Result<TwoAnchorPlacementDecision>::Failure(
        ErrorCode::InvalidArgument, "invalid two-anchor placement request");
  }
  if (anchors.size() > request.maxCandidateAnchors) {
    return Result<TwoAnchorPlacementDecision>::Failure(
        ErrorCode::BudgetExceeded, "two-anchor source set exceeds candidate budget");
  }

  std::vector<TwoAnchorSnapAnchor> eligible;
  eligible.reserve(anchors.size());
  std::vector<Id> seen;
  seen.reserve(anchors.size());
  for (const auto& anchor : anchors) {
    if (anchor.anchorId == 0 ||
        std::find(seen.begin(), seen.end(), anchor.anchorId) != seen.end()) {
      return Result<TwoAnchorPlacementDecision>::Failure(
          ErrorCode::Conflict, "anchor source contains a zero or duplicate anchor ID");
    }
    seen.push_back(anchor.anchorId);
    if (IsEligible(request, anchor)) eligible.push_back(anchor);
  }
  std::sort(eligible.begin(), eligible.end(),
      [](const TwoAnchorSnapAnchor& a, const TwoAnchorSnapAnchor& b) {
        return a.anchorId < b.anchorId;
      });
  if (eligible.size() < 2) {
    return Result<TwoAnchorPlacementDecision>::Failure(
        ErrorCode::NotFound, "fewer than two eligible supported anchors remain");
  }

  std::size_t evaluated = 0;
  bool found = false;
  TwoAnchorPlacementDecision best;
  for (std::size_t i = 0; i < eligible.size(); ++i) {
    for (std::size_t j = 0; j < eligible.size(); ++j) {
      if (i == j) continue;
      if (evaluated >= request.maxPairsToEvaluate) {
        return Result<TwoAnchorPlacementDecision>::Failure(
            ErrorCode::BudgetExceeded, "two-anchor pair evaluation budget exhausted");
      }
      ++evaluated;
      auto pair = EvaluatePair(request, eligible[i], eligible[j]);
      if (!pair) continue;
      if (!found || Better(request, pair.value, best)) {
        best = pair.value;
        found = true;
      }
    }
  }
  if (!found) {
    return Result<TwoAnchorPlacementDecision>::Failure(
        ErrorCode::NotFound, "no anchor pair passed span, angle, axis and support constraints");
  }
  return Result<TwoAnchorPlacementDecision>::Success(best);
}

Result<bool> WorldSnapTwoAnchorSolver::ValidateCommit(
    const TwoAnchorSnapRequest& request,
    const TwoAnchorPlacementDecision& preview,
    const std::vector<TwoAnchorSnapAnchor>& liveAnchors,
    Revision currentSurfaceRevision,
    Revision currentClassificationRevision,
    Revision currentSupportRevision,
    bool collisionFree) {
  if (!collisionFree) {
    return Result<bool>::Failure(
        ErrorCode::Conflict, "two-anchor piece volume is obstructed at commit time");
  }
  if (currentSurfaceRevision != request.expectedSurfaceRevision ||
      currentClassificationRevision != request.expectedClassificationRevision ||
      currentSupportRevision != request.expectedSupportRevision ||
      preview.surfaceRevision != currentSurfaceRevision ||
      preview.classificationRevision != currentClassificationRevision ||
      preview.supportRevision != currentSupportRevision) {
    return Result<bool>::Failure(
        ErrorCode::StaleRevision, "world, classification or support changed after preview");
  }
  if (preview.pieceId != request.pieceId || preview.anchorAId == 0 ||
      preview.anchorBId == 0 || preview.anchorAId == preview.anchorBId) {
    return Result<bool>::Failure(
        ErrorCode::InvalidArgument, "preview does not describe this two-anchor request");
  }

  const TwoAnchorSnapAnchor* liveA = nullptr;
  const TwoAnchorSnapAnchor* liveB = nullptr;
  for (const auto& anchor : liveAnchors) {
    if (anchor.anchorId == preview.anchorAId) {
      if (liveA) return Result<bool>::Failure(ErrorCode::Conflict, "duplicate live anchor A ID");
      liveA = &anchor;
    }
    if (anchor.anchorId == preview.anchorBId) {
      if (liveB) return Result<bool>::Failure(ErrorCode::Conflict, "duplicate live anchor B ID");
      liveB = &anchor;
    }
  }
  if (!liveA || !liveB) {
    return Result<bool>::Failure(ErrorCode::NotFound, "preview anchor disappeared before commit");
  }
  auto current = Resolve(request, {*liveA, *liveB});
  if (!current) {
    return Result<bool>::Failure(current.error, "preview anchors no longer form a valid placement: " + current.message);
  }
  if (current.value.anchorAId != preview.anchorAId ||
      current.value.anchorBId != preview.anchorBId ||
      Length(current.value.transform.position - preview.transform.position) > kGeometryEpsilon ||
      Length(current.value.transform.forward - preview.transform.forward) > kGeometryEpsilon) {
    return Result<bool>::Failure(
        ErrorCode::StaleRevision, "anchor geometry changed after preview");
  }
  return Result<bool>::Success(true);
}

}  // namespace wm

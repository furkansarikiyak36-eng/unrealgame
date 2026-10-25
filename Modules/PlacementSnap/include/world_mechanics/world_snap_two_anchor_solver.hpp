#pragma once

#include "world_mechanics/core.hpp"

#include <cstddef>
#include <vector>

namespace wm {

// A provider-created anchor from a bounded local world query. positionM is in
// world meters. longitudinalAxis is the socket's preferred connection axis.
struct TwoAnchorSnapAnchor {
  Id anchorId{0};
  Id ownerPieceId{0};
  Vec3 positionM{};
  Vec3 longitudinalAxis{1,0,0};
  Revision surfaceRevision{0};
  Revision classificationRevision{0};
  Revision supportRevision{0};
  double maxSupportedLoad{0.0};
  bool available{false};
  bool targetCompatible{false};
  bool supportValid{false};
  bool socketOccupied{false};
};

struct TwoAnchorSnapRequest {
  Id pieceId{0};
  // Local longitudinal endpoint coordinates along the piece's +X axis.
  // Example: a canonical 6 m log uses -3.0 and +3.0.
  double localEndAOffsetM{-3.0};
  double localEndBOffsetM{3.0};
  double maxLengthErrorM{0.10};
  double maxEndPenetrationM{0.05};
  double maxGapM{0.05};
  double minPitchDegrees{-90.0};
  double maxPitchDegrees{90.0};
  double minimumAnchorAxisDot{0.0};
  double requiredLoad{0.0};
  Vec3 desiredCenterM{};
  bool useDesiredCenter{false};
  Revision expectedSurfaceRevision{0};
  Revision expectedClassificationRevision{0};
  Revision expectedSupportRevision{0};
  std::size_t maxCandidateAnchors{128};
  std::size_t maxPairsToEvaluate{16384};
};

struct TwoAnchorPlacementDecision {
  Id pieceId{0};
  Id anchorAId{0};
  Id anchorBId{0};
  Id anchorAOwnerId{0};
  Id anchorBOwnerId{0};
  Transform transform{};
  Vec3 worldEndA{};
  Vec3 worldEndB{};
  double requiredLengthM{0.0};
  double actualAnchorDistanceM{0.0};
  double signedLengthErrorM{0.0};
  double absoluteLengthErrorM{0.0};
  double yawDegrees{0.0};
  double pitchDegrees{0.0};
  double centerDistanceM{0.0};
  double minimumAnchorCapacity{0.0};
  Revision surfaceRevision{0};
  Revision classificationRevision{0};
  Revision supportRevision{0};
};

// Finds a stable two-ended placement from anchor geometry, not camera pitch.
// It does not spawn actors or mutate inventory/support graphs; those effects
// must be committed by the Unreal adapter after ValidateCommit succeeds.
class WorldSnapTwoAnchorSolver {
 public:
  static Result<TwoAnchorPlacementDecision> Resolve(
      const TwoAnchorSnapRequest& request,
      const std::vector<TwoAnchorSnapAnchor>& anchors);

  static Result<bool> ValidateCommit(
      const TwoAnchorSnapRequest& request,
      const TwoAnchorPlacementDecision& preview,
      const std::vector<TwoAnchorSnapAnchor>& liveAnchors,
      Revision currentSurfaceRevision,
      Revision currentClassificationRevision,
      Revision currentSupportRevision,
      bool collisionFree);
};

}  // namespace wm

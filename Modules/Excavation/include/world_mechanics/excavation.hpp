#pragma once
#include "world_mechanics/world_scan.hpp"
#include "world_mechanics/world_classification.hpp"
#include "world_mechanics/terrain_voxel.hpp"
#include <functional>

namespace wm {
struct ExcavationRequest {
  std::string operationId;
  Id intendedTargetId{0};
  RawHit observation{};
  Revision expectedClassificationRevision{0};
  double maxTargetDistanceM{5.0};
  double allowedEditOffsetM{0.75};
  double maxObservationDriftM{0.15};
  EditRequest edit{};
};
struct ExcavationCommit {
  EditReceipt editReceipt{};
  ClassificationResult classification{};
};
class ExcavationService {
 public:
  using ClassifyAt = std::function<ClassificationResult(const Vec3&)>;
  using ValidateObservation = std::function<Result<RawHit>(const RawHit&)>;

  ExcavationService(DensityVolume& terrain,ClassifyAt classify,ValidateObservation validateObservation)
    : terrain_(terrain),classify_(std::move(classify)),
      validateObservation_(std::move(validateObservation)) {}

  Result<ExcavationCommit> Execute(const ExcavationRequest& request);
 private:
  DensityVolume& terrain_;
  ClassifyAt classify_;
  ValidateObservation validateObservation_;
};
} // namespace wm

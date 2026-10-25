#include "world_mechanics/stone_fireplace_lifecycle.hpp"

#include <algorithm>
#include <cmath>

namespace wm {

StoneFireplaceLifecycle::StoneFireplaceLifecycle(StoneFireplaceConfig config)
    : config_(std::move(config)) {}

bool StoneFireplaceLifecycle::ConfigValid() const noexcept {
  return config_.ring.minStones >= 3 &&
         config_.ring.maxStones >= config_.ring.minStones &&
         std::isfinite(config_.ring.minRadius) && config_.ring.minRadius > 0.0 &&
         std::isfinite(config_.ring.maxRadius) &&
         config_.ring.maxRadius > config_.ring.minRadius &&
         std::isfinite(config_.ring.maxRadiusDeviation) &&
         config_.ring.maxRadiusDeviation >= 0.0 &&
         std::isfinite(config_.ring.maxHeightSpan) &&
         config_.ring.maxHeightSpan >= 0.0 &&
         std::isfinite(config_.ring.maxAngularGapDegrees) &&
         config_.ring.maxAngularGapDegrees > 0.0 &&
         config_.ring.maxAngularGapDegrees < 360.0 &&
         std::isfinite(config_.ring.maxCenterOccupancyRadius) &&
         config_.ring.maxCenterOccupancyRadius >= 0.0 &&
         std::isfinite(config_.maxFuelUnits) && config_.maxFuelUnits > 0.0 &&
         std::isfinite(config_.fuelUnitsPerSecond) &&
         config_.fuelUnitsPerSecond > 0.0 &&
         std::isfinite(config_.nominalHeatOutput) &&
         config_.nominalHeatOutput >= 0.0 &&
         std::isfinite(config_.maxAdvanceSeconds) &&
         config_.maxAdvanceSeconds > 0.0 &&
         config_.maxStoneSamples >= config_.ring.minStones;
}

Result<StoneRingAssessment> StoneFireplaceLifecycle::ReassessRing(
    const std::vector<LocalStoneSample>& localStones,
    double centerX, double centerY, double baseZ, Revision sourceRevision) {
  if (!ConfigValid() || !std::isfinite(centerX) || !std::isfinite(centerY) ||
      !std::isfinite(baseZ) || sourceRevision == 0) {
    return Result<StoneRingAssessment>::Failure(
        ErrorCode::InvalidArgument, "invalid fireplace config, center or ring source revision");
  }
  if (localStones.size() > config_.maxStoneSamples) {
    return Result<StoneRingAssessment>::Failure(
        ErrorCode::BudgetExceeded, "local stone sample budget exceeded");
  }
  if (sourceRevision <= ringSourceRevision_) {
    return Result<StoneRingAssessment>::Failure(
        ErrorCode::StaleRevision, "stone ring observation is not newer than the committed source revision");
  }

  StoneRingAssessment nextAssessment =
      LocalStoneRingRecognizer::Assess(localStones, centerX, centerY, baseZ, config_.ring);

  // Commit the observation only after all request-level validation has succeeded.
  ringAssessment_ = nextAssessment;
  ringSourceRevision_ = sourceRevision;
  ringRecognized_ = nextAssessment.recognized;
  if (!ringRecognized_) {
    state_ = StoneFireplaceState::NotBuilt;
  } else if (state_ == StoneFireplaceState::NotBuilt) {
    state_ = StoneFireplaceState::Ready;
  }
  ++stateRevision_;
  return Result<StoneRingAssessment>::Success(ringAssessment_);
}

Result<double> StoneFireplaceLifecycle::AddFuel(
    double amount, Revision expectedStateRevision) {
  if (!ConfigValid() || !std::isfinite(amount) || amount <= 0.0) {
    return Result<double>::Failure(ErrorCode::InvalidArgument, "fuel amount or fireplace config is invalid");
  }
  if (expectedStateRevision != stateRevision_) {
    return Result<double>::Failure(ErrorCode::StaleRevision, "fireplace changed after the fuel request was prepared");
  }
  if (!ringRecognized_ || state_ == StoneFireplaceState::NotBuilt) {
    return Result<double>::Failure(ErrorCode::MissingDependency, "a recognized stone ring is required before fueling");
  }

  const double accepted = std::min(amount, config_.maxFuelUnits - fuelUnits_);
  if (!std::isfinite(accepted) || accepted <= 0.0) {
    return Result<double>::Failure(ErrorCode::OutOfRange, "fireplace fuel store is already full");
  }
  fuelUnits_ += accepted;
  ++stateRevision_;
  return Result<double>::Success(accepted);
}

Result<bool> StoneFireplaceLifecycle::Ignite(Revision expectedStateRevision) {
  if (!ConfigValid()) {
    return Result<bool>::Failure(ErrorCode::InvalidArgument, "fireplace config is invalid");
  }
  if (expectedStateRevision != stateRevision_) {
    return Result<bool>::Failure(ErrorCode::StaleRevision, "fireplace changed before ignition");
  }
  if (!ringRecognized_ || state_ == StoneFireplaceState::NotBuilt) {
    return Result<bool>::Failure(ErrorCode::MissingDependency, "a valid stone ring is required for ignition");
  }
  if (state_ == StoneFireplaceState::Burning) {
    return Result<bool>::Success(false);
  }
  if (fuelUnits_ <= 0.0) {
    return Result<bool>::Failure(ErrorCode::MissingDependency, "fireplace has no fuel");
  }
  state_ = StoneFireplaceState::Burning;
  ++stateRevision_;
  return Result<bool>::Success(true);
}

Result<bool> StoneFireplaceLifecycle::Extinguish(Revision expectedStateRevision) {
  if (expectedStateRevision != stateRevision_) {
    return Result<bool>::Failure(ErrorCode::StaleRevision, "fireplace changed before extinguishing");
  }
  if (state_ != StoneFireplaceState::Burning) {
    return Result<bool>::Success(false);
  }
  state_ = ringRecognized_ ? StoneFireplaceState::Ready : StoneFireplaceState::NotBuilt;
  ++stateRevision_;
  return Result<bool>::Success(true);
}

Result<bool> StoneFireplaceLifecycle::PlaceCookingPot(Revision expectedStateRevision) {
  if (expectedStateRevision != stateRevision_) {
    return Result<bool>::Failure(ErrorCode::StaleRevision, "fireplace changed before pot placement");
  }
  if (!ringRecognized_ || state_ != StoneFireplaceState::Burning) {
    return Result<bool>::Failure(ErrorCode::MissingDependency, "a cooking pot requires a valid burning fireplace");
  }
  if (hasCookingPot_) {
    return Result<bool>::Success(false);
  }
  hasCookingPot_ = true;
  ++stateRevision_;
  return Result<bool>::Success(true);
}

Result<StoneFireplaceTick> StoneFireplaceLifecycle::Advance(
    double deltaSeconds, Revision expectedStateRevision) {
  if (!ConfigValid() || !std::isfinite(deltaSeconds) || deltaSeconds < 0.0) {
    return Result<StoneFireplaceTick>::Failure(ErrorCode::InvalidArgument, "invalid fireplace time step or config");
  }
  if (deltaSeconds > config_.maxAdvanceSeconds) {
    return Result<StoneFireplaceTick>::Failure(ErrorCode::BudgetExceeded, "fireplace time step exceeds the configured catch-up budget");
  }
  if (expectedStateRevision != stateRevision_) {
    return Result<StoneFireplaceTick>::Failure(ErrorCode::StaleRevision, "fireplace changed before its time step");
  }

  StoneFireplaceTick result;
  result.state = state_;
  result.fuelRemaining = fuelUnits_;
  result.heatOutput = CurrentHeatOutput();
  result.stateRevision = stateRevision_;
  if (state_ != StoneFireplaceState::Burning || deltaSeconds == 0.0) {
    return Result<StoneFireplaceTick>::Success(result);
  }

  const double requestedConsumption = deltaSeconds * config_.fuelUnitsPerSecond;
  if (!std::isfinite(requestedConsumption)) {
    return Result<StoneFireplaceTick>::Failure(ErrorCode::OutOfRange, "fuel consumption overflowed");
  }
  result.fuelConsumed = std::min(fuelUnits_, requestedConsumption);
  fuelUnits_ -= result.fuelConsumed;
  if (fuelUnits_ <= 1e-9) {
    fuelUnits_ = 0.0;
    state_ = ringRecognized_ ? StoneFireplaceState::Ready : StoneFireplaceState::NotBuilt;
    result.extinguishedForFuel = true;
  }
  ++stateRevision_;
  result.state = state_;
  result.fuelRemaining = fuelUnits_;
  result.heatOutput = CurrentHeatOutput();
  result.stateRevision = stateRevision_;
  return Result<StoneFireplaceTick>::Success(result);
}

StoneFireplaceSnapshot StoneFireplaceLifecycle::Snapshot() const {
  StoneFireplaceSnapshot snapshot;
  snapshot.stateRevision = stateRevision_;
  snapshot.ringSourceRevision = ringSourceRevision_;
  snapshot.state = state_;
  snapshot.ringRecognized = ringRecognized_;
  snapshot.hasCookingPot = hasCookingPot_;
  snapshot.fuelUnits = fuelUnits_;
  snapshot.heatOutput = CurrentHeatOutput();
  snapshot.ringAssessment = ringAssessment_;
  return snapshot;
}

double StoneFireplaceLifecycle::CurrentHeatOutput() const noexcept {
  if (state_ != StoneFireplaceState::Burning || !ringRecognized_) {
    return 0.0;
  }
  const double fuelFraction = std::clamp(fuelUnits_ / config_.maxFuelUnits, 0.0, 1.0);
  return config_.nominalHeatOutput * fuelFraction;
}

}  // namespace wm

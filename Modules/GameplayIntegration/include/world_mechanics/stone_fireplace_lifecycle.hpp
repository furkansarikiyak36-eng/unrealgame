#pragma once

#include "world_mechanics/core.hpp"
#include "world_mechanics/local_stone_ring_recognition.hpp"

#include <cstddef>

namespace wm {

enum class StoneFireplaceState {
  NotBuilt,
  Ready,
  Burning
};

struct StoneFireplaceConfig {
  StoneRingConfig ring{};
  double maxFuelUnits{100.0};
  double fuelUnitsPerSecond{2.0};
  double nominalHeatOutput{500.0};
  double maxAdvanceSeconds{10.0};
  std::size_t maxStoneSamples{128};
};

struct StoneFireplaceSnapshot {
  Revision stateRevision{1};
  Revision ringSourceRevision{0};
  StoneFireplaceState state{StoneFireplaceState::NotBuilt};
  bool ringRecognized{false};
  bool hasCookingPot{false};
  double fuelUnits{0.0};
  double heatOutput{0.0};
  StoneRingAssessment ringAssessment{};
};

struct StoneFireplaceTick {
  StoneFireplaceState state{StoneFireplaceState::NotBuilt};
  double fuelRemaining{0.0};
  double fuelConsumed{0.0};
  double heatOutput{0.0};
  bool extinguishedForFuel{false};
  Revision stateRevision{0};
};

// Engine-neutral authority for the stone-ring -> fireplace -> fuel -> heat loop.
// Unreal owns traces, actors, effects and inventory; this class owns the rules.
class StoneFireplaceLifecycle {
 public:
  explicit StoneFireplaceLifecycle(StoneFireplaceConfig config = {});

  Result<StoneRingAssessment> ReassessRing(
      const std::vector<LocalStoneSample>& localStones,
      double centerX, double centerY, double baseZ,
      Revision sourceRevision);

  Result<double> AddFuel(double amount, Revision expectedStateRevision);
  Result<bool> Ignite(Revision expectedStateRevision);
  Result<bool> Extinguish(Revision expectedStateRevision);
  Result<bool> PlaceCookingPot(Revision expectedStateRevision);
  Result<StoneFireplaceTick> Advance(double deltaSeconds,
                                     Revision expectedStateRevision);

  StoneFireplaceSnapshot Snapshot() const;
  Revision GetRevision() const noexcept { return stateRevision_; }

 private:
  bool ConfigValid() const noexcept;
  double CurrentHeatOutput() const noexcept;

  StoneFireplaceConfig config_;
  Revision stateRevision_{1};
  Revision ringSourceRevision_{0};
  StoneFireplaceState state_{StoneFireplaceState::NotBuilt};
  bool ringRecognized_{false};
  bool hasCookingPot_{false};
  double fuelUnits_{0.0};
  StoneRingAssessment ringAssessment_{};
};

}  // namespace wm

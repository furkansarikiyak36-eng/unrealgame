#pragma once
#include "world_mechanics/world_environment.hpp"

namespace wm {
enum class EffortKind { ToolSwing, HeavyExcavation, ShoringPlacement, ShoringCarry, ShoringRepair, HeavyCarry };
struct EffortCosts {
  double toolSwing{1.0};
  double heavyExcavation{7.0};
  double shoringPlacement{3.0};
  double shoringCarry{4.0};
  double shoringRepair{2.0};
  double heavyCarry{5.0};
};
struct ExposureState {
  double thermalStress{0};
  double wetness{0};
  double darknessExposure{0};
  double waterExposure{0};
  double lastUpdateSeconds{0};
};
class SurvivalExposure {
 public:
  explicit SurvivalExposure(EffortCosts costs={}) : costs_(costs) {}
  Result<double> EffortCost(EffortKind kind,double intensity=1.0) const;
  Result<ExposureState> Update(const ExposureState& current,const EnvironmentSample& sample,
                               double deltaSeconds) const;
 private:
  EffortCosts costs_;
  const double maxStepSeconds_{0.25};
};
} // namespace wm

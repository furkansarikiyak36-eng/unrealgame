#pragma once

#include "world_mechanics/world_environment.hpp"

#include <cstddef>
#include <map>
#include <vector>

namespace wm {

struct EnvironmentCellState {
  Id cellId{0};
  Vec3 positionM{};
  double terrainElevationM{0.0};
  double areaM2{1.0};

  double surfaceWaterM3{0.0};
  double surfaceWaterCapacityM3{1.0};
  double soilWaterM3{0.0};
  double soilWaterCapacityM3{1.0};

  double nitrogenKg{0.0};
  double phosphorusKg{0.0};
  double potassiumKg{0.0};

  double temperatureC{20.0};
  double fuelKg{0.0};
  double fuelMoisture01{0.0};
  bool burning{false};
  double burningAgeSeconds{0.0};
};

struct EnvironmentConnection {
  Id a{0};
  Id b{0};
  // Volume transfer = head difference (m) * conductance (m^2/s) * dt (s).
  double hydraulicConductanceM2PerSecond{0.0};
  double thermalConductancePerSecond{0.0};
};

struct EnvironmentSimulationConfig {
  double fixedStepSeconds{0.1};
  std::uint32_t maxSubstepsPerCall{8};
  double maxQueuedSeconds{30.0};
  std::size_t maxCells{4096};
  std::size_t maxConnections{16384};

  double maxWaterTransferFractionPerStep{0.25};
  double infiltrationRateMPerSecond{0.0005};
  double evaporationRateMPerSecond{0.00001};
  double deepDrainageRateMPerSecond{0.00001};
  double nutrientLeachFractionPerSecond{0.0001};

  double temperatureRelaxationPerSecond{0.08};
  double fireHeatCPerSecond{80.0};
  double fireBurnRateKgPerSecond{0.15};
  double autoIgnitionTemperatureC{300.0};
  double maximumFuelMoisture01{0.65};
  double minimumBurnableFuelKg{0.02};
  double windHeatBoost{1.0};
};

struct WaterInputResult {
  double requestedM3{0.0};
  double acceptedM3{0.0};
  double runoffM3{0.0};
};

struct EnvironmentStepReport {
  std::uint32_t completedSubsteps{0};
  std::uint32_t newIgnitions{0};
  double simulatedSeconds{0.0};
  double pendingTimeSeconds{0.0};

  double waterBeforeM3{0.0};
  double waterAfterM3{0.0};
  double waterBalanceErrorM3{0.0};

  double evaporationThisCallM3{0.0};
  double drainageThisCallM3{0.0};
  double fuelBurnedThisCallKg{0.0};
};

// Deterministic, bounded environment simulation for a connected active region.
// Surface water and soil water are separate stores; mesh/density remains authoritative
// in TerrainVoxel. All flow decisions use a stable cell/link order.
class WorldEnvironmentSimulation {
 public:
  explicit WorldEnvironmentSimulation(EnvironmentSimulationConfig config = {});

  Result<bool> AddCell(const EnvironmentCellState& cell);
  Result<bool> Connect(EnvironmentConnection connection);
  Result<WaterInputResult> AddSurfaceWater(Id cellId, double requestedM3);
  Result<bool> Ignite(Id cellId);
  Result<bool> Extinguish(Id cellId);

  // Fixed-step backlog is retained when maxSubstepsPerCall is reached. A call that
  // would exceed maxQueuedSeconds is rejected without adding time or mutating state.
  Result<EnvironmentStepReport> Step(
      double deltaSeconds,
      double ambientTemperatureC,
      Vec3 windDirection = {});

  Result<EnvironmentCellState> GetCell(Id cellId) const;
  Result<EnvironmentSample> Sample(Id cellId, double sampledAtSeconds) const;

  std::size_t CellCount() const noexcept { return cells_.size(); }
  std::size_t ConnectionCount() const noexcept { return connections_.size(); }
  Revision GetRevision() const noexcept { return sourceRevision_; }
  double GetPendingTimeSeconds() const noexcept { return pendingSeconds_; }
  double GetElapsedSimulationSeconds() const noexcept { return elapsedSeconds_; }
  double TotalWaterM3() const noexcept;
  double WaterBalanceErrorM3() const noexcept;

 private:
  bool ValidateConfig(std::string* reason = nullptr) const;
  bool ValidateCell(const EnvironmentCellState& cell, std::string* reason = nullptr) const;
  Result<std::uint32_t> AdvanceFixedStep(double ambientTemperatureC, const Vec3& normalizedWind);
  bool CanIgnite(const EnvironmentCellState& cell) const noexcept;
  double EffectiveFuelMoisture01(const EnvironmentCellState& cell) const noexcept;

  EnvironmentSimulationConfig config_;
  std::map<Id, EnvironmentCellState> cells_;
  std::vector<EnvironmentConnection> connections_;

  Revision sourceRevision_{1};
  double pendingSeconds_{0.0};
  double elapsedSeconds_{0.0};

  double initialWaterM3_{0.0};
  double waterInputM3_{0.0};
  double evaporatedWaterM3_{0.0};
  double runoffWaterM3_{0.0};
  double deepDrainageWaterM3_{0.0};
  double burnedFuelKg_{0.0};
  double leachedNitrogenKg_{0.0};
  double leachedPhosphorusKg_{0.0};
  double leachedPotassiumKg_{0.0};
};

}  // namespace wm

#include "world_mechanics/world_environment_simulation.hpp"

#include <algorithm>
#include <cmath>
#include <tuple>
#include <utility>

namespace wm {
namespace {
constexpr double kEpsilon = 1e-9;
constexpr double kMinimumTemperatureC = -273.15;
constexpr double kMaximumTemperatureC = 3000.0;
constexpr double kMaximumCellWaterM3 = 1.0e9;
constexpr double kMaximumFuelKg = 1.0e9;

bool IsFiniteNonNegative(double value) {
  return std::isfinite(value) && value >= 0.0;
}

bool IsFiniteInRange(double value, double low, double high) {
  return std::isfinite(value) && value >= low && value <= high;
}
}  // namespace

WorldEnvironmentSimulation::WorldEnvironmentSimulation(EnvironmentSimulationConfig config)
    : config_(config) {}

bool WorldEnvironmentSimulation::ValidateConfig(std::string* reason) const {
  auto fail = [&](const char* message) {
    if (reason) *reason = message;
    return false;
  };
  if (!IsFiniteInRange(config_.fixedStepSeconds, 1e-4, 1.0))
    return fail("fixedStepSeconds must be in [0.0001, 1]");
  if (config_.maxSubstepsPerCall == 0 || config_.maxSubstepsPerCall > 256)
    return fail("maxSubstepsPerCall must be in [1, 256]");
  if (!IsFiniteInRange(config_.maxQueuedSeconds, config_.fixedStepSeconds, 3600.0))
    return fail("maxQueuedSeconds must cover one step and not exceed one hour");
  if (config_.maxCells == 0 || config_.maxCells > 65536)
    return fail("maxCells must be in [1, 65536]");
  if (config_.maxConnections == 0 || config_.maxConnections > 262144)
    return fail("maxConnections must be in [1, 262144]");
  if (!IsFiniteInRange(config_.maxWaterTransferFractionPerStep, 1e-6, 0.5))
    return fail("maxWaterTransferFractionPerStep must be in (0, 0.5]");
  if (!IsFiniteInRange(config_.infiltrationRateMPerSecond, 0.0, 10.0) ||
      !IsFiniteInRange(config_.evaporationRateMPerSecond, 0.0, 10.0) ||
      !IsFiniteInRange(config_.deepDrainageRateMPerSecond, 0.0, 10.0))
    return fail("water rates must be finite and in [0, 10] m/s");
  if (!IsFiniteInRange(config_.nutrientLeachFractionPerSecond, 0.0, 10.0))
    return fail("nutrientLeachFractionPerSecond must be in [0, 10]");
  if (!IsFiniteInRange(config_.temperatureRelaxationPerSecond, 0.0, 5.0))
    return fail("temperatureRelaxationPerSecond must be in [0, 5]");
  if (!IsFiniteInRange(config_.fireHeatCPerSecond, 0.0, 2000.0) ||
      !IsFiniteInRange(config_.fireBurnRateKgPerSecond, 0.0, 1.0e6))
    return fail("fire rate settings are outside safe bounds");
  if (!IsFiniteInRange(config_.autoIgnitionTemperatureC, 0.0, 2000.0))
    return fail("autoIgnitionTemperatureC must be in [0, 2000]");
  if (!IsFiniteInRange(config_.maximumFuelMoisture01, 0.0, 1.0))
    return fail("maximumFuelMoisture01 must be in [0, 1]");
  if (!IsFiniteInRange(config_.minimumBurnableFuelKg, 1e-9, 1.0e6))
    return fail("minimumBurnableFuelKg must be positive and bounded");
  if (!IsFiniteInRange(config_.windHeatBoost, 0.0, 5.0))
    return fail("windHeatBoost must be in [0, 5]");
  return true;
}

bool WorldEnvironmentSimulation::ValidateCell(
    const EnvironmentCellState& cell, std::string* reason) const {
  auto fail = [&](const char* message) {
    if (reason) *reason = message;
    return false;
  };
  if (cell.cellId == 0) return fail("cellId must be nonzero");
  if (!cell.positionM.IsFinite()) return fail("cell position must be finite");
  if (!IsFiniteInRange(cell.positionM.x, -1.0e9, 1.0e9) ||
      !IsFiniteInRange(cell.positionM.y, -1.0e9, 1.0e9) ||
      !IsFiniteInRange(cell.positionM.z, -1.0e9, 1.0e9))
    return fail("cell position is outside safe bounds");
  if (!IsFiniteInRange(cell.terrainElevationM, -1.0e6, 1.0e6))
    return fail("terrain elevation is outside safe bounds");
  if (!IsFiniteInRange(cell.areaM2, 0.01, 1.0e6))
    return fail("cell area must be in [0.01, 1000000] m^2");
  if (!IsFiniteInRange(cell.surfaceWaterCapacityM3, 0.0, kMaximumCellWaterM3) ||
      !IsFiniteInRange(cell.soilWaterCapacityM3, 0.0, kMaximumCellWaterM3))
    return fail("water capacities must be finite and bounded");
  if (!IsFiniteInRange(cell.surfaceWaterM3, 0.0, cell.surfaceWaterCapacityM3) ||
      !IsFiniteInRange(cell.soilWaterM3, 0.0, cell.soilWaterCapacityM3))
    return fail("initial water must fit its corresponding capacity");
  if (!IsFiniteNonNegative(cell.nitrogenKg) ||
      !IsFiniteNonNegative(cell.phosphorusKg) ||
      !IsFiniteNonNegative(cell.potassiumKg))
    return fail("soil nutrients must be finite and nonnegative");
  if (!IsFiniteInRange(cell.temperatureC, kMinimumTemperatureC, kMaximumTemperatureC))
    return fail("temperature is outside supported bounds");
  if (!IsFiniteInRange(cell.fuelKg, 0.0, kMaximumFuelKg))
    return fail("fuelKg must be finite and bounded");
  if (!IsFiniteInRange(cell.fuelMoisture01, 0.0, 1.0))
    return fail("fuelMoisture01 must be in [0, 1]");
  if (!IsFiniteNonNegative(cell.burningAgeSeconds))
    return fail("burningAgeSeconds must be finite and nonnegative");
  if (cell.burning && (cell.fuelKg < config_.minimumBurnableFuelKg ||
                       EffectiveFuelMoisture01(cell) > config_.maximumFuelMoisture01))
    return fail("burning cell must have sufficient dry fuel");
  return true;
}

double WorldEnvironmentSimulation::EffectiveFuelMoisture01(
    const EnvironmentCellState& cell) const noexcept {
  const double soilSaturation = cell.soilWaterCapacityM3 > kEpsilon
      ? Clamp01(cell.soilWaterM3 / cell.soilWaterCapacityM3)
      : 0.0;
  return std::max(cell.fuelMoisture01, soilSaturation * 0.75);
}

bool WorldEnvironmentSimulation::CanIgnite(
    const EnvironmentCellState& cell) const noexcept {
  return cell.fuelKg >= config_.minimumBurnableFuelKg &&
         EffectiveFuelMoisture01(cell) <= config_.maximumFuelMoisture01;
}

Result<bool> WorldEnvironmentSimulation::AddCell(const EnvironmentCellState& cell) {
  std::string reason;
  if (!ValidateConfig(&reason))
    return Result<bool>::Failure(ErrorCode::InvalidArgument, reason);
  if (!ValidateCell(cell, &reason))
    return Result<bool>::Failure(ErrorCode::InvalidArgument, reason);
  if (cells_.size() >= config_.maxCells)
    return Result<bool>::Failure(ErrorCode::BudgetExceeded, "environment cell budget exceeded");
  if (cells_.find(cell.cellId) != cells_.end())
    return Result<bool>::Failure(ErrorCode::Conflict, "environment cell ID already exists");

  const double initialWater = cell.surfaceWaterM3 + cell.soilWaterM3;
  if (!std::isfinite(initialWaterM3_ + initialWater))
    return Result<bool>::Failure(ErrorCode::OutOfRange, "initial water total overflowed");

  cells_.emplace(cell.cellId, cell);
  initialWaterM3_ += initialWater;
  ++sourceRevision_;
  return Result<bool>::Success(true);
}

Result<bool> WorldEnvironmentSimulation::Connect(EnvironmentConnection connection) {
  std::string reason;
  if (!ValidateConfig(&reason))
    return Result<bool>::Failure(ErrorCode::InvalidArgument, reason);
  if (connection.a == 0 || connection.b == 0 || connection.a == connection.b)
    return Result<bool>::Failure(ErrorCode::InvalidArgument, "connection requires two distinct nonzero cell IDs");
  if (cells_.find(connection.a) == cells_.end() || cells_.find(connection.b) == cells_.end())
    return Result<bool>::Failure(ErrorCode::NotFound, "connection endpoint does not exist");
  if (!IsFiniteInRange(connection.hydraulicConductanceM2PerSecond, 0.0, 1000.0) ||
      !IsFiniteInRange(connection.thermalConductancePerSecond, 0.0, 1.0))
    return Result<bool>::Failure(ErrorCode::InvalidArgument, "connection conductance is outside safe bounds");
  if (connections_.size() >= config_.maxConnections)
    return Result<bool>::Failure(ErrorCode::BudgetExceeded, "environment connection budget exceeded");

  if (connection.b < connection.a) std::swap(connection.a, connection.b);
  const auto existing = std::find_if(
      connections_.begin(), connections_.end(), [&](const EnvironmentConnection& item) {
        return item.a == connection.a && item.b == connection.b;
      });
  if (existing != connections_.end())
    return Result<bool>::Failure(ErrorCode::Conflict, "connection already exists");

  connections_.push_back(connection);
  std::sort(connections_.begin(), connections_.end(), [](const auto& lhs, const auto& rhs) {
    return std::tie(lhs.a, lhs.b) < std::tie(rhs.a, rhs.b);
  });
  ++sourceRevision_;
  return Result<bool>::Success(true);
}

Result<WaterInputResult> WorldEnvironmentSimulation::AddSurfaceWater(
    Id cellId, double requestedM3) {
  if (!std::isfinite(requestedM3) || requestedM3 < 0.0 || requestedM3 > kMaximumCellWaterM3)
    return Result<WaterInputResult>::Failure(ErrorCode::InvalidArgument, "requested water must be finite and bounded");
  auto found = cells_.find(cellId);
  if (found == cells_.end())
    return Result<WaterInputResult>::Failure(ErrorCode::NotFound, "water target cell does not exist");
  if (requestedM3 == 0.0)
    return Result<WaterInputResult>::Success({0.0, 0.0, 0.0});
  if (!std::isfinite(waterInputM3_ + requestedM3))
    return Result<WaterInputResult>::Failure(ErrorCode::OutOfRange, "cumulative water input overflowed");

  const double room = std::max(0.0, found->second.surfaceWaterCapacityM3 - found->second.surfaceWaterM3);
  const double accepted = std::min(room, requestedM3);
  const double runoff = requestedM3 - accepted;
  if (!std::isfinite(runoffWaterM3_ + runoff))
    return Result<WaterInputResult>::Failure(ErrorCode::OutOfRange, "cumulative runoff overflowed");

  found->second.surfaceWaterM3 += accepted;
  waterInputM3_ += requestedM3;
  runoffWaterM3_ += runoff;
  ++sourceRevision_;
  return Result<WaterInputResult>::Success({requestedM3, accepted, runoff});
}

Result<bool> WorldEnvironmentSimulation::Ignite(Id cellId) {
  auto found = cells_.find(cellId);
  if (found == cells_.end())
    return Result<bool>::Failure(ErrorCode::NotFound, "ignition target cell does not exist");
  if (found->second.burning)
    return Result<bool>::Failure(ErrorCode::Conflict, "cell is already burning");
  if (!CanIgnite(found->second))
    return Result<bool>::Failure(ErrorCode::Unsupported, "fuel is absent, exhausted or too wet to ignite");

  found->second.burning = true;
  found->second.burningAgeSeconds = 0.0;
  found->second.temperatureC = std::max(
      found->second.temperatureC, config_.autoIgnitionTemperatureC + 1.0);
  ++sourceRevision_;
  return Result<bool>::Success(true);
}

Result<bool> WorldEnvironmentSimulation::Extinguish(Id cellId) {
  auto found = cells_.find(cellId);
  if (found == cells_.end())
    return Result<bool>::Failure(ErrorCode::NotFound, "extinguish target cell does not exist");
  if (!found->second.burning)
    return Result<bool>::Failure(ErrorCode::Conflict, "cell is not burning");
  found->second.burning = false;
  ++sourceRevision_;
  return Result<bool>::Success(true);
}

Result<EnvironmentStepReport> WorldEnvironmentSimulation::Step(
    double deltaSeconds, double ambientTemperatureC, Vec3 windDirection) {
  std::string reason;
  if (!ValidateConfig(&reason))
    return Result<EnvironmentStepReport>::Failure(ErrorCode::InvalidArgument, reason);
  if (!std::isfinite(deltaSeconds) || deltaSeconds < 0.0)
    return Result<EnvironmentStepReport>::Failure(ErrorCode::InvalidArgument, "deltaSeconds must be finite and nonnegative");
  if (!IsFiniteInRange(ambientTemperatureC, kMinimumTemperatureC, kMaximumTemperatureC))
    return Result<EnvironmentStepReport>::Failure(ErrorCode::InvalidArgument, "ambient temperature is outside supported bounds");
  if (!windDirection.IsFinite())
    return Result<EnvironmentStepReport>::Failure(ErrorCode::InvalidArgument, "wind direction must be finite");

  const double queued = pendingSeconds_ + deltaSeconds;
  if (!std::isfinite(queued) || queued > config_.maxQueuedSeconds + kEpsilon)
    return Result<EnvironmentStepReport>::Failure(
        ErrorCode::BudgetExceeded,
        "time backlog would exceed maxQueuedSeconds; split the input delta and retry");

  const double evaporationBefore = evaporatedWaterM3_;
  const double drainageBefore = deepDrainageWaterM3_;
  const double burnedBefore = burnedFuelKg_;
  pendingSeconds_ = queued;

  EnvironmentStepReport report;
  report.waterBeforeM3 = TotalWaterM3();
  const Vec3 normalizedWind = LengthSquared(windDirection) > kEpsilon
      ? Normalize(windDirection)
      : Vec3{};

  while (pendingSeconds_ + kEpsilon >= config_.fixedStepSeconds &&
         report.completedSubsteps < config_.maxSubstepsPerCall) {
    auto advanced = AdvanceFixedStep(ambientTemperatureC, normalizedWind);
    if (!advanced)
      return Result<EnvironmentStepReport>::Failure(advanced.error, advanced.message);
    pendingSeconds_ = std::max(0.0, pendingSeconds_ - config_.fixedStepSeconds);
    report.completedSubsteps++;
    report.newIgnitions += advanced.value;
  }

  report.simulatedSeconds = report.completedSubsteps * config_.fixedStepSeconds;
  report.pendingTimeSeconds = pendingSeconds_;
  report.waterAfterM3 = TotalWaterM3();
  report.waterBalanceErrorM3 = WaterBalanceErrorM3();
  report.evaporationThisCallM3 = evaporatedWaterM3_ - evaporationBefore;
  report.drainageThisCallM3 = deepDrainageWaterM3_ - drainageBefore;
  report.fuelBurnedThisCallKg = burnedFuelKg_ - burnedBefore;
  return Result<EnvironmentStepReport>::Success(report);
}

Result<std::uint32_t> WorldEnvironmentSimulation::AdvanceFixedStep(
    double ambientTemperatureC, const Vec3& normalizedWind) {
  const double dt = config_.fixedStepSeconds;
  auto next = cells_;  // Stage the bounded active region; publish once per fixed step.

  struct WaterTransfer {
    Id from;
    Id to;
    double requestedM3;
    double sourceLimitedM3{0.0};
  };
  std::vector<WaterTransfer> transfers;
  transfers.reserve(connections_.size());
  std::map<Id, double> requestedOutBySource;
  std::map<Id, double> sourceScale;
  std::map<Id, double> requestedInByTarget;

  for (const auto& connection : connections_) {
    const auto& a = cells_.at(connection.a);
    const auto& b = cells_.at(connection.b);
    if (connection.hydraulicConductanceM2PerSecond <= 0.0) continue;

    const double levelA = a.terrainElevationM + a.surfaceWaterM3 / a.areaM2;
    const double levelB = b.terrainElevationM + b.surfaceWaterM3 / b.areaM2;
    const double headDifference = levelA - levelB;
    if (std::abs(headDifference) <= kEpsilon) continue;

    const Id from = headDifference > 0.0 ? connection.a : connection.b;
    const Id to = headDifference > 0.0 ? connection.b : connection.a;
    const double proposed = std::abs(headDifference) *
        connection.hydraulicConductanceM2PerSecond * dt;
    if (!IsFiniteNonNegative(proposed))
      return Result<std::uint32_t>::Failure(ErrorCode::OutOfRange, "water transfer proposal is non-finite");
    if (proposed <= kEpsilon) continue;

    transfers.push_back({from, to, proposed, 0.0});
    requestedOutBySource[from] += proposed;
  }

  for (const auto& entry : requestedOutBySource) {
    const double volume = cells_.at(entry.first).surfaceWaterM3;
    const double permitted = volume * config_.maxWaterTransferFractionPerStep;
    sourceScale[entry.first] = entry.second > kEpsilon
        ? std::min(1.0, permitted / entry.second)
        : 0.0;
  }

  for (auto& transfer : transfers) {
    transfer.sourceLimitedM3 = transfer.requestedM3 * sourceScale[transfer.from];
    requestedInByTarget[transfer.to] += transfer.sourceLimitedM3;
  }

  std::map<Id, double> targetScale;
  for (const auto& entry : requestedInByTarget) {
    const auto& target = cells_.at(entry.first);
    const double room = std::max(0.0, target.surfaceWaterCapacityM3 - target.surfaceWaterM3);
    targetScale[entry.first] = entry.second > kEpsilon
        ? std::min(1.0, room / entry.second)
        : 0.0;
  }

  std::map<Id, double> waterDelta;
  for (const auto& transfer : transfers) {
    const double amount = transfer.sourceLimitedM3 * targetScale[transfer.to];
    waterDelta[transfer.from] -= amount;
    waterDelta[transfer.to] += amount;
  }

  for (auto& entry : next) {
    const double base = cells_.at(entry.first).surfaceWaterM3;
    entry.second.surfaceWaterM3 = base + waterDelta[entry.first];
    if (entry.second.surfaceWaterM3 < 0.0 && entry.second.surfaceWaterM3 > -1e-8)
      entry.second.surfaceWaterM3 = 0.0;
    if (!std::isfinite(entry.second.surfaceWaterM3) ||
        entry.second.surfaceWaterM3 < 0.0 ||
        entry.second.surfaceWaterM3 > entry.second.surfaceWaterCapacityM3 + 1e-7)
      return Result<std::uint32_t>::Failure(ErrorCode::CorruptData, "water flow violated cell volume bounds");
  }

  double evaporationThisStep = 0.0;
  double drainageThisStep = 0.0;
  double leachedNitrogenThisStep = 0.0;
  double leachedPhosphorusThisStep = 0.0;
  double leachedPotassiumThisStep = 0.0;
  for (auto& entry : next) {
    auto& cell = entry.second;

    const double soilRoom = std::max(0.0, cell.soilWaterCapacityM3 - cell.soilWaterM3);
    const double infiltration = std::min(
        cell.surfaceWaterM3,
        std::min(soilRoom, config_.infiltrationRateMPerSecond * cell.areaM2 * dt));
    cell.surfaceWaterM3 -= infiltration;
    cell.soilWaterM3 += infiltration;

    const double drainage = std::min(
        cell.soilWaterM3,
        config_.deepDrainageRateMPerSecond * cell.areaM2 * dt);
    cell.soilWaterM3 -= drainage;
    drainageThisStep += drainage;

    const double temperatureFactor = std::clamp(
        1.0 + std::max(0.0, cell.temperatureC - 20.0) * 0.01, 1.0, 5.0);
    double evaporationDemand = config_.evaporationRateMPerSecond *
        cell.areaM2 * dt * temperatureFactor;
    const double surfaceEvaporation = std::min(cell.surfaceWaterM3, evaporationDemand);
    cell.surfaceWaterM3 -= surfaceEvaporation;
    evaporationDemand -= surfaceEvaporation;
    const double soilEvaporation = std::min(cell.soilWaterM3, evaporationDemand);
    cell.soilWaterM3 -= soilEvaporation;
    evaporationThisStep += surfaceEvaporation + soilEvaporation;

    const double soilSaturation = cell.soilWaterCapacityM3 > kEpsilon
        ? Clamp01(cell.soilWaterM3 / cell.soilWaterCapacityM3)
        : 0.0;
    const double leachFraction = Clamp01(
        config_.nutrientLeachFractionPerSecond * dt * soilSaturation);
    const double leachedN = cell.nitrogenKg * leachFraction;
    const double leachedP = cell.phosphorusKg * leachFraction;
    const double leachedK = cell.potassiumKg * leachFraction;
    cell.nitrogenKg -= leachedN;
    cell.phosphorusKg -= leachedP;
    cell.potassiumKg -= leachedK;
    leachedNitrogenThisStep += leachedN;
    leachedPhosphorusThisStep += leachedP;
    leachedPotassiumThisStep += leachedK;
  }

  std::map<Id, double> temperatureBefore;
  std::map<Id, bool> wasBurning;
  std::map<Id, double> incomingHeat;
  double fuelBurnedThisStep = 0.0;
  const double relaxationAlpha = Clamp01(config_.temperatureRelaxationPerSecond * dt);

  for (auto& entry : next) {
    auto& cell = entry.second;
    temperatureBefore[entry.first] = cell.temperatureC;
    wasBurning[entry.first] = cell.burning;
    cell.temperatureC += (ambientTemperatureC - cell.temperatureC) * relaxationAlpha;

    if (cell.burning) {
      const double burned = std::min(cell.fuelKg, config_.fireBurnRateKgPerSecond * dt);
      cell.fuelKg = std::max(0.0, cell.fuelKg - burned);
      fuelBurnedThisStep += burned;
      cell.temperatureC += config_.fireHeatCPerSecond * dt;
      cell.burningAgeSeconds += dt;
      if (cell.fuelKg < config_.minimumBurnableFuelKg)
        cell.burning = false;
    }
  }

  struct HeatTransfer {
    Id from;
    Id to;
    double requestedC;
  };
  std::vector<HeatTransfer> heatTransfers;
  heatTransfers.reserve(connections_.size() * 2);
  std::map<Id, double> requestedHeatOutBySource;
  auto requestHeat = [&](Id from, Id to, double conductance) {
    if (!wasBurning[from] || conductance <= 0.0) return;
    const double sourceTemperature = next.at(from).temperatureC;
    const double targetTemperature = next.at(to).temperatureC;
    const double difference = sourceTemperature - targetTemperature;
    if (difference <= kEpsilon) return;

    const Vec3 direction = Normalize(next.at(to).positionM - next.at(from).positionM);
    const double windAlong = Clamp01(Dot(normalizedWind, direction));
    const double windFactor = 1.0 + config_.windHeatBoost * windAlong;
    const double transfer = std::min(
        difference * 0.25,
        difference * conductance * dt * windFactor);
    if (transfer <= kEpsilon || !std::isfinite(transfer)) return;
    heatTransfers.push_back({from, to, transfer});
    requestedHeatOutBySource[from] += transfer;
  };

  for (const auto& connection : connections_) {
    requestHeat(connection.a, connection.b, connection.thermalConductancePerSecond);
    requestHeat(connection.b, connection.a, connection.thermalConductancePerSecond);
  }

  std::map<Id, double> heatScale;
  for (const auto& entry : requestedHeatOutBySource) {
    const double thermalExcess = std::max(
        0.0, next.at(entry.first).temperatureC - ambientTemperatureC);
    const double permitted = thermalExcess * 0.25;
    heatScale[entry.first] = entry.second > kEpsilon
        ? std::min(1.0, permitted / entry.second)
        : 0.0;
  }

  std::map<Id, double> heatDelta;
  for (const auto& transfer : heatTransfers) {
    const double amount = transfer.requestedC * heatScale[transfer.from];
    heatDelta[transfer.from] -= amount;
    heatDelta[transfer.to] += amount;
    incomingHeat[transfer.to] += amount;
  }

  std::uint32_t newIgnitions = 0;
  for (auto& entry : next) {
    auto& cell = entry.second;
    const double newTemperature = cell.temperatureC + heatDelta[entry.first];
    if (!std::isfinite(newTemperature))
      return Result<std::uint32_t>::Failure(ErrorCode::OutOfRange, "heat integration overflowed");
    cell.temperatureC = std::clamp(newTemperature, kMinimumTemperatureC, kMaximumTemperatureC);

    if (!wasBurning[entry.first] &&
        temperatureBefore[entry.first] < config_.autoIgnitionTemperatureC &&
        cell.temperatureC >= config_.autoIgnitionTemperatureC &&
        incomingHeat[entry.first] > kEpsilon &&
        CanIgnite(cell)) {
      cell.burning = true;
      cell.burningAgeSeconds = 0.0;
      ++newIgnitions;
    }
    std::string reason;
    if (!ValidateCell(cell, &reason))
      return Result<std::uint32_t>::Failure(
          ErrorCode::CorruptData, "staged environment state invalid: " + reason);
  }

  const double stagedWater = [&]() {
    double total = 0.0;
    for (const auto& entry : next)
      total += entry.second.surfaceWaterM3 + entry.second.soilWaterM3;
    return total;
  }();
  if (!std::isfinite(stagedWater) ||
      !std::isfinite(evaporatedWaterM3_ + evaporationThisStep) ||
      !std::isfinite(deepDrainageWaterM3_ + drainageThisStep) ||
      !std::isfinite(burnedFuelKg_ + fuelBurnedThisStep) ||
      !std::isfinite(leachedNitrogenKg_ + leachedNitrogenThisStep) ||
      !std::isfinite(leachedPhosphorusKg_ + leachedPhosphorusThisStep) ||
      !std::isfinite(leachedPotassiumKg_ + leachedPotassiumThisStep))
    return Result<std::uint32_t>::Failure(ErrorCode::OutOfRange, "environment state totals overflowed");

  cells_ = std::move(next);
  evaporatedWaterM3_ += evaporationThisStep;
  deepDrainageWaterM3_ += drainageThisStep;
  burnedFuelKg_ += fuelBurnedThisStep;
  leachedNitrogenKg_ += leachedNitrogenThisStep;
  leachedPhosphorusKg_ += leachedPhosphorusThisStep;
  leachedPotassiumKg_ += leachedPotassiumThisStep;
  elapsedSeconds_ += dt;
  ++sourceRevision_;
  return Result<std::uint32_t>::Success(newIgnitions);
}

Result<EnvironmentCellState> WorldEnvironmentSimulation::GetCell(Id cellId) const {
  const auto found = cells_.find(cellId);
  if (found == cells_.end())
    return Result<EnvironmentCellState>::Failure(ErrorCode::NotFound, "environment cell does not exist");
  return Result<EnvironmentCellState>::Success(found->second);
}

Result<EnvironmentSample> WorldEnvironmentSimulation::Sample(
    Id cellId, double sampledAtSeconds) const {
  if (!std::isfinite(sampledAtSeconds) || sampledAtSeconds < 0.0)
    return Result<EnvironmentSample>::Failure(ErrorCode::InvalidArgument, "sample time must be finite and nonnegative");
  const auto found = cells_.find(cellId);
  if (found == cells_.end())
    return Result<EnvironmentSample>::Failure(ErrorCode::NotFound, "environment sample cell does not exist");

  const auto& cell = found->second;
  EnvironmentSample sample;
  sample.sourceRevision = sourceRevision_;
  sample.sampledAtSeconds = sampledAtSeconds;
  sample.valid = true;
  sample.stale = false;
  sample.temperatureC = cell.temperatureC;
  sample.wetness01 = cell.soilWaterCapacityM3 > kEpsilon
      ? Clamp01(cell.soilWaterM3 / cell.soilWaterCapacityM3)
      : 0.0;
  sample.waterDepthM = cell.surfaceWaterM3 / cell.areaM2;
  std::string reason;
  if (!sample.Validate(&reason))
    return Result<EnvironmentSample>::Failure(ErrorCode::CorruptData, reason);
  return Result<EnvironmentSample>::Success(sample);
}

double WorldEnvironmentSimulation::TotalWaterM3() const noexcept {
  double total = 0.0;
  for (const auto& entry : cells_)
    total += entry.second.surfaceWaterM3 + entry.second.soilWaterM3;
  return total;
}

double WorldEnvironmentSimulation::WaterBalanceErrorM3() const noexcept {
  return initialWaterM3_ + waterInputM3_ - TotalWaterM3() -
      evaporatedWaterM3_ - runoffWaterM3_ - deepDrainageWaterM3_;
}

}  // namespace wm

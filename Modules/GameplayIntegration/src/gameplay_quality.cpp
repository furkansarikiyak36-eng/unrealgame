#include "world_mechanics/gameplay_quality.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <set>
#include <tuple>

namespace wm {
namespace {
constexpr double kEpsilon = 1e-9;
constexpr double kMaxSafeDays = 30.0;
constexpr double kDaysPerGrowthStep = 0.25;
constexpr std::size_t kHardCandidateLimit = 4096;
constexpr std::size_t kHardSchedulerQueueLimit = 65536;
constexpr std::size_t kHardMaterialRuleLimit = 65536;
constexpr std::size_t kHardInventorySlotsLimit = 4096;
constexpr std::size_t kHardRecipeEntriesLimit = 1024;

bool FiniteRange(double value, double low, double high) {
  return std::isfinite(value) && value >= low && value <= high;
}

bool NonNegativeFinite(double value) {
  return std::isfinite(value) && value >= 0.0;
}

double ResourceSatisfaction(double available, double required) {
  if (required <= kEpsilon) return 1.0;
  return std::clamp(available / required, 0.0, 1.0);
}

double TemperatureResponse(double temperature, double minimum,
                           double optimal, double maximum) {
  if (temperature < minimum || temperature > maximum) return 0.0;
  if (std::abs(temperature - optimal) <= kEpsilon) return 1.0;
  if (temperature < optimal) {
    const double span = optimal - minimum;
    return span <= kEpsilon ? 1.0 : std::clamp((temperature - minimum) / span, 0.0, 1.0);
  }
  const double span = maximum - optimal;
  return span <= kEpsilon ? 1.0 : std::clamp((maximum - temperature) / span, 0.0, 1.0);
}

bool ValidChunkKind(ChunkWorkKind kind) {
  switch (kind) {
    case ChunkWorkKind::SnapshotRead:
    case ChunkWorkKind::DensityEdit:
    case ChunkWorkKind::Classification:
    case ChunkWorkKind::MeshBuild:
    case ChunkWorkKind::CollisionBuild:
    case ChunkWorkKind::NavigationBuild:
      return true;
  }
  return false;
}

std::uint64_t HashMix(std::uint64_t seed, std::uint64_t value) {
  // Stable, non-cryptographic fingerprint for stale-plan detection, not security.
  seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
  return seed;
}

std::uint64_t DoubleBits(double value) {
  std::uint64_t bits = 0;
  static_assert(sizeof(bits) == sizeof(value), "unexpected double width");
  std::memcpy(&bits, &value, sizeof(bits));
  return bits;
}

std::uint64_t InventoryFingerprint(const CraftingInventorySnapshot& inventory) {
  std::uint64_t hash = 0xcbf29ce484222325ULL;
  hash = HashMix(hash, inventory.revision);
  hash = HashMix(hash, static_cast<std::uint64_t>(inventory.slotCapacity));
  hash = HashMix(hash, DoubleBits(inventory.weightLimitKg));
  hash = HashMix(hash, static_cast<std::uint64_t>(inventory.slots.size()));
  for (const auto& slot : inventory.slots) {
    hash = HashMix(hash, slot.itemDefinitionId);
    hash = HashMix(hash, slot.count);
    hash = HashMix(hash, slot.maxStack);
    hash = HashMix(hash, DoubleBits(slot.unitWeightKg));
  }
  return hash;
}

bool SameStackDefinition(const InventoryStack& lhs, const InventoryStack& rhs) {
  return lhs.itemDefinitionId == rhs.itemDefinitionId &&
         lhs.maxStack == rhs.maxStack &&
         std::abs(lhs.unitWeightKg - rhs.unitWeightKg) <= kEpsilon;
}

}  // namespace

// --------------------------- Raid targeting --------------------------------

Result<RaidTargetDecision> RaidTargetScorer::Choose(
    const std::vector<RaidTargetCandidate>& candidates,
    Revision expectedWorldRevision,
    const RaidTargetWeights& weights,
    std::size_t maxCandidates) {
  if (expectedWorldRevision == 0)
    return Result<RaidTargetDecision>::Failure(ErrorCode::InvalidArgument,
                                                "expected world revision must be nonzero");
  if (maxCandidates == 0 || maxCandidates > kHardCandidateLimit)
    return Result<RaidTargetDecision>::Failure(ErrorCode::InvalidArgument,
                                                "candidate cap is outside the supported range");
  if (candidates.size() > maxCandidates)
    return Result<RaidTargetDecision>::Failure(ErrorCode::BudgetExceeded,
                                                "raid target candidate budget exceeded");

  const double allWeights[] = {weights.threat, weights.accessibility,
      weights.strategicValue, weights.defensiveExposure, weights.distancePenalty};
  double featureWeightSum = 0.0;
  for (const double weight : allWeights) {
    if (!NonNegativeFinite(weight))
      return Result<RaidTargetDecision>::Failure(ErrorCode::InvalidArgument,
                                                  "target scoring weights must be finite and nonnegative");
  }
  featureWeightSum = weights.threat + weights.accessibility +
      weights.strategicValue + weights.defensiveExposure;
  if (!std::isfinite(featureWeightSum) || featureWeightSum <= kEpsilon ||
      !FiniteRange(weights.distanceNormalizationM, 1e-6, 1e9))
    return Result<RaidTargetDecision>::Failure(ErrorCode::InvalidArgument,
                                                "target feature weights and distance normalization are invalid");

  std::set<Id> seenIds;
  struct ScoredTarget {
    Id id;
    double score;
  };
  std::vector<ScoredTarget> scored;
  scored.reserve(candidates.size());

  for (const auto& candidate : candidates) {
    if (candidate.targetId == 0 || candidate.worldRevision == 0)
      return Result<RaidTargetDecision>::Failure(ErrorCode::InvalidArgument,
                                                  "target ID and revision must be nonzero");
    if (!seenIds.insert(candidate.targetId).second)
      return Result<RaidTargetDecision>::Failure(ErrorCode::Conflict,
                                                  "duplicate raid target ID");
    const double features[] = {candidate.threat01, candidate.accessibility01,
        candidate.strategicValue01, candidate.defensiveExposure01};
    for (const double feature : features) {
      if (!FiniteRange(feature, 0.0, 1.0))
        return Result<RaidTargetDecision>::Failure(ErrorCode::InvalidArgument,
                                                    "target features must be normalized to [0,1]");
    }
    if (!NonNegativeFinite(candidate.distanceM))
      return Result<RaidTargetDecision>::Failure(ErrorCode::InvalidArgument,
                                                  "target distance must be finite and nonnegative");
    if (candidate.worldRevision != expectedWorldRevision ||
        !candidate.alive || !candidate.reachable || !candidate.visible)
      continue;

    const double utility =
        (candidate.threat01 * weights.threat +
         candidate.accessibility01 * weights.accessibility +
         candidate.strategicValue01 * weights.strategicValue +
         candidate.defensiveExposure01 * weights.defensiveExposure) / featureWeightSum;
    const double normalizedDistance =
        std::clamp(candidate.distanceM / weights.distanceNormalizationM, 0.0, 1.0);
    const double score = utility - weights.distancePenalty * normalizedDistance;
    if (!std::isfinite(score))
      return Result<RaidTargetDecision>::Failure(ErrorCode::OutOfRange,
                                                  "target score overflowed");
    scored.push_back({candidate.targetId, score});
  }

  if (scored.empty())
    return Result<RaidTargetDecision>::Failure(ErrorCode::NotFound,
                                                "no live, visible, reachable target matches the current revision");

  std::sort(scored.begin(), scored.end(), [](const auto& lhs, const auto& rhs) {
    if (std::abs(lhs.score - rhs.score) > kEpsilon) return lhs.score > rhs.score;
    return lhs.id < rhs.id;
  });
  return Result<RaidTargetDecision>::Success({
      scored.front().id, expectedWorldRevision, scored.front().score,
      scored.size(), scored.size()});
}

// ---------------------------- Spawn outcomes -------------------------------

Result<SpawnDecision> SpawnDecisionPipeline::Choose(
    const SpawnRequest& request,
    const std::vector<SpawnCandidate>& candidates,
    std::uint32_t availableAttempts,
    std::size_t maxCandidates) {
  if (request.requestId == 0 || request.expectedWorldRevision == 0 ||
      request.maxAttempts == 0)
    return Result<SpawnDecision>::Failure(ErrorCode::InvalidArgument,
                                          "spawn request ID, revision and attempt cap must be nonzero");
  if (maxCandidates == 0 || maxCandidates > kHardCandidateLimit)
    return Result<SpawnDecision>::Failure(ErrorCode::InvalidArgument,
                                          "spawn candidate cap is outside the supported range");
  if (candidates.size() > maxCandidates)
    return Result<SpawnDecision>::Failure(ErrorCode::BudgetExceeded,
                                          "spawn candidate input exceeds its cap");

  SpawnDecision outcome;
  if (candidates.empty()) {
    outcome.disposition = SpawnDisposition::NoCandidates;
    outcome.reason = "no spawn candidates were supplied";
    return Result<SpawnDecision>::Success(outcome);
  }
  if (availableAttempts == 0) {
    outcome.disposition = SpawnDisposition::AttemptBudgetExhausted;
    outcome.reason = "no spawn attempts are available this frame";
    return Result<SpawnDecision>::Success(outcome);
  }

  std::set<Id> seenIds;
  std::vector<SpawnCandidate> ordered = candidates;
  for (const auto& candidate : ordered) {
    if (candidate.candidateId == 0 || candidate.worldRevision == 0 ||
        !candidate.positionM.IsFinite() || !std::isfinite(candidate.utility))
      return Result<SpawnDecision>::Failure(ErrorCode::InvalidArgument,
                                            "spawn candidate ID, revision, position and utility must be valid");
    if (!seenIds.insert(candidate.candidateId).second)
      return Result<SpawnDecision>::Failure(ErrorCode::Conflict,
                                            "duplicate spawn candidate ID");
  }
  std::sort(ordered.begin(), ordered.end(), [](const auto& lhs, const auto& rhs) {
    if (std::abs(lhs.utility - rhs.utility) > kEpsilon) return lhs.utility > rhs.utility;
    return lhs.candidateId < rhs.candidateId;
  });

  const std::size_t attemptLimit = std::min<std::size_t>(
      {ordered.size(), request.maxAttempts, availableAttempts});
  bool sawNavigationAvailable = false;
  bool sawBlocked = false;

  for (std::size_t index = 0; index < attemptLimit; ++index) {
    const auto& candidate = ordered[index];
    ++outcome.attemptsUsed;

    if (candidate.worldRevision != request.expectedWorldRevision)
      continue;
    if (!candidate.navigationAvailable)
      continue;

    sawNavigationAvailable = true;
    if (!candidate.regionAllowed || !candidate.overlapFree) {
      sawBlocked = true;
      continue;
    }

    outcome.disposition = SpawnDisposition::Approved;
    outcome.candidateId = candidate.candidateId;
    outcome.positionM = candidate.positionM;
    outcome.utility = candidate.utility;
    outcome.reason = "candidate passes navigation, region and overlap preflight";
    return Result<SpawnDecision>::Success(outcome);
  }

  if (attemptLimit < ordered.size()) {
    outcome.disposition = SpawnDisposition::AttemptBudgetExhausted;
    outcome.reason = "candidate evaluation stopped at the attempt budget";
  } else if (!sawNavigationAvailable) {
    outcome.disposition = SpawnDisposition::NavigationUnavailable;
    outcome.reason = "no current candidate supplied a usable navigation result";
  } else if (sawBlocked) {
    outcome.disposition = SpawnDisposition::AllCandidatesBlocked;
    outcome.reason = "all navigable current candidates are region-denied or overlapping";
  } else {
    outcome.disposition = SpawnDisposition::NoCandidates;
    outcome.reason = "no candidate matched the current world revision";
  }
  return Result<SpawnDecision>::Success(outcome);
}

// ------------------------- Chunk work scheduler ----------------------------

ChunkWorkScheduler::ChunkWorkScheduler(ChunkSchedulerConfig config)
    : config_(config) {}

bool ChunkWorkScheduler::ValidateConfig(std::string* reason) const {
  auto fail = [&](const char* message) {
    if (reason) *reason = message;
    return false;
  };
  if (config_.maxQueuedJobs == 0 || config_.maxQueuedJobs > kHardSchedulerQueueLimit)
    return fail("maxQueuedJobs is outside the supported range");
  if (config_.maxJobCostUnits == 0 || config_.maxJobCostUnits > 65536)
    return fail("maxJobCostUnits is outside the supported range");
  if (config_.maxCostPerDrain == 0 ||
      config_.maxCostPerDrain < config_.maxJobCostUnits ||
      config_.maxCostPerDrain > 1048576)
    return fail("maxCostPerDrain must accommodate every legal job");
  if (config_.maxJobsPerDrain == 0 || config_.maxJobsPerDrain > 4096)
    return fail("maxJobsPerDrain is outside the supported range");
  if (config_.maxExaminedPerDrain == 0 || config_.maxExaminedPerDrain > 16384)
    return fail("maxExaminedPerDrain is outside the supported range");
  return true;
}

bool ChunkWorkScheduler::IsValidKind(ChunkWorkKind kind) const noexcept {
  return ValidChunkKind(kind);
}

Result<bool> ChunkWorkScheduler::Schedule(const ChunkWorkItem& item) {
  std::string reason;
  if (!ValidateConfig(&reason))
    return Result<bool>::Failure(ErrorCode::InvalidArgument, reason);
  if (item.workId == 0 || item.sourceRevision == 0 || item.costUnits == 0)
    return Result<bool>::Failure(ErrorCode::InvalidArgument,
                                 "work ID, source revision and cost must be nonzero");
  if (!IsValidKind(item.kind))
    return Result<bool>::Failure(ErrorCode::InvalidArgument, "unknown chunk work kind");
  if (item.costUnits > config_.maxJobCostUnits)
    return Result<bool>::Failure(ErrorCode::BudgetExceeded, "job exceeds maxJobCostUnits");
  if (workOwners_.find(item.workId) != workOwners_.end())
    return Result<bool>::Failure(ErrorCode::Conflict, "work ID is already queued");

  auto queueIt = queues_.find(item.chunk);
  if (queueIt != queues_.end()) {
    auto existing = std::find_if(queueIt->second.begin(), queueIt->second.end(),
        [&](const ChunkWorkItem& queued) { return queued.kind == item.kind; });
    if (existing != queueIt->second.end()) {
      if (item.sourceRevision <= existing->sourceRevision)
        return Result<bool>::Failure(
            item.sourceRevision < existing->sourceRevision
                ? ErrorCode::StaleRevision : ErrorCode::Conflict,
            "duplicate or older same-kind chunk work cannot replace a queued job");
      workOwners_.erase(existing->workId);
      *existing = item;
      workOwners_[item.workId] = item.chunk;
      return Result<bool>::Success(true);
    }
  }

  if (queuedCount_ >= config_.maxQueuedJobs)
    return Result<bool>::Failure(ErrorCode::BudgetExceeded, "chunk work queue is full");

  queues_[item.chunk].push_back(item);
  workOwners_[item.workId] = item.chunk;
  ++queuedCount_;
  return Result<bool>::Success(true);
}

void ChunkWorkScheduler::RemoveFrontAndTrack(const ChunkCoord& coord) {
  auto found = queues_.find(coord);
  if (found == queues_.end() || found->second.empty()) return;
  workOwners_.erase(found->second.front().workId);
  found->second.pop_front();
  if (queuedCount_ > 0) --queuedCount_;
  if (found->second.empty()) queues_.erase(found);
}

std::optional<ChunkCoord> ChunkWorkScheduler::NextNonEmptyChunk() const {
  if (queues_.empty()) return std::nullopt;
  auto isNonEmpty = [](const auto& entry) { return !entry.second.empty(); };
  auto it = lastServedChunk_ ? queues_.upper_bound(*lastServedChunk_) : queues_.begin();
  for (; it != queues_.end(); ++it) {
    if (isNonEmpty(*it)) return it->first;
  }
  for (it = queues_.begin(); it != queues_.end(); ++it) {
    if (isNonEmpty(*it)) return it->first;
  }
  return std::nullopt;
}

Result<ChunkDrainReport> ChunkWorkScheduler::Drain(
    const std::map<ChunkCoord, Revision>& liveRevisions) {
  std::string reason;
  if (!ValidateConfig(&reason))
    return Result<ChunkDrainReport>::Failure(ErrorCode::InvalidArgument, reason);

  ChunkDrainReport report;
  while (report.examined < config_.maxExaminedPerDrain &&
         report.ready.size() < config_.maxJobsPerDrain) {
    const auto nextCoord = NextNonEmptyChunk();
    if (!nextCoord) break;

    auto queueIt = queues_.find(*nextCoord);
    if (queueIt == queues_.end() || queueIt->second.empty()) {
      lastServedChunk_ = *nextCoord;
      continue;
    }
    const ChunkWorkItem item = queueIt->second.front();
    ++report.examined;

    const auto live = liveRevisions.find(item.chunk);
    if (live == liveRevisions.end() || item.sourceRevision < live->second) {
      RemoveFrontAndTrack(item.chunk);
      ++report.staleDiscarded;
      lastServedChunk_ = item.chunk;
      continue;
    }
    if (item.sourceRevision > live->second) {
      // The producer is ahead of the published source; defer rather than dispatch stale work.
      auto& queue = queueIt->second;
      queue.pop_front();
      queue.push_back(item);
      ++report.deferred;
      lastServedChunk_ = item.chunk;
      continue;
    }
    if (report.costConsumed + item.costUnits > config_.maxCostPerDrain) {
      // Rotate an expensive job so smaller work in other chunks can still use this frame's budget.
      auto& queue = queueIt->second;
      queue.pop_front();
      queue.push_back(item);
      ++report.deferred;
      lastServedChunk_ = item.chunk;
      continue;
    }

    report.ready.push_back(item);
    report.costConsumed += item.costUnits;
    RemoveFrontAndTrack(item.chunk);
    lastServedChunk_ = item.chunk;
  }

  report.remainingQueued = queuedCount_;
  return Result<ChunkDrainReport>::Success(std::move(report));
}

// ------------------------- Material M1-M6 rules ----------------------------

bool MaterialRuleResolver::IsValidInteraction(
    MaterialInteraction interaction) const noexcept {
  switch (interaction) {
    case MaterialInteraction::Fire:
    case MaterialInteraction::Water:
    case MaterialInteraction::Impact:
    case MaterialInteraction::Weather:
    case MaterialInteraction::Time:
      return true;
  }
  return false;
}

bool MaterialRuleResolver::IsValidScope(MaterialRuleScope scope) const noexcept {
  switch (scope) {
    case MaterialRuleScope::Global:
    case MaterialRuleScope::Definition:
    case MaterialRuleScope::Composite:
    case MaterialRuleScope::Instance:
      return true;
  }
  return false;
}

int MaterialRuleResolver::ScopeRank(MaterialRuleScope scope) const noexcept {
  switch (scope) {
    case MaterialRuleScope::Global: return 0;
    case MaterialRuleScope::Definition: return 1;
    case MaterialRuleScope::Composite: return 2;
    case MaterialRuleScope::Instance: return 3;
  }
  return -1;
}

bool MaterialRuleResolver::ValidateRule(
    const MaterialInteractionRule& rule, std::string* reason) const {
  auto fail = [&](const char* message) {
    if (reason) *reason = message;
    return false;
  };
  if (rule.ruleId == 0) return fail("material rule ID must be nonzero");
  if (!IsValidScope(rule.scope) || !IsValidInteraction(rule.interaction))
    return fail("unknown material rule scope or interaction");
  if (!FiniteRange(rule.minObservedValue, -1.0e9, 1.0e9) ||
      !FiniteRange(rule.maxObservedValue, -1.0e9, 1.0e9) ||
      rule.minObservedValue > rule.maxObservedValue)
    return fail("material rule observation interval is invalid");
  if (!FiniteRange(rule.durabilityDelta, -1.0e6, 1.0e6) ||
      !FiniteRange(rule.moistureDelta, -1.0, 1.0) ||
      !FiniteRange(rule.temperatureDeltaC, -3000.0, 3000.0))
    return fail("material rule effect is outside supported bounds");

  switch (rule.scope) {
    case MaterialRuleScope::Global:
      if (rule.materialId != 0 || rule.contextId != 0)
        return fail("global rules must not bind material or context IDs");
      break;
    case MaterialRuleScope::Definition:
      if (rule.materialId == 0 || rule.contextId != 0)
        return fail("definition rules require a material ID and no context ID");
      break;
    case MaterialRuleScope::Composite:
    case MaterialRuleScope::Instance:
      if (rule.materialId == 0 || rule.contextId == 0)
        return fail("composite/instance rules require material and context IDs");
      break;
  }
  return true;
}

Result<bool> MaterialRuleResolver::UpsertRule(
    const MaterialInteractionRule& rule) {
  std::string reason;
  if (!ValidateRule(rule, &reason))
    return Result<bool>::Failure(ErrorCode::InvalidArgument, reason);
  const auto existing = rules_.find(rule.ruleId);
  if (existing == rules_.end() && rules_.size() >= maxRules_)
    return Result<bool>::Failure(ErrorCode::BudgetExceeded, "material rule registry is full");
  if (revision_ == std::numeric_limits<Revision>::max())
    return Result<bool>::Failure(ErrorCode::OutOfRange, "material registry revision exhausted");

  rules_[rule.ruleId] = rule;
  ++revision_;
  candidateCache_.clear();
  return Result<bool>::Success(true);
}

Result<bool> MaterialRuleResolver::RemoveRule(Id ruleId) {
  if (ruleId == 0)
    return Result<bool>::Failure(ErrorCode::InvalidArgument, "material rule ID must be nonzero");
  const auto found = rules_.find(ruleId);
  if (found == rules_.end())
    return Result<bool>::Failure(ErrorCode::NotFound, "material rule does not exist");
  if (revision_ == std::numeric_limits<Revision>::max())
    return Result<bool>::Failure(ErrorCode::OutOfRange, "material registry revision exhausted");

  rules_.erase(found);
  ++revision_;
  candidateCache_.clear();
  return Result<bool>::Success(true);
}

const std::vector<Id>& MaterialRuleResolver::CandidateRuleIds(
    Id materialId, MaterialInteraction interaction) const {
  const CacheKey key{materialId, interaction};
  const auto cached = candidateCache_.find(key);
  if (cached != candidateCache_.end()) return cached->second;

  std::vector<Id> ids;
  for (const auto& entry : rules_) {
    const auto& rule = entry.second;
    if (rule.interaction != interaction) continue;
    if (rule.scope == MaterialRuleScope::Global ||
        rule.materialId == materialId) {
      ids.push_back(rule.ruleId);
    }
  }
  std::sort(ids.begin(), ids.end(), [&](Id lhsId, Id rhsId) {
    const auto& lhs = rules_.at(lhsId);
    const auto& rhs = rules_.at(rhsId);
    if (ScopeRank(lhs.scope) != ScopeRank(rhs.scope))
      return ScopeRank(lhs.scope) > ScopeRank(rhs.scope);
    if (lhs.priority != rhs.priority) return lhs.priority > rhs.priority;
    return lhs.ruleId < rhs.ruleId;
  });
  return candidateCache_.emplace(key, std::move(ids)).first->second;
}

bool MaterialRuleResolver::AppliesToContext(
    const MaterialInteractionRule& rule,
    Id materialId, Id compositeId, Id instanceId) const noexcept {
  switch (rule.scope) {
    case MaterialRuleScope::Global:
      return true;
    case MaterialRuleScope::Definition:
      return rule.materialId == materialId;
    case MaterialRuleScope::Composite:
      return rule.materialId == materialId && rule.contextId == compositeId &&
             compositeId != 0;
    case MaterialRuleScope::Instance:
      return rule.materialId == materialId && rule.contextId == instanceId &&
             instanceId != 0;
  }
  return false;
}

Result<MaterialRuleMatch> MaterialRuleResolver::Resolve(
    Id materialId, Id compositeId, Id instanceId,
    MaterialInteraction interaction, double observedValue) const {
  if (materialId == 0 || !IsValidInteraction(interaction) ||
      !std::isfinite(observedValue))
    return Result<MaterialRuleMatch>::Failure(ErrorCode::InvalidArgument,
                                               "material context or observation is invalid");

  const auto& candidates = CandidateRuleIds(materialId, interaction);
  for (const Id ruleId : candidates) {
    const auto found = rules_.find(ruleId);
    if (found == rules_.end()) continue;
    const auto& rule = found->second;
    if (!AppliesToContext(rule, materialId, compositeId, instanceId)) continue;
    if (observedValue < rule.minObservedValue ||
        observedValue > rule.maxObservedValue) continue;
    return Result<MaterialRuleMatch>::Success({
        rule.ruleId, rule.scope, rule.priority, rule.durabilityDelta,
        rule.moistureDelta, rule.temperatureDeltaC, rule.ignite, rule.destroy,
        revision_});
  }
  return Result<MaterialRuleMatch>::Failure(ErrorCode::NotFound,
                                             "no material rule matches context and observation");
}

// ----------------------- Plant growth and animal care ----------------------

Result<PlantGrowthResult> PlantGrowthModel::Advance(
    const PlantGrowthDefinition& definition,
    const PlantGrowthState& state,
    const PlantGrowthInput& input) {
  if (definition.definitionId == 0 || state.plantId == 0 ||
      state.definitionId != definition.definitionId || state.revision == 0)
    return Result<PlantGrowthResult>::Failure(ErrorCode::InvalidArgument,
                                               "plant IDs or revision do not match");
  if (!FiniteRange(definition.growthRatePerDay, 0.0, 10.0) ||
      !FiniteRange(definition.minimumTemperatureC, -273.15, 3000.0) ||
      !FiniteRange(definition.optimalTemperatureC, -273.15, 3000.0) ||
      !FiniteRange(definition.maximumTemperatureC, -273.15, 3000.0) ||
      !(definition.minimumTemperatureC < definition.optimalTemperatureC &&
        definition.optimalTemperatureC < definition.maximumTemperatureC) ||
      !FiniteRange(definition.idealLight01, 0.0, 1.0) ||
      !FiniteRange(definition.waterNeedM3PerDay, 0.0, 1.0e6) ||
      !FiniteRange(definition.nitrogenNeedKgPerDay, 0.0, 1.0e6) ||
      !FiniteRange(definition.phosphorusNeedKgPerDay, 0.0, 1.0e6) ||
      !FiniteRange(definition.potassiumNeedKgPerDay, 0.0, 1.0e6))
    return Result<PlantGrowthResult>::Failure(ErrorCode::InvalidArgument,
                                               "plant definition values are invalid");
  if (!FiniteRange(state.ageDays, 0.0, 1.0e9) ||
      !FiniteRange(state.growth01, 0.0, 1.0))
    return Result<PlantGrowthResult>::Failure(ErrorCode::CorruptData,
                                               "plant growth state is invalid");
  const double resources[] = {input.waterAvailableM3, input.nitrogenAvailableKg,
      input.phosphorusAvailableKg, input.potassiumAvailableKg};
  for (const double resource : resources) {
    if (!NonNegativeFinite(resource) || resource > 1.0e9)
      return Result<PlantGrowthResult>::Failure(ErrorCode::InvalidArgument,
                                                 "plant resources must be finite and nonnegative");
  }
  if (!FiniteRange(input.deltaDays, 0.0, kMaxSafeDays) ||
      !FiniteRange(input.temperatureC, -273.15, 3000.0) ||
      !FiniteRange(input.light01, 0.0, 1.0))
    return Result<PlantGrowthResult>::Failure(ErrorCode::InvalidArgument,
                                               "plant time, temperature or light input is invalid");
  if (input.deltaDays > 0.0 &&
      state.revision == std::numeric_limits<Revision>::max())
    return Result<PlantGrowthResult>::Failure(ErrorCode::OutOfRange,
                                               "plant revision exhausted");

  PlantGrowthResult result;
  result.nextState = state;
  if (input.deltaDays <= 0.0) return Result<PlantGrowthResult>::Success(result);

  const auto steps = static_cast<std::uint32_t>(
      std::ceil(input.deltaDays / kDaysPerGrowthStep));
  if (steps == 0 || steps > 120)
    return Result<PlantGrowthResult>::Failure(ErrorCode::BudgetExceeded,
                                               "plant growth substep budget exceeded");
  const double dt = input.deltaDays / static_cast<double>(steps);
  double waterLeft = input.waterAvailableM3;
  double nitrogenLeft = input.nitrogenAvailableKg;
  double phosphorusLeft = input.phosphorusAvailableKg;
  double potassiumLeft = input.potassiumAvailableKg;
  const double temperatureFactor = TemperatureResponse(
      input.temperatureC, definition.minimumTemperatureC,
      definition.optimalTemperatureC, definition.maximumTemperatureC);
  const double lightFactor = definition.idealLight01 <= kEpsilon
      ? 1.0 : std::clamp(input.light01 / definition.idealLight01, 0.0, 1.0);
  const double growthBefore = result.nextState.growth01;

  for (std::uint32_t step = 0; step < steps; ++step) {
    const double waterDemand = definition.waterNeedM3PerDay * dt;
    const double nitrogenDemand = definition.nitrogenNeedKgPerDay * dt;
    const double phosphorusDemand = definition.phosphorusNeedKgPerDay * dt;
    const double potassiumDemand = definition.potassiumNeedKgPerDay * dt;

    const double waterUsed = std::min(waterLeft, waterDemand);
    const double nitrogenUsed = std::min(nitrogenLeft, nitrogenDemand);
    const double phosphorusUsed = std::min(phosphorusLeft, phosphorusDemand);
    const double potassiumUsed = std::min(potassiumLeft, potassiumDemand);
    waterLeft -= waterUsed;
    nitrogenLeft -= nitrogenUsed;
    phosphorusLeft -= phosphorusUsed;
    potassiumLeft -= potassiumUsed;
    result.waterConsumedM3 += waterUsed;
    result.nitrogenConsumedKg += nitrogenUsed;
    result.phosphorusConsumedKg += phosphorusUsed;
    result.potassiumConsumedKg += potassiumUsed;

    const double waterFactor = ResourceSatisfaction(waterUsed, waterDemand);
    const double nutrientFactor = std::min({
        ResourceSatisfaction(nitrogenUsed, nitrogenDemand),
        ResourceSatisfaction(phosphorusUsed, phosphorusDemand),
        ResourceSatisfaction(potassiumUsed, potassiumDemand)});
    const double growth = definition.growthRatePerDay * dt *
        temperatureFactor * lightFactor * waterFactor * nutrientFactor;
    result.nextState.growth01 = std::clamp(result.nextState.growth01 + growth, 0.0, 1.0);
    result.nextState.ageDays += dt;
    ++result.stepsProcessed;
  }

  if (!std::isfinite(result.nextState.ageDays) ||
      !std::isfinite(result.nextState.growth01) ||
      !std::isfinite(result.waterConsumedM3) ||
      !std::isfinite(result.nitrogenConsumedKg) ||
      !std::isfinite(result.phosphorusConsumedKg) ||
      !std::isfinite(result.potassiumConsumedKg))
    return Result<PlantGrowthResult>::Failure(ErrorCode::OutOfRange,
                                               "plant update overflowed");
  ++result.nextState.revision;
  result.becameMature = growthBefore < 1.0 && result.nextState.growth01 >= 1.0;
  return Result<PlantGrowthResult>::Success(result);
}

Result<AnimalCareResult> AnimalCareModel::Advance(
    const AnimalCareDefinition& definition,
    const AnimalCareState& state,
    const AnimalCareInput& input) {
  if (definition.definitionId == 0 || definition.speciesId == 0 ||
      state.animalId == 0 || state.definitionId != definition.definitionId ||
      state.revision == 0)
    return Result<AnimalCareResult>::Failure(ErrorCode::InvalidArgument,
                                               "animal IDs or revision do not match");
  if (!FiniteRange(definition.feedNeedKgPerDay, 0.0, 1.0e6) ||
      !FiniteRange(definition.waterNeedLitersPerDay, 0.0, 1.0e6) ||
      !FiniteRange(definition.growthRatePerDay, 0.0, 10.0) ||
      !FiniteRange(definition.productionUnitsPerDay, 0.0, 1.0e6) ||
      !FiniteRange(definition.maturityGrowth01, 0.0, 1.0) ||
      !FiniteRange(definition.starvationHealthLossPerDay, 0.0, 1.0) ||
      !FiniteRange(definition.recoveryPerDay, 0.0, 1.0) ||
      !FiniteRange(definition.reproductionHealthMinimum01, 0.0, 1.0) ||
      !FiniteRange(definition.reproductionReadyDaysRequired, 0.0, kMaxSafeDays))
    return Result<AnimalCareResult>::Failure(ErrorCode::InvalidArgument,
                                               "animal definition values are invalid");
  if (!FiniteRange(state.ageDays, 0.0, 1.0e9) ||
      !FiniteRange(state.growth01, 0.0, 1.0) ||
      !FiniteRange(state.health01, 0.0, 1.0) ||
      !FiniteRange(state.reproductionReadyDays, 0.0, 1.0e9))
    return Result<AnimalCareResult>::Failure(ErrorCode::CorruptData,
                                               "animal state is invalid");
  if (!FiniteRange(input.deltaDays, 0.0, kMaxSafeDays) ||
      !NonNegativeFinite(input.feedAvailableKg) ||
      !NonNegativeFinite(input.waterAvailableLiters) ||
      input.feedAvailableKg > 1.0e9 || input.waterAvailableLiters > 1.0e9)
    return Result<AnimalCareResult>::Failure(ErrorCode::InvalidArgument,
                                               "animal time/feed/water input is invalid");
  if (input.deltaDays > 0.0 &&
      state.revision == std::numeric_limits<Revision>::max())
    return Result<AnimalCareResult>::Failure(ErrorCode::OutOfRange,
                                               "animal revision exhausted");

  AnimalCareResult result;
  result.nextState = state;
  if (input.deltaDays <= 0.0) {
    result.reproductionReady =
        state.reproductionReadyDays >= definition.reproductionReadyDaysRequired &&
        state.growth01 >= definition.maturityGrowth01 &&
        state.health01 >= definition.reproductionHealthMinimum01;
    return Result<AnimalCareResult>::Success(result);
  }
  const auto steps = static_cast<std::uint32_t>(
      std::ceil(input.deltaDays / kDaysPerGrowthStep));
  if (steps == 0 || steps > 120)
    return Result<AnimalCareResult>::Failure(ErrorCode::BudgetExceeded,
                                               "animal care substep budget exceeded");
  const double dt = input.deltaDays / static_cast<double>(steps);
  double feedLeft = input.feedAvailableKg;
  double waterLeft = input.waterAvailableLiters;

  for (std::uint32_t step = 0; step < steps; ++step) {
    const double feedDemand = definition.feedNeedKgPerDay * dt;
    const double waterDemand = definition.waterNeedLitersPerDay * dt;
    const double feedUsed = std::min(feedLeft, feedDemand);
    const double waterUsed = std::min(waterLeft, waterDemand);
    feedLeft -= feedUsed;
    waterLeft -= waterUsed;
    result.feedConsumedKg += feedUsed;
    result.waterConsumedLiters += waterUsed;
    const double feedRatio = ResourceSatisfaction(feedUsed, feedDemand);
    const double waterRatio = ResourceSatisfaction(waterUsed, waterDemand);
    const double satisfaction = std::min(feedRatio, waterRatio);

    if (satisfaction < 1.0) {
      result.nextState.health01 = std::max(
          0.0, result.nextState.health01 -
          definition.starvationHealthLossPerDay * dt * (1.0 - satisfaction));
    } else {
      result.nextState.health01 = std::min(
          1.0, result.nextState.health01 + definition.recoveryPerDay * dt);
    }

    const bool adultAtStepStart =
        result.nextState.growth01 >= definition.maturityGrowth01;
    if (result.nextState.health01 > 0.0) {
      result.nextState.growth01 = std::clamp(
          result.nextState.growth01 + definition.growthRatePerDay * dt *
          satisfaction * result.nextState.health01, 0.0, 1.0);
      if (adultAtStepStart) {
        result.productionUnits += definition.productionUnitsPerDay * dt *
            feedRatio * waterRatio * result.nextState.health01;
      }
    }

    const bool readyForReproduction =
        result.nextState.growth01 >= definition.maturityGrowth01 &&
        result.nextState.health01 >= definition.reproductionHealthMinimum01 &&
        feedRatio >= 0.8 && waterRatio >= 0.8;
    if (readyForReproduction)
      result.nextState.reproductionReadyDays += dt;
    else
      result.nextState.reproductionReadyDays = 0.0;
    result.nextState.ageDays += dt;
  }

  if (!FiniteRange(result.nextState.ageDays, 0.0, 1.0e12) ||
      !FiniteRange(result.nextState.growth01, 0.0, 1.0) ||
      !FiniteRange(result.nextState.health01, 0.0, 1.0) ||
      !NonNegativeFinite(result.productionUnits) ||
      !NonNegativeFinite(result.feedConsumedKg) ||
      !NonNegativeFinite(result.waterConsumedLiters) ||
      !NonNegativeFinite(result.nextState.reproductionReadyDays))
    return Result<AnimalCareResult>::Failure(ErrorCode::OutOfRange,
                                               "animal update overflowed");

  result.feedSatisfaction01 = ResourceSatisfaction(
      result.feedConsumedKg, definition.feedNeedKgPerDay * input.deltaDays);
  result.waterSatisfaction01 = ResourceSatisfaction(
      result.waterConsumedLiters, definition.waterNeedLitersPerDay * input.deltaDays);
  result.reproductionReady =
      result.nextState.reproductionReadyDays >= definition.reproductionReadyDaysRequired &&
      result.nextState.growth01 >= definition.maturityGrowth01 &&
      result.nextState.health01 >= definition.reproductionHealthMinimum01;
  ++result.nextState.revision;
  return Result<AnimalCareResult>::Success(result);
}

Result<std::vector<AnimalPair>> AnimalReproductionPlanner::BuildPairs(
    const std::vector<AnimalReproductionCandidate>& candidates,
    std::size_t maxCandidates,
    std::size_t maxPairs) {
  if (maxCandidates == 0 || maxCandidates > kHardCandidateLimit ||
      maxPairs == 0 || maxPairs > 4096)
    return Result<std::vector<AnimalPair>>::Failure(
        ErrorCode::InvalidArgument, "reproduction caps are outside supported bounds");
  if (candidates.size() > maxCandidates)
    return Result<std::vector<AnimalPair>>::Failure(
        ErrorCode::BudgetExceeded, "reproduction candidate cap exceeded");

  std::set<Id> seenAnimals;
  std::map<Id, std::pair<std::vector<Id>, std::vector<Id>>> bySpecies;
  for (const auto& candidate : candidates) {
    if (candidate.animalId == 0 || candidate.speciesId == 0)
      return Result<std::vector<AnimalPair>>::Failure(
          ErrorCode::InvalidArgument, "animal and species IDs must be nonzero");
    if (!seenAnimals.insert(candidate.animalId).second)
      return Result<std::vector<AnimalPair>>::Failure(
          ErrorCode::Conflict, "duplicate animal ID in reproduction candidates");
    if (!candidate.mature || !candidate.healthy || !candidate.ready) continue;
    auto& group = bySpecies[candidate.speciesId];
    (candidate.female ? group.first : group.second).push_back(candidate.animalId);
  }

  std::vector<AnimalPair> pairs;
  pairs.reserve(std::min(maxPairs, candidates.size() / 2));
  for (auto& species : bySpecies) {
    auto& females = species.second.first;
    auto& males = species.second.second;
    std::sort(females.begin(), females.end());
    std::sort(males.begin(), males.end());
    const std::size_t pairCount = std::min(females.size(), males.size());
    for (std::size_t i = 0; i < pairCount; ++i) {
      if (pairs.size() >= maxPairs) return Result<std::vector<AnimalPair>>::Success(pairs);
      pairs.push_back({species.first, females[i], males[i]});
    }
  }
  return Result<std::vector<AnimalPair>>::Success(std::move(pairs));
}

// ------------------------- Atomic crafting planner -------------------------

Result<CraftingPlan> CraftingTransactionPlanner::Build(
    const CraftingInventorySnapshot& inventory,
    const std::vector<CraftIngredient>& ingredients,
    const std::vector<CraftOutput>& outputs,
    std::size_t maxRecipeEntries,
    std::size_t maxSlots) {
  if (inventory.revision == 0)
    return Result<CraftingPlan>::Failure(ErrorCode::InvalidArgument,
                                          "inventory revision must be nonzero");
  if (maxRecipeEntries == 0 || maxRecipeEntries > kHardRecipeEntriesLimit ||
      maxSlots == 0 || maxSlots > kHardInventorySlotsLimit)
    return Result<CraftingPlan>::Failure(ErrorCode::InvalidArgument,
                                          "crafting plan caps are outside supported bounds");
  if (inventory.slotCapacity == 0 || inventory.slotCapacity > maxSlots ||
      inventory.slots.size() > inventory.slotCapacity ||
      inventory.slots.size() > maxSlots)
    return Result<CraftingPlan>::Failure(ErrorCode::BudgetExceeded,
                                          "inventory slot capacity is invalid or exceeded");
  if (!NonNegativeFinite(inventory.weightLimitKg) || inventory.weightLimitKg > 1.0e12)
    return Result<CraftingPlan>::Failure(ErrorCode::InvalidArgument,
                                          "inventory weight limit is invalid");
  if (ingredients.empty() || outputs.empty() ||
      ingredients.size() > maxRecipeEntries || outputs.size() > maxRecipeEntries)
    return Result<CraftingPlan>::Failure(ErrorCode::InvalidArgument,
                                          "recipe must have bounded nonempty inputs and outputs");

  std::map<Id, std::uint64_t> required;
  for (const auto& ingredient : ingredients) {
    if (ingredient.itemDefinitionId == 0 || ingredient.count == 0)
      return Result<CraftingPlan>::Failure(ErrorCode::InvalidArgument,
                                            "ingredient IDs and counts must be nonzero");
    required[ingredient.itemDefinitionId] += ingredient.count;
    if (required[ingredient.itemDefinitionId] > std::numeric_limits<std::uint32_t>::max())
      return Result<CraftingPlan>::Failure(ErrorCode::OutOfRange,
                                            "aggregated ingredient count overflowed");
  }

  std::map<Id, CraftOutput> outputDefinitions;
  for (const auto& output : outputs) {
    if (output.itemDefinitionId == 0 || output.count == 0 ||
        output.maxStack == 0 || !NonNegativeFinite(output.unitWeightKg))
      return Result<CraftingPlan>::Failure(ErrorCode::InvalidArgument,
                                            "output definitions/count/weight are invalid");
    auto found = outputDefinitions.find(output.itemDefinitionId);
    if (found == outputDefinitions.end()) {
      outputDefinitions.emplace(output.itemDefinitionId, output);
    } else {
      if (found->second.maxStack != output.maxStack ||
          std::abs(found->second.unitWeightKg - output.unitWeightKg) > kEpsilon)
        return Result<CraftingPlan>::Failure(ErrorCode::Conflict,
                                              "duplicate output definitions disagree");
      const std::uint64_t total =
          static_cast<std::uint64_t>(found->second.count) + output.count;
      if (total > std::numeric_limits<std::uint32_t>::max())
        return Result<CraftingPlan>::Failure(ErrorCode::OutOfRange,
                                              "aggregated output count overflowed");
      found->second.count = static_cast<std::uint32_t>(total);
    }
  }

  std::map<Id, InventoryStack> stackDefinitions;
  std::map<Id, std::uint64_t> available;
  double originalWeight = 0.0;
  for (const auto& slot : inventory.slots) {
    if (slot.itemDefinitionId == 0 || slot.count == 0 || slot.maxStack == 0 ||
        slot.count > slot.maxStack || !NonNegativeFinite(slot.unitWeightKg))
      return Result<CraftingPlan>::Failure(ErrorCode::CorruptData,
                                            "inventory contains an invalid stack");
    auto definition = stackDefinitions.find(slot.itemDefinitionId);
    if (definition == stackDefinitions.end()) {
      stackDefinitions.emplace(slot.itemDefinitionId, slot);
    } else if (definition->second.maxStack != slot.maxStack ||
               std::abs(definition->second.unitWeightKg - slot.unitWeightKg) > kEpsilon) {
      return Result<CraftingPlan>::Failure(ErrorCode::Conflict,
                                            "same item definition has inconsistent stack metadata");
    }
    available[slot.itemDefinitionId] += slot.count;
    originalWeight += static_cast<double>(slot.count) * slot.unitWeightKg;
  }
  if (!std::isfinite(originalWeight) || originalWeight > inventory.weightLimitKg + kEpsilon)
    return Result<CraftingPlan>::Failure(ErrorCode::CorruptData,
                                          "current inventory exceeds its weight limit");

  for (const auto& needed : required) {
    const auto found = available.find(needed.first);
    if (found == available.end() || found->second < needed.second)
      return Result<CraftingPlan>::Failure(ErrorCode::OutOfRange,
                                            "recipe ingredients are insufficient");
  }

  for (const auto& output : outputDefinitions) {
    const auto existing = stackDefinitions.find(output.first);
    if (existing != stackDefinitions.end() &&
        (existing->second.maxStack != output.second.maxStack ||
         std::abs(existing->second.unitWeightKg - output.second.unitWeightKg) > kEpsilon))
      return Result<CraftingPlan>::Failure(ErrorCode::Conflict,
                                            "output metadata differs from the existing item definition");
  }

  std::vector<InventoryStack> staged = inventory.slots;
  for (const auto& needed : required) {
    std::uint64_t remaining = needed.second;
    for (auto& slot : staged) {
      if (remaining == 0) break;
      if (slot.itemDefinitionId != needed.first) continue;
      const auto consumed = static_cast<std::uint32_t>(
          std::min<std::uint64_t>(remaining, slot.count));
      slot.count -= consumed;
      remaining -= consumed;
    }
    if (remaining != 0)
      return Result<CraftingPlan>::Failure(ErrorCode::CorruptData,
                                            "staged ingredient debit did not match validated inventory");
  }
  staged.erase(std::remove_if(staged.begin(), staged.end(),
                              [](const InventoryStack& slot) { return slot.count == 0; }),
               staged.end());

  for (const auto& outputEntry : outputDefinitions) {
    const auto& output = outputEntry.second;
    std::uint64_t remaining = output.count;
    for (auto& slot : staged) {
      if (remaining == 0) break;
      if (slot.itemDefinitionId != output.itemDefinitionId) continue;
      const auto room = slot.maxStack - slot.count;
      const auto added = static_cast<std::uint32_t>(
          std::min<std::uint64_t>(remaining, room));
      slot.count += added;
      remaining -= added;
    }
    while (remaining > 0) {
      if (staged.size() >= inventory.slotCapacity)
        return Result<CraftingPlan>::Failure(ErrorCode::BudgetExceeded,
                                              "craft outputs do not fit in inventory slots");
      const auto added = static_cast<std::uint32_t>(
          std::min<std::uint64_t>(remaining, output.maxStack));
      staged.push_back({output.itemDefinitionId, added,
                        output.maxStack, output.unitWeightKg});
      remaining -= added;
    }
  }

  double resultWeight = 0.0;
  for (const auto& slot : staged)
    resultWeight += static_cast<double>(slot.count) * slot.unitWeightKg;
  if (!std::isfinite(resultWeight) || resultWeight > inventory.weightLimitKg + kEpsilon)
    return Result<CraftingPlan>::Failure(ErrorCode::BudgetExceeded,
                                          "craft outputs exceed inventory weight capacity");

  CraftingPlan plan;
  plan.expectedInventoryRevision = inventory.revision;
  plan.baseFingerprint = InventoryFingerprint(inventory);
  plan.resultingSlots = std::move(staged);
  plan.resultingWeightKg = resultWeight;
  return Result<CraftingPlan>::Success(std::move(plan));
}

Result<CraftingPlan> CraftingTransactionPlanner::Revalidate(
    const CraftingPlan& plan,
    const CraftingInventorySnapshot& liveInventory) {
  if (plan.expectedInventoryRevision == 0 ||
      liveInventory.revision != plan.expectedInventoryRevision)
    return Result<CraftingPlan>::Failure(ErrorCode::StaleRevision,
                                          "inventory changed after crafting preflight");
  if (plan.baseFingerprint == 0 ||
      InventoryFingerprint(liveInventory) != plan.baseFingerprint)
    return Result<CraftingPlan>::Failure(ErrorCode::Conflict,
                                          "inventory contents changed without a matching revision");
  if (!NonNegativeFinite(plan.resultingWeightKg) ||
      plan.resultingSlots.size() > liveInventory.slotCapacity)
    return Result<CraftingPlan>::Failure(ErrorCode::CorruptData,
                                          "staged crafting result is invalid");
  double recomputedWeight = 0.0;
  for (const auto& slot : plan.resultingSlots) {
    if (slot.itemDefinitionId == 0 || slot.count == 0 || slot.maxStack == 0 ||
        slot.count > slot.maxStack || !NonNegativeFinite(slot.unitWeightKg))
      return Result<CraftingPlan>::Failure(ErrorCode::CorruptData,
                                            "staged crafting result contains an invalid stack");
    recomputedWeight += static_cast<double>(slot.count) * slot.unitWeightKg;
  }
  if (!std::isfinite(recomputedWeight) ||
      recomputedWeight > liveInventory.weightLimitKg + kEpsilon ||
      std::abs(recomputedWeight - plan.resultingWeightKg) > kEpsilon)
    return Result<CraftingPlan>::Failure(ErrorCode::CorruptData,
                                          "staged crafting weight does not match its inventory");
  return Result<CraftingPlan>::Success(plan);
}

}  // namespace wm

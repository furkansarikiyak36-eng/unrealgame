#pragma once

#include "world_mechanics/core.hpp"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace wm {

// --------------------------- Raid targeting --------------------------------

struct RaidTargetCandidate {
  Id targetId{0};
  Revision worldRevision{0};
  double threat01{0.0};
  double accessibility01{0.0};
  double strategicValue01{0.0};
  double defensiveExposure01{0.0};
  double distanceM{0.0};
  bool alive{true};
  bool reachable{true};
  bool visible{true};
};

struct RaidTargetWeights {
  double threat{0.30};
  double accessibility{0.20};
  double strategicValue{0.30};
  double defensiveExposure{0.20};
  double distancePenalty{0.15};
  double distanceNormalizationM{100.0};
};

struct RaidTargetDecision {
  Id targetId{0};
  Revision worldRevision{0};
  double score{0.0};
  std::size_t eligibleCandidates{0};
  std::size_t evaluatedCandidates{0};
};

class RaidTargetScorer {
 public:
  static Result<RaidTargetDecision> Choose(
      const std::vector<RaidTargetCandidate>& candidates,
      Revision expectedWorldRevision,
      const RaidTargetWeights& weights = {},
      std::size_t maxCandidates = 512);
};

// ---------------------------- Spawn outcomes -------------------------------

enum class SpawnDisposition {
  Approved,
  NoCandidates,
  AttemptBudgetExhausted,
  NavigationUnavailable,
  AllCandidatesBlocked
};

struct SpawnCandidate {
  Id candidateId{0};
  Revision worldRevision{0};
  Vec3 positionM{};
  double utility{0.0};
  bool navigationAvailable{false};
  bool regionAllowed{false};
  bool overlapFree{false};
};

struct SpawnRequest {
  Id requestId{0};
  Revision expectedWorldRevision{0};
  std::uint32_t maxAttempts{32};
};

struct SpawnDecision {
  SpawnDisposition disposition{SpawnDisposition::NoCandidates};
  Id candidateId{0};
  Vec3 positionM{};
  double utility{0.0};
  std::uint32_t attemptsUsed{0};
  std::string reason;
};

class SpawnDecisionPipeline {
 public:
  // Produces a typed authorization decision. It deliberately does not spawn Actors;
  // the Unreal adapter must revalidate world revision/overlap immediately before spawn.
  static Result<SpawnDecision> Choose(
      const SpawnRequest& request,
      const std::vector<SpawnCandidate>& candidates,
      std::uint32_t availableAttempts,
      std::size_t maxCandidates = 512);
};

// ------------------------- Chunk work scheduler ----------------------------

enum class ChunkWorkKind {
  SnapshotRead,
  DensityEdit,
  Classification,
  MeshBuild,
  CollisionBuild,
  NavigationBuild
};

struct ChunkWorkItem {
  Id workId{0};
  ChunkCoord chunk{};
  ChunkWorkKind kind{ChunkWorkKind::SnapshotRead};
  Revision sourceRevision{0};
  std::uint32_t costUnits{1};
};

struct ChunkSchedulerConfig {
  std::size_t maxQueuedJobs{4096};
  std::uint32_t maxJobCostUnits{64};
  std::uint32_t maxCostPerDrain{256};
  std::uint32_t maxJobsPerDrain{64};
  std::uint32_t maxExaminedPerDrain{256};
};

struct ChunkDrainReport {
  std::vector<ChunkWorkItem> ready;
  std::uint32_t staleDiscarded{0};
  std::uint32_t deferred{0};
  std::uint32_t examined{0};
  std::uint64_t costConsumed{0};
  std::size_t remainingQueued{0};
};

class ChunkWorkScheduler {
 public:
  explicit ChunkWorkScheduler(ChunkSchedulerConfig config = {});

  Result<bool> Schedule(const ChunkWorkItem& item);
  Result<ChunkDrainReport> Drain(
      const std::map<ChunkCoord, Revision>& liveRevisions);
  std::size_t QueuedCount() const noexcept { return queuedCount_; }
  const ChunkSchedulerConfig& Config() const noexcept { return config_; }

 private:
  bool ValidateConfig(std::string* reason = nullptr) const;
  bool IsValidKind(ChunkWorkKind kind) const noexcept;
  void RemoveFrontAndTrack(const ChunkCoord& coord);
  std::optional<ChunkCoord> NextNonEmptyChunk() const;

  ChunkSchedulerConfig config_;
  std::map<ChunkCoord, std::deque<ChunkWorkItem>> queues_;
  std::map<Id, ChunkCoord> workOwners_;
  std::optional<ChunkCoord> lastServedChunk_;
  std::size_t queuedCount_{0};
};

// ------------------------- Material M1-M6 rules ----------------------------

enum class MaterialInteraction {
  Fire,
  Water,
  Impact,
  Weather,
  Time
};

enum class MaterialRuleScope {
  Global,
  Definition,
  Composite,
  Instance
};

struct MaterialInteractionRule {
  Id ruleId{0};
  MaterialRuleScope scope{MaterialRuleScope::Global};
  Id materialId{0};
  Id contextId{0};  // Composite ID or instance ID for the corresponding scope.
  MaterialInteraction interaction{MaterialInteraction::Fire};
  double minObservedValue{-1.0e9};
  double maxObservedValue{1.0e9};
  std::int32_t priority{0};
  double durabilityDelta{0.0};
  double moistureDelta{0.0};
  double temperatureDeltaC{0.0};
  bool ignite{false};
  bool destroy{false};
};

struct MaterialRuleMatch {
  Id ruleId{0};
  MaterialRuleScope scope{MaterialRuleScope::Global};
  std::int32_t priority{0};
  double durabilityDelta{0.0};
  double moistureDelta{0.0};
  double temperatureDeltaC{0.0};
  bool ignite{false};
  bool destroy{false};
  Revision registryRevision{0};
};

class MaterialRuleResolver {
 public:
  explicit MaterialRuleResolver(std::size_t maxRules = 1024)
      : maxRules_(maxRules) {}

  Result<bool> UpsertRule(const MaterialInteractionRule& rule);
  Result<bool> RemoveRule(Id ruleId);
  Result<MaterialRuleMatch> Resolve(
      Id materialId,
      Id compositeId,
      Id instanceId,
      MaterialInteraction interaction,
      double observedValue) const;

  Revision GetRevision() const noexcept { return revision_; }
  std::size_t RuleCount() const noexcept { return rules_.size(); }
  std::size_t CachedQueryCount() const noexcept { return candidateCache_.size(); }

 private:
  struct CacheKey {
    Id materialId{0};
    MaterialInteraction interaction{MaterialInteraction::Fire};
    bool operator<(const CacheKey& other) const noexcept {
      if (materialId != other.materialId) return materialId < other.materialId;
      return static_cast<int>(interaction) < static_cast<int>(other.interaction);
    }
  };

  bool ValidateRule(const MaterialInteractionRule& rule,
                    std::string* reason = nullptr) const;
  bool IsValidInteraction(MaterialInteraction interaction) const noexcept;
  bool IsValidScope(MaterialRuleScope scope) const noexcept;
  int ScopeRank(MaterialRuleScope scope) const noexcept;
  const std::vector<Id>& CandidateRuleIds(Id materialId,
                                          MaterialInteraction interaction) const;
  bool AppliesToContext(const MaterialInteractionRule& rule,
                        Id materialId, Id compositeId, Id instanceId) const noexcept;

  std::size_t maxRules_{1024};
  Revision revision_{1};
  std::map<Id, MaterialInteractionRule> rules_;
  mutable std::map<CacheKey, std::vector<Id>> candidateCache_;
};

// ----------------------- Plant growth and animal care ----------------------

struct PlantGrowthDefinition {
  Id definitionId{0};
  double growthRatePerDay{0.05};
  double minimumTemperatureC{0.0};
  double optimalTemperatureC{20.0};
  double maximumTemperatureC{40.0};
  double idealLight01{0.7};
  double waterNeedM3PerDay{0.01};
  double nitrogenNeedKgPerDay{0.001};
  double phosphorusNeedKgPerDay{0.0002};
  double potassiumNeedKgPerDay{0.0005};
};

struct PlantGrowthState {
  Id plantId{0};
  Id definitionId{0};
  double ageDays{0.0};
  double growth01{0.0};
  Revision revision{1};
};

struct PlantGrowthInput {
  double deltaDays{0.0};
  double temperatureC{20.0};
  double light01{1.0};
  double waterAvailableM3{0.0};
  double nitrogenAvailableKg{0.0};
  double phosphorusAvailableKg{0.0};
  double potassiumAvailableKg{0.0};
};

struct PlantGrowthResult {
  PlantGrowthState nextState{};
  double waterConsumedM3{0.0};
  double nitrogenConsumedKg{0.0};
  double phosphorusConsumedKg{0.0};
  double potassiumConsumedKg{0.0};
  std::uint32_t stepsProcessed{0};
  bool becameMature{false};
};

class PlantGrowthModel {
 public:
  static Result<PlantGrowthResult> Advance(
      const PlantGrowthDefinition& definition,
      const PlantGrowthState& state,
      const PlantGrowthInput& input);
};

struct AnimalCareDefinition {
  Id definitionId{0};
  Id speciesId{0};
  double feedNeedKgPerDay{0.5};
  double waterNeedLitersPerDay{2.0};
  double growthRatePerDay{0.01};
  double productionUnitsPerDay{0.0};
  double maturityGrowth01{0.8};
  double starvationHealthLossPerDay{0.05};
  double recoveryPerDay{0.02};
  double reproductionHealthMinimum01{0.6};
  double reproductionReadyDaysRequired{3.0};
};

struct AnimalCareState {
  Id animalId{0};
  Id definitionId{0};
  double ageDays{0.0};
  double growth01{0.0};
  double health01{1.0};
  double reproductionReadyDays{0.0};
  Revision revision{1};
};

struct AnimalCareInput {
  double deltaDays{0.0};
  double feedAvailableKg{0.0};
  double waterAvailableLiters{0.0};
};

struct AnimalCareResult {
  AnimalCareState nextState{};
  double feedConsumedKg{0.0};
  double waterConsumedLiters{0.0};
  double productionUnits{0.0};
  double feedSatisfaction01{1.0};
  double waterSatisfaction01{1.0};
  bool reproductionReady{false};
};

class AnimalCareModel {
 public:
  static Result<AnimalCareResult> Advance(
      const AnimalCareDefinition& definition,
      const AnimalCareState& state,
      const AnimalCareInput& input);
};

struct AnimalReproductionCandidate {
  Id animalId{0};
  Id speciesId{0};
  bool female{false};
  bool mature{false};
  bool healthy{false};
  bool ready{false};
};

struct AnimalPair {
  Id speciesId{0};
  Id femaleId{0};
  Id maleId{0};
};

class AnimalReproductionPlanner {
 public:
  static Result<std::vector<AnimalPair>> BuildPairs(
      const std::vector<AnimalReproductionCandidate>& candidates,
      std::size_t maxCandidates = 1024,
      std::size_t maxPairs = 256);
};

// ------------------------- Atomic crafting planner -------------------------

struct InventoryStack {
  Id itemDefinitionId{0};
  std::uint32_t count{0};
  std::uint32_t maxStack{1};
  double unitWeightKg{0.0};
};

struct CraftIngredient {
  Id itemDefinitionId{0};
  std::uint32_t count{0};
};

struct CraftOutput {
  Id itemDefinitionId{0};
  std::uint32_t count{0};
  std::uint32_t maxStack{1};
  double unitWeightKg{0.0};
};

struct CraftingInventorySnapshot {
  Revision revision{0};
  std::vector<InventoryStack> slots;
  std::size_t slotCapacity{0};
  double weightLimitKg{0.0};
};

struct CraftingPlan {
  Revision expectedInventoryRevision{0};
  std::uint64_t baseFingerprint{0};
  std::vector<InventoryStack> resultingSlots;
  double resultingWeightKg{0.0};
};

class CraftingTransactionPlanner {
 public:
  static Result<CraftingPlan> Build(
      const CraftingInventorySnapshot& inventory,
      const std::vector<CraftIngredient>& ingredients,
      const std::vector<CraftOutput>& outputs,
      std::size_t maxRecipeEntries = 128,
      std::size_t maxSlots = 512);

  // Returns a complete staged replacement only if the live revision still matches.
  // The caller commits this array once, after any required world outputs are staged.
  static Result<CraftingPlan> Revalidate(
      const CraftingPlan& plan,
      const CraftingInventorySnapshot& liveInventory);
};

}  // namespace wm

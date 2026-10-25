#pragma once
#include "world_mechanics/core.hpp"
#include <vector>
#include <unordered_map>

namespace wm {
enum class ShoringPieceKind { Upright, WallBoard, CrossStrut };
struct TrenchGeometry {
  Id planId{0};
  Vec3 start{}, end{}; // trench centerline at top edge
  double widthM{1.0};
  double depthM{2.0};
  double stationSpacingM{1.5};
  double boardHeightM{0.25};
  std::uint64_t maxPieces{10000};
};
struct PlannedShoringPiece {
  std::uint64_t ordinal{0};
  ShoringPieceKind kind{ShoringPieceKind::Upright};
  Vec3 position{};
  Vec3 axis{1,0,0};
  int side{0}; // -1/+1 for wall pieces, 0 for struts
  std::uint32_t station{0};
  std::uint32_t row{0};
};
struct ShoringPlan {
  Id planId{0};
  Revision sourceRevision{0};
  double stationSpacingUsedM{0};
  std::vector<PlannedShoringPiece> pieces;
  bool complete{false};
  std::vector<std::string> issues;
};
class ShoringPlanner {
 public:
  static Result<ShoringPlan> Build(const TrenchGeometry& g,Revision sourceRevision);
};

enum class ShoringPlanState { Proposed, Validated, Installed, NeedsRevalidation, Rejected, Removed };
enum class ShoringPieceState { Planned, Installed, Damaged, NeedsRevalidation, Removed, Failed };

struct ShoringPieceInstance {
  Id pieceId{0};
  Id planId{0};
  std::uint64_t ordinal{0};
  ShoringPieceKind kind{ShoringPieceKind::Upright};
  Vec3 position{};
  Vec3 axis{1,0,0};
  int side{0};
  std::uint32_t station{0};
  std::uint32_t row{0};
  double health{100};
  double maxHealth{100};
  Revision anchorRevision{0};
  ShoringPieceState state{ShoringPieceState::Planned};
};
struct ShoringAssemblyPlan {
  Id planId{0};
  Revision terrainRevision{0};
  ShoringPlanState state{ShoringPlanState::Proposed};
  bool physicallyInstalled{false};
  std::vector<Id> pieceIds;
};

// Engine-neutral lifecycle model. An Unreal adapter is responsible for actor spawn/physics;
// state changes occur here only after the adapter reports a successful placement.
class ShoringAssembly {
 public:
  Result<bool> RegisterPlan(const ShoringPlan& plan,Id firstPieceId,double maxHealth=100.0);
  Result<bool> ValidatePlan(Id planId,Revision currentTerrainRevision,bool allPlacementsValid);
  Result<bool> InstallPlan(Id planId,Revision currentTerrainRevision);
  void NotifyTerrainRevision(Revision newTerrainRevision);
  Result<bool> RevalidatePlan(Id planId,Revision currentTerrainRevision,bool allAnchorsValid);
  Result<bool> DamagePiece(Id pieceId,double damage);
  Result<bool> RepairPiece(Id pieceId,double amount);
  Result<bool> RemovePiece(Id pieceId);
  Result<ShoringAssemblyPlan> GetPlan(Id planId) const;
  Result<ShoringPieceInstance> GetPiece(Id pieceId) const;
  Revision GetRevision() const noexcept { return revision_; }
 private:
  std::unordered_map<Id,ShoringAssemblyPlan> plans_;
  std::unordered_map<Id,ShoringPieceInstance> pieces_;
  Revision revision_{1};
};
} // namespace wm

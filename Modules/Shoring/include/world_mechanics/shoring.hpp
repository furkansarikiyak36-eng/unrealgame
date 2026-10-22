#pragma once
#include "world_mechanics/core.hpp"
#include <vector>

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
} // namespace wm

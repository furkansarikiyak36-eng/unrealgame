#pragma once
#include "world_mechanics/core.hpp"
#include <unordered_map>

namespace wm {
struct StructureNode {
  Id id{0};
  bool grounded{false};
  bool active{true};
  double demand{0};
  double capacity{1};
  double health{1};
  double maxHealth{1};
  bool collapseLatched{false};
};
struct SupportEdge {
  Id id{0};
  Id supporterId{0};
  Id dependentId{0};
  double capacity{1};
  bool active{true};
};
enum class SupportState { Supported, Overloaded, Unsupported, Inactive };
struct NodeAssessment {
  Id nodeId{0};
  SupportState state{SupportState::Unsupported};
  double aggregateDemand{0};
  double demandCapacityRatio{0};
  bool collapseCandidate{false};
  std::string reason;
};
struct GraphAssessment {
  Revision graphRevision{0};
  std::vector<NodeAssessment> nodes;
};

struct StructuralGraphSnapshot {
  std::uint32_t schemaVersion{1};
  Revision savedGraphRevision{0};
  std::vector<StructureNode> nodes;
  std::vector<SupportEdge> edges;
};

class StructuralGraph {
 public:
  Result<bool> AddNode(const StructureNode& node);
  Result<bool> AddEdge(const SupportEdge& edge);
  Result<bool> RemoveNode(Id nodeId);
  Result<bool> RemoveEdge(Id edgeId);
  Result<bool> SetEdgeActive(Id edgeId,bool active);
  Result<bool> SetNodeDemand(Id nodeId,double demand);
  Result<bool> SetNodeGrounded(Id nodeId,bool grounded);
  Result<bool> SetNodeCapacity(Id nodeId,double capacity);
  Result<bool> ApplyDamage(Id nodeId,double amount);
  Result<bool> Repair(Id nodeId,double amount);
  Result<bool> LatchCollapse(Id nodeId);
  Result<GraphAssessment> Assess() const;
  Result<StructuralGraphSnapshot> CreateSnapshot(
      std::size_t maxNodes=100000,std::size_t maxEdges=500000) const;
  Result<bool> RestoreSnapshot(const StructuralGraphSnapshot& snapshot,
      std::size_t maxNodes=100000,std::size_t maxEdges=500000);
  Revision GetRevision() const noexcept { return revision_; }
 private:
  std::unordered_map<Id,StructureNode> nodes_;
  std::unordered_map<Id,SupportEdge> edges_;
  Revision revision_{1};
};
} // namespace wm

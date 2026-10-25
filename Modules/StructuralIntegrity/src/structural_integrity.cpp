#include "world_mechanics/structural_integrity.hpp"
#include <functional>
#include <queue>
#include <set>

namespace wm {
Result<bool> StructuralGraph::AddNode(const StructureNode& n) {
  if(n.id==0||!std::isfinite(n.demand)||n.demand<0||!std::isfinite(n.capacity)||n.capacity<0||
     !std::isfinite(n.health)||!std::isfinite(n.maxHealth)||n.maxHealth<=0||n.health<0||n.health>n.maxHealth)
    return Result<bool>::Failure(ErrorCode::InvalidArgument,"invalid structure node");
  if(nodes_.count(n.id)) return Result<bool>::Failure(ErrorCode::Conflict,"duplicate structure node ID");
  nodes_.emplace(n.id,n); ++revision_; return Result<bool>::Success(true);
}
Result<bool> StructuralGraph::AddEdge(const SupportEdge& e) {
  if(e.id==0||e.supporterId==0||e.dependentId==0||e.supporterId==e.dependentId||
     !std::isfinite(e.capacity)||e.capacity<0)
    return Result<bool>::Failure(ErrorCode::InvalidArgument,"invalid support edge");
  if(edges_.count(e.id)) return Result<bool>::Failure(ErrorCode::Conflict,"duplicate support edge ID");
  if(!nodes_.count(e.supporterId)||!nodes_.count(e.dependentId))
    return Result<bool>::Failure(ErrorCode::NotFound,"support edge endpoint does not exist");
  edges_.emplace(e.id,e); ++revision_; return Result<bool>::Success(true);
}
Result<bool> StructuralGraph::RemoveNode(Id id) {
  if(!nodes_.erase(id)) return Result<bool>::Failure(ErrorCode::NotFound,"structure node does not exist");
  for(auto it=edges_.begin();it!=edges_.end();) {
    if(it->second.supporterId==id||it->second.dependentId==id) it=edges_.erase(it); else ++it;
  }
  ++revision_; return Result<bool>::Success(true);
}
Result<bool> StructuralGraph::RemoveEdge(Id edgeId) {
  if(!edges_.erase(edgeId))
    return Result<bool>::Failure(ErrorCode::NotFound,"support edge does not exist");
  ++revision_;
  return Result<bool>::Success(true);
}
Result<bool> StructuralGraph::SetEdgeActive(Id edgeId,bool active) {
  auto it=edges_.find(edgeId);
  if(it==edges_.end())
    return Result<bool>::Failure(ErrorCode::NotFound,"support edge does not exist");
  if(it->second.active==active) return Result<bool>::Success(false);
  it->second.active=active;
  ++revision_;
  return Result<bool>::Success(true);
}
Result<bool> StructuralGraph::SetNodeDemand(Id nodeId,double demand) {
  if(!std::isfinite(demand)||demand<0)
    return Result<bool>::Failure(ErrorCode::InvalidArgument,"node demand must be finite and nonnegative");
  auto it=nodes_.find(nodeId);
  if(it==nodes_.end())
    return Result<bool>::Failure(ErrorCode::NotFound,"structure node does not exist");
  if(it->second.demand==demand) return Result<bool>::Success(false);
  it->second.demand=demand;
  ++revision_;
  return Result<bool>::Success(true);
}
Result<bool> StructuralGraph::SetNodeGrounded(Id nodeId,bool grounded) {
  auto it=nodes_.find(nodeId);
  if(it==nodes_.end())
    return Result<bool>::Failure(ErrorCode::NotFound,"structure node does not exist");
  if(it->second.grounded==grounded) return Result<bool>::Success(false);
  it->second.grounded=grounded;
  ++revision_;
  return Result<bool>::Success(true);
}
Result<bool> StructuralGraph::SetNodeCapacity(Id nodeId,double capacity) {
  if(!std::isfinite(capacity)||capacity<0)
    return Result<bool>::Failure(ErrorCode::InvalidArgument,"node capacity must be finite and nonnegative");
  auto it=nodes_.find(nodeId);
  if(it==nodes_.end())
    return Result<bool>::Failure(ErrorCode::NotFound,"structure node does not exist");
  if(it->second.capacity==capacity) return Result<bool>::Success(false);
  it->second.capacity=capacity;
  ++revision_;
  return Result<bool>::Success(true);
}
Result<bool> StructuralGraph::ApplyDamage(Id id,double amount) {
  if(!std::isfinite(amount)||amount<=0) return Result<bool>::Failure(ErrorCode::InvalidArgument,"damage must be positive and finite");
  auto it=nodes_.find(id); if(it==nodes_.end()) return Result<bool>::Failure(ErrorCode::NotFound,"structure node does not exist");
  it->second.health=std::max(0.0,it->second.health-amount);
  if(it->second.health<=0) it->second.active=false;
  ++revision_; return Result<bool>::Success(true);
}
Result<bool> StructuralGraph::Repair(Id id,double amount) {
  if(!std::isfinite(amount)||amount<=0) return Result<bool>::Failure(ErrorCode::InvalidArgument,"repair amount must be positive and finite");
  auto it=nodes_.find(id); if(it==nodes_.end()) return Result<bool>::Failure(ErrorCode::NotFound,"structure node does not exist");
  if(it->second.collapseLatched) return Result<bool>::Failure(ErrorCode::Unsupported,"collapsed node requires replacement, not simple repair");
  it->second.health=std::min(it->second.maxHealth,it->second.health+amount);
  if(it->second.health>0) it->second.active=true;
  ++revision_; return Result<bool>::Success(true);
}
Result<bool> StructuralGraph::LatchCollapse(Id id) {
  auto it=nodes_.find(id); if(it==nodes_.end()) return Result<bool>::Failure(ErrorCode::NotFound,"structure node does not exist");
  if(it->second.collapseLatched) return Result<bool>::Success(false);
  it->second.collapseLatched=true; it->second.active=false; ++revision_; return Result<bool>::Success(true);
}
Result<GraphAssessment> StructuralGraph::Assess() const {
  std::unordered_map<Id,std::uint32_t> indegree;
  std::unordered_map<Id,std::vector<const SupportEdge*>> outgoing,incoming;
  std::unordered_map<Id,double> aggregateDemand;

  for(const auto& pair:nodes_) {
    indegree[pair.first]=0;
    aggregateDemand[pair.first]=pair.second.demand;
  }
  for(const auto& pair:edges_) {
    const auto& e=pair.second;
    if(!nodes_.count(e.supporterId)||!nodes_.count(e.dependentId))
      return Result<GraphAssessment>::Failure(ErrorCode::CorruptData,"support graph contains dangling reference");
    if(e.active) {
      outgoing[e.supporterId].push_back(&e);
      incoming[e.dependentId].push_back(&e);
      ++indegree[e.dependentId];
    }
  }

  // Stable edge order makes load distribution and its floating-point summation
  // deterministic even though the backing node/edge registries are unordered.
  auto byEdgeId=[](const SupportEdge* a,const SupportEdge* b){return a->id<b->id;};
  for(auto& pair:outgoing) std::sort(pair.second.begin(),pair.second.end(),byEdgeId);
  for(auto& pair:incoming) std::sort(pair.second.begin(),pair.second.end(),byEdgeId);

  std::priority_queue<Id,std::vector<Id>,std::greater<Id>> ready;
  for(const auto& item:indegree) if(item.second==0) ready.push(item.first);
  std::vector<Id> order; order.reserve(nodes_.size());
  while(!ready.empty()) {
    const Id id=ready.top(); ready.pop(); order.push_back(id);
    for(const auto* e:outgoing[id]) if(--indegree[e->dependentId]==0) ready.push(e->dependentId);
  }
  if(order.size()!=nodes_.size())
    return Result<GraphAssessment>::Failure(ErrorCode::Conflict,"active support graph contains a cycle");

  // Load pass: send each dependent's total load upstream in equal shares over
  // its active support edges. This is an explicit conservative game model,
  // not an engineering stiffness solver. A single-support parent receives the
  // full load; multiple supports split the incoming load deterministically.
  std::unordered_map<Id,double> loadShareByEdge;
  for(auto it=order.rbegin();it!=order.rend();++it) {
    const Id dependentId=*it;
    const auto found=incoming.find(dependentId);
    if(found==incoming.end()||found->second.empty()) continue;
    const double share=aggregateDemand[dependentId]/static_cast<double>(found->second.size());
    if(!std::isfinite(share)||share<0)
      return Result<GraphAssessment>::Failure(ErrorCode::CorruptData,"aggregate support load overflowed");
    for(const SupportEdge* edge:found->second) {
      loadShareByEdge[edge->id]=share;
      double& parentLoad=aggregateDemand[edge->supporterId];
      parentLoad+=share;
      if(!std::isfinite(parentLoad))
        return Result<GraphAssessment>::Failure(ErrorCode::CorruptData,"aggregate support load overflowed");
    }
  }

  GraphAssessment assessment;
  assessment.graphRevision=revision_;
  std::unordered_map<Id,NodeAssessment> states;
  for(const Id id:order) {
    const auto& node=nodes_.at(id);
    NodeAssessment result;
    result.nodeId=id;
    result.aggregateDemand=aggregateDemand[id];

    if(!node.active||node.collapseLatched||node.health<=0) {
      result.state=SupportState::Inactive;
      result.reason="node inactive, destroyed or collapse-latched";
    } else {
      const double healthFraction=node.maxHealth>0?node.health/node.maxHealth:0;
      const double effectiveCapacity=std::max(0.0,node.capacity*healthFraction);
      result.demandCapacityRatio=effectiveCapacity>1e-12
        ? result.aggregateDemand/effectiveCapacity
        : (result.aggregateDemand>0?std::numeric_limits<double>::infinity():0.0);

      bool supported=node.grounded;
      const bool overloaded=result.aggregateDemand>effectiveCapacity;
      if(!node.grounded) {
        const auto parentEdges=incoming.find(id);
        if(parentEdges!=incoming.end()) {
          for(const SupportEdge* edge:parentEdges->second) {
            const auto parentState=states.find(edge->supporterId);
            const auto share=loadShareByEdge.find(edge->id);
            if(parentState!=states.end()&&share!=loadShareByEdge.end()&&
               parentState->second.state==SupportState::Supported&&
               edge->capacity>=share->second) {
              supported=true;
              break;
            }
          }
        }
      }

      if(!supported) {
        result.state=SupportState::Unsupported;
        result.reason="no valid active path to grounded support";
      } else if(overloaded) {
        result.state=SupportState::Overloaded;
        result.reason="aggregate dependent load exceeds effective capacity";
      } else {
        result.state=SupportState::Supported;
        result.reason="valid support path and aggregate load within capacity";
      }
    }
    result.collapseCandidate=(result.state==SupportState::Unsupported||result.state==SupportState::Overloaded)&&!node.collapseLatched;
    states[id]=result;
    assessment.nodes.push_back(result);
  }
  return Result<GraphAssessment>::Success(std::move(assessment));
}
} // namespace wm

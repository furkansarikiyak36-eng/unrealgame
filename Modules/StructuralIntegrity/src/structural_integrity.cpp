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
  for(const auto& pair:nodes_) indegree[pair.first]=0;
  for(const auto& pair:edges_) {
    const auto& e=pair.second;
    if(!nodes_.count(e.supporterId)||!nodes_.count(e.dependentId))
      return Result<GraphAssessment>::Failure(ErrorCode::CorruptData,"support graph contains dangling reference");
    if(e.active) { outgoing[e.supporterId].push_back(&e); incoming[e.dependentId].push_back(&e); ++indegree[e.dependentId]; }
  }
  std::priority_queue<Id,std::vector<Id>,std::greater<Id>> ready;
  for(const auto& item:indegree) if(item.second==0) ready.push(item.first);
  std::vector<Id> order; order.reserve(nodes_.size());
  while(!ready.empty()) {
    Id id=ready.top(); ready.pop(); order.push_back(id);
    for(const auto* e:outgoing[id]) if(--indegree[e->dependentId]==0) ready.push(e->dependentId);
  }
  if(order.size()!=nodes_.size())
    return Result<GraphAssessment>::Failure(ErrorCode::Conflict,"active support graph contains a cycle");
  GraphAssessment a; a.graphRevision=revision_;
  std::unordered_map<Id,NodeAssessment> states;
  for(Id id:order) {
    const auto& n=nodes_.at(id); NodeAssessment result; result.nodeId=id;
    if(!n.active||n.collapseLatched||n.health<=0) {
      result.state=SupportState::Inactive; result.reason="node inactive, destroyed or collapse-latched";
    } else {
      const double fraction=n.maxHealth>0?n.health/n.maxHealth:0;
      const double effective=std::max(0.0,n.capacity*fraction);
      result.demandCapacityRatio=effective>1e-12?n.demand/effective:(n.demand>0?std::numeric_limits<double>::infinity():0);
      bool supported=n.grounded;
      const bool overloaded=n.demand>effective;
      if(!n.grounded) {
        for(const auto* e:incoming[id]) {
          const auto parent=states.find(e->supporterId);
          const auto& parentNode=nodes_.at(e->supporterId);
          if(parent!=states.end()&&parent->second.state==SupportState::Supported&&
             e->capacity>=n.demand&&parentNode.capacity*(parentNode.health/parentNode.maxHealth)>=n.demand) {
            supported=true; break;
          }
        }
      }
      if(!supported) { result.state=SupportState::Unsupported; result.reason="no valid active path to grounded support"; }
      else if(overloaded) { result.state=SupportState::Overloaded; result.reason="demand exceeds effective capacity"; }
      else { result.state=SupportState::Supported; result.reason="valid support path and demand within capacity"; }
    }
    result.collapseCandidate=(result.state==SupportState::Unsupported||result.state==SupportState::Overloaded)&&!n.collapseLatched;
    states[id]=result; a.nodes.push_back(result);
  }
  return Result<GraphAssessment>::Success(std::move(a));
}
} // namespace wm

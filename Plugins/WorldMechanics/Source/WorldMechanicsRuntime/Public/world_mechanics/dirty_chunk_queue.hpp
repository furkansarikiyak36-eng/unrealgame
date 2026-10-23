#pragma once
#include "world_mechanics/core.hpp"
#include <map>
#include <vector>

namespace wm {
struct DirtyChunkWork {
  ChunkCoord chunk{};
  Revision densityRevision{0};
  std::uint32_t priority{0};
};

class DirtyChunkQueue {
 public:
  explicit DirtyChunkQueue(std::size_t capacity=32):capacity_(capacity) {}
  Result<bool> Enqueue(const DirtyChunkWork& work) {
    if(capacity_==0||work.densityRevision==0)
      return Result<bool>::Failure(ErrorCode::InvalidArgument,"queue capacity and density revision must be nonzero");
    auto found=work_.find(work.chunk);
    if(found!=work_.end()) {
      if(work.densityRevision<found->second.densityRevision)
        return Result<bool>::Success(false); // stale request is safely ignored
      if(work.densityRevision>found->second.densityRevision) found->second.densityRevision=work.densityRevision;
      found->second.priority=std::max(found->second.priority,work.priority);
      return Result<bool>::Success(true);
    }
    if(work_.size()>=capacity_)
      return Result<bool>::Failure(ErrorCode::BudgetExceeded,"dirty chunk queue is full");
    work_.emplace(work.chunk,work);
    return Result<bool>::Success(true);
  }
  std::vector<DirtyChunkWork> Pop(std::size_t maximum) {
    std::vector<DirtyChunkWork> ordered;
    if(maximum==0||work_.empty()) return ordered;
    ordered.reserve(work_.size());
    for(const auto& item:work_) ordered.push_back(item.second);
    std::sort(ordered.begin(),ordered.end(),[](const auto& a,const auto& b){
      if(a.priority!=b.priority) return a.priority>b.priority;
      if(a.densityRevision!=b.densityRevision) return a.densityRevision>b.densityRevision;
      return a.chunk<b.chunk;
    });
    if(ordered.size()>maximum) ordered.resize(maximum);
    for(const auto& item:ordered) work_.erase(item.chunk);
    return ordered;
  }
  bool Erase(const ChunkCoord& chunk) { return work_.erase(chunk)>0; }
  void Clear() noexcept { work_.clear(); }
  std::size_t Size() const noexcept { return work_.size(); }
  std::size_t Capacity() const noexcept { return capacity_; }
 private:
  std::size_t capacity_{32};
  std::map<ChunkCoord,DirtyChunkWork> work_;
};
} // namespace wm

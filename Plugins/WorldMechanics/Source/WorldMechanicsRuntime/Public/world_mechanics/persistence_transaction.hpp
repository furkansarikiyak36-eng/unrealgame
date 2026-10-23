#pragma once
#include "world_mechanics/core.hpp"
#include <vector>

namespace wm {
class IRestoreParticipant {
 public:
  virtual ~IRestoreParticipant()=default;
  virtual Result<bool> Preflight()=0;
  virtual Result<bool> Stage()=0;
  virtual Result<bool> Commit()=0;
  virtual void Rollback() noexcept=0;
  virtual void Discard() noexcept=0;
};

class RestoreTransaction {
 public:
  Result<bool> Execute(const std::vector<IRestoreParticipant*>& participants) const {
    for(auto* p:participants) {
      if(!p) return Result<bool>::Failure(ErrorCode::MissingDependency,"null restore participant");
      auto r=p->Preflight();
      if(!r) return Result<bool>::Failure(r.error,"preflight failed: "+r.message);
    }
    std::size_t staged=0;
    for(auto* p:participants) {
      auto r=p->Stage();
      if(!r) {
        for(std::size_t i=staged;i>0;--i) participants[i-1]->Discard();
        p->Discard();
        return Result<bool>::Failure(r.error,"staging failed: "+r.message);
      }
      ++staged;
    }
    for(std::size_t i=0;i<participants.size();++i) {
      auto r=participants[i]->Commit();
      if(!r) {
        // Roll back every staged participant in reverse order, including the participant
        // whose Commit reported failure (it may have mutated before detecting the error).
        for(std::size_t j=staged;j>0;--j) participants[j-1]->Rollback();
        for(auto* p:participants) p->Discard();
        return Result<bool>::Failure(r.error,"commit failed: "+r.message);
      }
    }
    for(auto* p:participants) p->Discard();
    return Result<bool>::Success(true);
  }
};
} // namespace wm

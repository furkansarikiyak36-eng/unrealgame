#pragma once

#include "world_mechanics/core.hpp"

namespace wm {

enum class TrapTriggerSource {
  Pressure,
  TripWire,
  Proximity,
  Manual
};

enum class TrapLifecycleState {
  Disarmed,
  Armed,
  CoolingDown,
  Consumed
};

struct TrapLifecycleConfig {
  Id trapId{0};
  TrapTriggerSource triggerSource{TrapTriggerSource::Pressure};
  double cooldownSeconds{5.0};
  double minimumRearmDelaySeconds{0.01};
  double damage{40.0};
  bool consumeOnTrigger{false};
  double maximumCooldownSeconds{3600.0};
};

struct TrapLifecycleSnapshot {
  Id trapId{0};
  Revision stateRevision{1};
  TrapLifecycleState state{TrapLifecycleState::Armed};
  Id lastEventId{0};
  Id lastInstigatorId{0};
  Seconds lastTriggerTime{-1.0};
  Seconds cooldownEndsAt{0.0};
  std::uint64_t triggerCount{0};
};

struct TrapTriggerRequest {
  Id eventId{0};  // Must increase monotonically for this trap to make retries idempotent.
  Id instigatorId{0};
  TrapTriggerSource source{TrapTriggerSource::Pressure};
  Seconds nowSeconds{0.0};
  Revision expectedStateRevision{0};
};

struct TrapTriggerDecision {
  Id trapId{0};
  Id eventId{0};
  Id instigatorId{0};
  double damage{0.0};
  bool applyDamage{false};
  TrapLifecycleState resultingState{TrapLifecycleState::Armed};
  Revision resultingRevision{0};
  Seconds cooldownEndsAt{0.0};
  std::string reason;
};

// Authoritative portable state machine. The Unreal adapter applies damage, audio,
// particles and physical impulses only after a successful trigger decision.
class TrapLifecycle {
 public:
  explicit TrapLifecycle(TrapLifecycleConfig config);

  Result<bool> Arm(Revision expectedStateRevision);
  Result<bool> Disarm(Revision expectedStateRevision);
  Result<TrapTriggerDecision> TryTrigger(const TrapTriggerRequest& request);
  Result<bool> Advance(Seconds nowSeconds, Revision expectedStateRevision);

  TrapLifecycleSnapshot Snapshot() const;
  Revision GetRevision() const noexcept { return stateRevision_; }

 private:
  bool ConfigValid() const noexcept;

  TrapLifecycleConfig config_;
  Revision stateRevision_{1};
  TrapLifecycleState state_{TrapLifecycleState::Armed};
  Id lastEventId_{0};
  Id lastInstigatorId_{0};
  Seconds lastTriggerTime_{-1.0};
  Seconds cooldownEndsAt_{0.0};
  std::uint64_t triggerCount_{0};
};

}  // namespace wm

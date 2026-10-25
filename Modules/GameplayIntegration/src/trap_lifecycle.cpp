#include "world_mechanics/trap_lifecycle.hpp"

#include <algorithm>
#include <cmath>

namespace wm {

TrapLifecycle::TrapLifecycle(TrapLifecycleConfig config)
    : config_(std::move(config)) {}

bool TrapLifecycle::ConfigValid() const noexcept {
  const bool validSource =
      config_.triggerSource == TrapTriggerSource::Pressure ||
      config_.triggerSource == TrapTriggerSource::TripWire ||
      config_.triggerSource == TrapTriggerSource::Proximity ||
      config_.triggerSource == TrapTriggerSource::Manual;
  return config_.trapId != 0 && validSource &&
         std::isfinite(config_.cooldownSeconds) && config_.cooldownSeconds >= 0.0 &&
         std::isfinite(config_.minimumRearmDelaySeconds) &&
         config_.minimumRearmDelaySeconds > 0.0 &&
         std::isfinite(config_.damage) && config_.damage >= 0.0 &&
         std::isfinite(config_.maximumCooldownSeconds) &&
         config_.maximumCooldownSeconds > 0.0 &&
         config_.cooldownSeconds <= config_.maximumCooldownSeconds &&
         config_.minimumRearmDelaySeconds <= config_.maximumCooldownSeconds;
}

Result<bool> TrapLifecycle::Arm(Revision expectedStateRevision) {
  if (!ConfigValid()) {
    return Result<bool>::Failure(ErrorCode::InvalidArgument, "trap configuration is invalid");
  }
  if (expectedStateRevision != stateRevision_) {
    return Result<bool>::Failure(ErrorCode::StaleRevision, "trap changed before arm request");
  }
  if (state_ == TrapLifecycleState::Consumed) {
    return Result<bool>::Failure(ErrorCode::Unsupported, "consumed trap cannot be armed again");
  }
  if (state_ == TrapLifecycleState::Armed) {
    return Result<bool>::Success(false);
  }
  if (state_ == TrapLifecycleState::CoolingDown) {
    return Result<bool>::Failure(ErrorCode::Conflict, "cooling trap must finish cooldown before rearming");
  }
  state_ = TrapLifecycleState::Armed;
  ++stateRevision_;
  return Result<bool>::Success(true);
}

Result<bool> TrapLifecycle::Disarm(Revision expectedStateRevision) {
  if (expectedStateRevision != stateRevision_) {
    return Result<bool>::Failure(ErrorCode::StaleRevision, "trap changed before disarm request");
  }
  if (state_ == TrapLifecycleState::Consumed) {
    return Result<bool>::Failure(ErrorCode::Unsupported, "consumed trap cannot be disarmed");
  }
  if (state_ == TrapLifecycleState::Disarmed) {
    return Result<bool>::Success(false);
  }
  state_ = TrapLifecycleState::Disarmed;
  ++stateRevision_;
  return Result<bool>::Success(true);
}

Result<TrapTriggerDecision> TrapLifecycle::TryTrigger(
    const TrapTriggerRequest& request) {
  if (!ConfigValid() || request.eventId == 0 ||
      !std::isfinite(request.nowSeconds) || request.nowSeconds < 0.0) {
    return Result<TrapTriggerDecision>::Failure(
        ErrorCode::InvalidArgument, "invalid trap config, event ID or event time");
  }
  if (request.expectedStateRevision != stateRevision_) {
    return Result<TrapTriggerDecision>::Failure(
        ErrorCode::StaleRevision, "trap changed after the trigger observation");
  }
  if (request.eventId <= lastEventId_) {
    return Result<TrapTriggerDecision>::Failure(
        ErrorCode::Conflict, "duplicate or out-of-order trigger event");
  }
  if (request.source != config_.triggerSource) {
    return Result<TrapTriggerDecision>::Failure(
        ErrorCode::Unsupported, "trigger source does not match this trap");
  }
  if (state_ != TrapLifecycleState::Armed) {
    return Result<TrapTriggerDecision>::Failure(
        ErrorCode::Conflict, "trap is not armed");
  }
  if (lastTriggerTime_ >= 0.0 && request.nowSeconds < lastTriggerTime_) {
    return Result<TrapTriggerDecision>::Failure(
        ErrorCode::StaleRevision, "trigger time moved backwards");
  }

  const double effectiveCooldown =
      std::max(config_.cooldownSeconds, config_.minimumRearmDelaySeconds);
  const double nextCooldownEnd = request.nowSeconds + effectiveCooldown;
  if (!std::isfinite(nextCooldownEnd)) {
    return Result<TrapTriggerDecision>::Failure(
        ErrorCode::OutOfRange, "trap cooldown end time overflowed");
  }

  // The event is consumed once. An adapter should key side effects by eventId as well.
  lastEventId_ = request.eventId;
  lastInstigatorId_ = request.instigatorId;
  lastTriggerTime_ = request.nowSeconds;
  cooldownEndsAt_ = nextCooldownEnd;
  ++triggerCount_;
  state_ = config_.consumeOnTrigger ? TrapLifecycleState::Consumed
                                    : TrapLifecycleState::CoolingDown;
  ++stateRevision_;

  TrapTriggerDecision decision;
  decision.trapId = config_.trapId;
  decision.eventId = request.eventId;
  decision.instigatorId = request.instigatorId;
  decision.damage = config_.damage;
  decision.applyDamage = request.instigatorId != 0 && config_.damage > 0.0;
  decision.resultingState = state_;
  decision.resultingRevision = stateRevision_;
  decision.cooldownEndsAt = cooldownEndsAt_;
  decision.reason = config_.consumeOnTrigger
      ? "trigger accepted; trap consumed"
      : "trigger accepted; cooldown started";
  return Result<TrapTriggerDecision>::Success(std::move(decision));
}

Result<bool> TrapLifecycle::Advance(
    Seconds nowSeconds, Revision expectedStateRevision) {
  if (!ConfigValid() || !std::isfinite(nowSeconds) || nowSeconds < 0.0) {
    return Result<bool>::Failure(ErrorCode::InvalidArgument, "invalid trap config or advance time");
  }
  if (expectedStateRevision != stateRevision_) {
    return Result<bool>::Failure(ErrorCode::StaleRevision, "trap changed before cooldown advance");
  }
  if (lastTriggerTime_ >= 0.0 && nowSeconds < lastTriggerTime_) {
    return Result<bool>::Failure(ErrorCode::StaleRevision, "trap advance time moved backwards");
  }
  if (state_ != TrapLifecycleState::CoolingDown || nowSeconds < cooldownEndsAt_) {
    return Result<bool>::Success(false);
  }
  state_ = TrapLifecycleState::Armed;
  ++stateRevision_;
  return Result<bool>::Success(true);
}

TrapLifecycleSnapshot TrapLifecycle::Snapshot() const {
  TrapLifecycleSnapshot snapshot;
  snapshot.trapId = config_.trapId;
  snapshot.stateRevision = stateRevision_;
  snapshot.state = state_;
  snapshot.lastEventId = lastEventId_;
  snapshot.lastInstigatorId = lastInstigatorId_;
  snapshot.lastTriggerTime = lastTriggerTime_;
  snapshot.cooldownEndsAt = cooldownEndsAt_;
  snapshot.triggerCount = triggerCount_;
  return snapshot;
}

}  // namespace wm

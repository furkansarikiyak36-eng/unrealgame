#include "world_mechanics/trap_lifecycle.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

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
  if (!ConfigValid()) {
    return Result<bool>::Failure(ErrorCode::InvalidArgument, "trap configuration is invalid");
  }
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
  if (hasTriggered_ && request.nowSeconds < lastTriggerTime_) {
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
  hasTriggered_ = true;
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
  if (hasTriggered_ && nowSeconds < lastTriggerTime_) {
    return Result<bool>::Failure(ErrorCode::StaleRevision, "trap advance time moved backwards");
  }
  if (state_ != TrapLifecycleState::CoolingDown || nowSeconds < cooldownEndsAt_) {
    return Result<bool>::Success(false);
  }
  state_ = TrapLifecycleState::Armed;
  ++stateRevision_;
  return Result<bool>::Success(true);
}

Result<TrapLifecycleSaveRecord> TrapLifecycle::CreateSaveRecord(
    Seconds nowSeconds) const {
  if (!ConfigValid() || !std::isfinite(nowSeconds) || nowSeconds < 0.0) {
    return Result<TrapLifecycleSaveRecord>::Failure(
        ErrorCode::InvalidArgument, "invalid trap configuration or save time");
  }
  if (hasTriggered_ && nowSeconds < lastTriggerTime_) {
    return Result<TrapLifecycleSaveRecord>::Failure(
        ErrorCode::StaleRevision, "save time precedes the last trap trigger");
  }

  TrapLifecycleSaveRecord record;
  record.schemaVersion = 1;
  record.trapId = config_.trapId;
  record.savedStateRevision = stateRevision_;
  record.state = state_;
  record.lastEventId = lastEventId_;
  record.lastInstigatorId = lastInstigatorId_;
  record.hasTriggered = hasTriggered_;
  record.secondsSinceLastTrigger = hasTriggered_ ? nowSeconds - lastTriggerTime_ : 0.0;
  record.remainingCooldownSeconds = state_ == TrapLifecycleState::CoolingDown
      ? std::max(0.0, cooldownEndsAt_ - nowSeconds)
      : 0.0;
  record.triggerCount = triggerCount_;
  if (!std::isfinite(record.secondsSinceLastTrigger) ||
      !std::isfinite(record.remainingCooldownSeconds) ||
      record.remainingCooldownSeconds > config_.maximumCooldownSeconds) {
    return Result<TrapLifecycleSaveRecord>::Failure(
        ErrorCode::CorruptData, "trap state cannot be represented as a bounded save record");
  }
  return Result<TrapLifecycleSaveRecord>::Success(record);
}

Result<bool> TrapLifecycle::RestoreFromSaveRecord(
    const TrapLifecycleSaveRecord& record, Seconds newWorldTimeSeconds) {
  if (!ConfigValid() || !std::isfinite(newWorldTimeSeconds) || newWorldTimeSeconds < 0.0) {
    return Result<bool>::Failure(
        ErrorCode::InvalidArgument, "invalid trap configuration or restore time");
  }
  if (record.schemaVersion != 1) {
    return Result<bool>::Failure(ErrorCode::Unsupported, "unsupported trap save schema");
  }
  const bool validState =
      record.state == TrapLifecycleState::Disarmed ||
      record.state == TrapLifecycleState::Armed ||
      record.state == TrapLifecycleState::CoolingDown ||
      record.state == TrapLifecycleState::Consumed;
  if (record.trapId == 0 || record.trapId != config_.trapId ||
      record.savedStateRevision == 0 || !validState ||
      !std::isfinite(record.secondsSinceLastTrigger) ||
      record.secondsSinceLastTrigger < 0.0 ||
      !std::isfinite(record.remainingCooldownSeconds) ||
      record.remainingCooldownSeconds < 0.0 ||
      record.remainingCooldownSeconds > config_.maximumCooldownSeconds) {
    return Result<bool>::Failure(ErrorCode::CorruptData, "trap save record contains invalid identity, state or time");
  }
  if (record.hasTriggered) {
    if (record.lastEventId == 0 || record.triggerCount == 0) {
      return Result<bool>::Failure(ErrorCode::CorruptData, "trigger history is incomplete");
    }
  } else if (record.lastEventId != 0 || record.triggerCount != 0 ||
             record.lastInstigatorId != 0 || record.secondsSinceLastTrigger != 0.0) {
    return Result<bool>::Failure(ErrorCode::CorruptData, "untriggered trap contains trigger history");
  }
  if ((record.state == TrapLifecycleState::Consumed && !config_.consumeOnTrigger) ||
      (record.state == TrapLifecycleState::CoolingDown && config_.consumeOnTrigger) ||
      (record.state != TrapLifecycleState::CoolingDown &&
       record.remainingCooldownSeconds > 1e-9) ||
      (record.state == TrapLifecycleState::Consumed && !record.hasTriggered)) {
    return Result<bool>::Failure(ErrorCode::CorruptData, "trap save state conflicts with current trap policy");
  }

  const Revision baseRevision = std::max(stateRevision_, record.savedStateRevision);
  if (baseRevision == std::numeric_limits<Revision>::max()) {
    return Result<bool>::Failure(ErrorCode::OutOfRange, "trap revision cannot advance after restore");
  }
  const double restoredLastTriggerTime = record.hasTriggered
      ? newWorldTimeSeconds - record.secondsSinceLastTrigger
      : -1.0;
  const double restoredCooldownEnd = newWorldTimeSeconds + record.remainingCooldownSeconds;
  if (!std::isfinite(restoredLastTriggerTime) || !std::isfinite(restoredCooldownEnd)) {
    return Result<bool>::Failure(ErrorCode::OutOfRange, "restored trap time overflowed");
  }

  // Use remaining duration, not a saved absolute world clock, because play sessions
  // may restart at time zero or resume under a different clock origin.
  state_ = record.state;
  lastEventId_ = record.lastEventId;
  lastInstigatorId_ = record.lastInstigatorId;
  hasTriggered_ = record.hasTriggered;
  lastTriggerTime_ = restoredLastTriggerTime;
  cooldownEndsAt_ = restoredCooldownEnd;
  triggerCount_ = record.triggerCount;
  stateRevision_ = baseRevision + 1;
  return Result<bool>::Success(true);
}

TrapLifecycleSnapshot TrapLifecycle::Snapshot() const {
  TrapLifecycleSnapshot snapshot;
  snapshot.trapId = config_.trapId;
  snapshot.stateRevision = stateRevision_;
  snapshot.state = state_;
  snapshot.lastEventId = lastEventId_;
  snapshot.lastInstigatorId = lastInstigatorId_;
  snapshot.hasTriggered = hasTriggered_;
  snapshot.lastTriggerTime = lastTriggerTime_;
  snapshot.cooldownEndsAt = cooldownEndsAt_;
  snapshot.triggerCount = triggerCount_;
  return snapshot;
}

}  // namespace wm

#include "world_mechanics/survival.hpp"
#include <cmath>

namespace wm {
Result<double> SurvivalExposure::EffortCost(EffortKind kind,double intensity) const {
  if(!std::isfinite(intensity)||intensity<0||intensity>100)
    return Result<double>::Failure(ErrorCode::InvalidArgument,"effort intensity must be finite and within [0,100]");
  double base=0;
  switch(kind) {
    case EffortKind::ToolSwing: base=costs_.toolSwing; break;
    case EffortKind::HeavyExcavation: base=costs_.heavyExcavation; break;
    case EffortKind::ShoringPlacement: base=costs_.shoringPlacement; break;
    case EffortKind::ShoringCarry: base=costs_.shoringCarry; break;
    case EffortKind::ShoringRepair: base=costs_.shoringRepair; break;
    case EffortKind::HeavyCarry: base=costs_.heavyCarry; break;
  }
  if(!std::isfinite(base)||base<0) return Result<double>::Failure(ErrorCode::CorruptData,"effort cost table contains invalid value");
  return Result<double>::Success(base*intensity);
}
Result<ExposureState> SurvivalExposure::Update(const ExposureState& current,const EnvironmentSample& sample,
                                                double deltaSeconds) const {
  if(!std::isfinite(deltaSeconds)||deltaSeconds<0)
    return Result<ExposureState>::Failure(ErrorCode::InvalidArgument,"deltaSeconds must be finite and nonnegative");
  if(!std::isfinite(current.thermalStress)||!std::isfinite(current.wetness)||
     !std::isfinite(current.darknessExposure)||!std::isfinite(current.waterExposure)||
     !std::isfinite(current.lastUpdateSeconds)||current.wetness<0||current.wetness>1)
    return Result<ExposureState>::Failure(ErrorCode::CorruptData,"current exposure state is invalid");
  std::string why;
  if(!sample.Validate(&why)) return Result<ExposureState>::Failure(ErrorCode::StaleRevision,why);
  ExposureState next=current;
  const double dt=std::min(deltaSeconds,maxStepSeconds_);
  if(sample.wetness01) {
    const double wetRate=std::min(1.0,dt*0.8);
    next.wetness=Clamp01(next.wetness+(*sample.wetness01-next.wetness)*wetRate);
  }
  if(sample.waterDepthM) {
    const double waterSignal=Clamp01(*sample.waterDepthM/0.25);
    const double wetRate=std::min(1.0,dt*1.2);
    next.wetness=Clamp01(next.wetness+(waterSignal-next.wetness)*wetRate);
    next.waterExposure=std::max(0.0,next.waterExposure+waterSignal*dt);
  }
  if(sample.temperatureC) {
    double effectiveTemp=*sample.temperatureC;
    if(sample.shelterHeatC) effectiveTemp+=*sample.shelterHeatC;
    const double cold=std::max(0.0,15.0-effectiveTemp);
    const double hot=std::max(0.0,effectiveTemp-32.0);
    next.thermalStress=std::max(0.0,next.thermalStress+(cold*(1.0+next.wetness)+hot)*dt*0.05);
  }
  if(sample.light01&&sample.underground) {
    if(*sample.underground) next.darknessExposure=std::max(0.0,next.darknessExposure+(1.0-*sample.light01)*dt);
    else next.darknessExposure=std::max(0.0,next.darknessExposure-0.5*dt);
  }
  next.lastUpdateSeconds=std::max(current.lastUpdateSeconds,sample.sampledAtSeconds);
  if(!std::isfinite(next.thermalStress)||!std::isfinite(next.waterExposure)||!std::isfinite(next.darknessExposure))
    return Result<ExposureState>::Failure(ErrorCode::CorruptData,"exposure calculation overflowed");
  return Result<ExposureState>::Success(next);
}
} // namespace wm

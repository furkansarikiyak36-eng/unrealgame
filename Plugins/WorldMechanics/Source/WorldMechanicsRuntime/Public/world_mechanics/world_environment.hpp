#pragma once
#include "world_mechanics/core.hpp"
#include <optional>

namespace wm {
struct EnvironmentSample {
  Revision sourceRevision{0};
  double sampledAtSeconds{0};
  bool valid{false};
  bool stale{false};
  std::optional<double> temperatureC;
  std::optional<double> wetness01;
  std::optional<double> light01;
  std::optional<double> waterDepthM;
  std::optional<double> undergroundDepthM;
  std::optional<double> shelterHeatC;
  std::optional<bool> underground;
  bool Validate(std::string* reason=nullptr) const {
    auto fail=[&](const char* m){if(reason)*reason=m;return false;};
    if(!valid||stale||sourceRevision==0) return fail("environment sample is invalid or stale");
    if(!std::isfinite(sampledAtSeconds)||sampledAtSeconds<0) return fail("invalid sample timestamp");
    auto finite=[](const std::optional<double>& v){return !v||std::isfinite(*v);};
    if(!finite(temperatureC)||!finite(wetness01)||!finite(light01)||!finite(waterDepthM)||
       !finite(undergroundDepthM)||!finite(shelterHeatC)) return fail("non-finite environment channel");
    if(wetness01&&(*wetness01<0||*wetness01>1)) return fail("wetness must be in [0,1]");
    if(light01&&(*light01<0||*light01>1)) return fail("light must be in [0,1]");
    if(waterDepthM&&*waterDepthM<0) return fail("water depth cannot be negative");
    if(undergroundDepthM&&*undergroundDepthM<0) return fail("underground depth cannot be negative");
    return true;
  }
};
} // namespace wm

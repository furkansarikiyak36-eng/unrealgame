#include "RegionSeasonProfile.h"

FRegionSeasonModifier URegionSeasonProfile::GetModifierForSeason(EWorldSeason Season) const
{
    for (const FRegionSeasonModifier& Modifier : SeasonalModifiers)
    {
        if (Modifier.Season == Season)
        {
            FRegionSeasonModifier Safe = Modifier;
            Safe.PrecipitationMultiplier = FMath::Clamp(Safe.PrecipitationMultiplier, 0.0f, 5.0f);
            Safe.WindMultiplier = FMath::Clamp(Safe.WindMultiplier, 0.0f, 5.0f);
            Safe.DaylightHours = FMath::Clamp(Safe.DaylightHours, 0.0f, 24.0f);
            return Safe;
        }
    }
    FRegionSeasonModifier Default;
    Default.Season = Season;
    return Default;
}

#include "RegionSeasonSubsystem.h"
#include "RegionSeasonProfile.h"
#include "WorldRegionVolume.h"
#include "SimCore/SimulationSubsystem.h"
#include "SimCore/SimulationState.h"
#include "Engine/World.h"

void URegionSeasonSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CurrentWorldDay = FMath::Max(1, InitialWorldDay);
    CurrentSeason = GetSeasonForWorldDay(CurrentWorldDay);

    if (UWorld* World = GetWorld())
    {
        if (USimulationSubsystem* Clock = World->GetSubsystem<USimulationSubsystem>())
        {
            if (USimulationState* State = Clock->GetState()) UpdateCalendar(State->DayNumber, false);
            Clock->OnDayTick.AddDynamic(this, &URegionSeasonSubsystem::HandleSimulationDayTick);
        }
    }
}

void URegionSeasonSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld())
        if (USimulationSubsystem* Clock = World->GetSubsystem<USimulationSubsystem>())
            Clock->OnDayTick.RemoveDynamic(this, &URegionSeasonSubsystem::HandleSimulationDayTick);
    RegisteredRegions.Reset();
    Super::Deinitialize();
}

int32 URegionSeasonSubsystem::GetCurrentDayOfYear() const
{
    const int32 YearLength = FMath::Max(4, DaysPerYear);
    return ((FMath::Max(1, CurrentWorldDay) - 1) % YearLength) + 1;
}

EWorldSeason URegionSeasonSubsystem::GetSeasonForWorldDay(int32 WorldDay) const
{
    const int32 YearLength = FMath::Max(4, DaysPerYear);
    const int32 SeasonLength = FMath::Max(1, FMath::Min(DaysPerSeason, YearLength / 4));
    const int32 DayOfYearZeroBased = (FMath::Max(1, WorldDay) - 1) % YearLength;
    return static_cast<EWorldSeason>(FMath::Clamp(DayOfYearZeroBased / SeasonLength, 0, 3));
}

void URegionSeasonSubsystem::HandleSimulationDayTick(int32 DayNumber)
{
    UpdateCalendar(DayNumber, true);
}

void URegionSeasonSubsystem::UpdateCalendar(int32 NewWorldDay, bool bBroadcast)
{
    const EWorldSeason Previous = CurrentSeason;
    CurrentWorldDay = FMath::Max(1, NewWorldDay);
    CurrentSeason = GetSeasonForWorldDay(CurrentWorldDay);
    if (!bBroadcast) return;
    if (Previous != CurrentSeason) OnSeasonChanged.Broadcast(Previous, CurrentSeason);
    OnSeasonDayAdvanced.Broadcast(GetCurrentDayOfYear(), CurrentSeason, CurrentWorldDay);
}

void URegionSeasonSubsystem::RegisterRegion(AWorldRegionVolume* Region)
{
    if (!IsValid(Region)) return;
    RegisteredRegions.RemoveAll([](const TObjectPtr<AWorldRegionVolume>& Entry) { return !IsValid(Entry.Get()); });
    if (!RegisteredRegions.Contains(Region)) RegisteredRegions.Add(Region);
}

void URegionSeasonSubsystem::UnregisterRegion(AWorldRegionVolume* Region)
{
    RegisteredRegions.Remove(Region);
    RegisteredRegions.RemoveAll([](const TObjectPtr<AWorldRegionVolume>& Entry) { return !IsValid(Entry.Get()); });
}

AWorldRegionVolume* URegionSeasonSubsystem::GetRegionAtLocation(const FVector& WorldLocation) const
{
    AWorldRegionVolume* Best = nullptr;
    for (const TObjectPtr<AWorldRegionVolume>& Entry : RegisteredRegions)
    {
        AWorldRegionVolume* Region = Entry.Get();
        if (!IsValid(Region) || !Region->ContainsWorldPoint(WorldLocation)) continue;
        if (!Best || Region->Priority > Best->Priority
            || (Region->Priority == Best->Priority && Region->GetPathName() < Best->GetPathName()))
        {
            Best = Region;
        }
    }
    return Best;
}

FRegionClimateSnapshot URegionSeasonSubsystem::GetClimateAtLocation(const FVector& WorldLocation) const
{
    FRegionClimateSnapshot Snapshot;
    Snapshot.Season = CurrentSeason;
    const AWorldRegionVolume* Region = GetRegionAtLocation(WorldLocation);
    if (!IsValid(Region)) return Snapshot;

    Snapshot.bHasKnownRegion = true;
    Snapshot.RegionId = Region->RegionId;
    Snapshot.RegionDisplayName = Region->DisplayName;
    Snapshot.RegionType = Region->RegionType;

    const URegionSeasonProfile* Profile = Region->ClimateProfile;
    if (!Profile) return Snapshot;

    Snapshot.RegionId = Profile->RegionId.IsNone() ? Region->RegionId : Profile->RegionId;
    Snapshot.RegionDisplayName = Profile->DisplayName.IsEmpty() ? Region->DisplayName : Profile->DisplayName;
    Snapshot.RegionType = Profile->RegionType == EWorldRegionType::Unknown ? Region->RegionType : Profile->RegionType;

    const FRegionSeasonModifier Modifier = Profile->GetModifierForSeason(CurrentSeason);
    Snapshot.ElevationMeters = Profile->ElevationMeters;
    Snapshot.MeanAnnualRainfallMm = Profile->MeanAnnualRainfallMm;
    Snapshot.BaselineTemperatureC = Profile->BaselineTemperatureC - (Profile->ElevationMeters * 0.0065f);
    Snapshot.TemperatureOffsetC = Modifier.TemperatureOffsetC;
    Snapshot.PrecipitationMultiplier = Modifier.PrecipitationMultiplier;
    Snapshot.WindMultiplier = FMath::Clamp(Profile->BaselineWindMultiplier * Modifier.WindMultiplier, 0.0f, 10.0f);
    Snapshot.SnowThresholdOffsetC = Modifier.SnowThresholdOffsetC;
    Snapshot.DaylightHours = Modifier.DaylightHours;
    return Snapshot;
}

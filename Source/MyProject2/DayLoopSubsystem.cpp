// Copyright Epic Games, Inc. All Rights Reserved.

#include "DayLoopSubsystem.h"
#include "Campfire.h"
#include "CarvableLog.h"
#include "RaidEnemy.h"
#include "StickStructure.h"
#include "WorldFeatures.h"
#include "Components/LightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "SimCore/SimulationState.h"
#include "SimCore/SimulationSubsystem.h"

namespace
{
	constexpr float ShelterBonusC = 8.f;
	constexpr float ShelterHeightCm = 600.f;
	constexpr float FireReachCm = 500.f;
	constexpr float FireWarmthC = 28.f;
}

UDayLoopSubsystem* UDayLoopSubsystem::Get(const UWorld* World)
{
	return World ? World->GetSubsystem<UDayLoopSubsystem>() : nullptr;
}

bool UDayLoopSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

USimulationSubsystem* UDayLoopSubsystem::GetClock() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<USimulationSubsystem>() : nullptr;
}

int32 UDayLoopSubsystem::GetDay() const
{
	const USimulationSubsystem* Clock = GetClock();
	return Clock && Clock->GetState() ? Clock->GetState()->DayNumber : 1;
}

float UDayLoopSubsystem::GetHour() const
{
	const USimulationSubsystem* Clock = GetClock();
	return Clock && Clock->GetState() ? Clock->GetState()->TimeOfDay : 12.f;
}

bool UDayLoopSubsystem::IsNight() const
{
	const float Hour = GetHour();
	return Hour < DawnHour || Hour >= DuskHour;
}

int32 UDayLoopSubsystem::GetNextRaidDay() const
{
	const int32 Day = GetDay();
	const bool bTonightPassed = IsRaidDay(Day) && GetHour() >= RaidStartHour;
	int32 Next = Day + (bTonightPassed ? 1 : 0);
	while (!IsRaidDay(Next))
	{
		++Next;
	}
	return Next;
}

FString UDayLoopSubsystem::RegionNameAt(const FVector& Where)
{
	return Where.X < -1200.f ? TEXT("Kayalık") : (Where.X > 1200.f ? TEXT("Nehir kıyısı") : TEXT("Orman"));
}

float UDayLoopSubsystem::RegionOffsetAt(const FVector& Where)
{
	return Where.X < -1200.f ? -4.f : (Where.X > 1200.f ? -2.f : 0.f);
}

FTemperatureReading UDayLoopSubsystem::GetTemperatureAt(const FVector& Where) const
{
	FTemperatureReading Reading;
	float Low = 15.f;
	float High = 25.f;
	if (const USimulationSubsystem* Clock = GetClock(); Clock && Clock->GetState())
	{
		Low = Clock->GetState()->TempMin;
		High = Clock->GetState()->TempMax;
	}
	// Coldest at midnight, warmest at noon (the same curve as the simulation's weather).
	const float Phase = GetHour() / 24.f * 2.f * PI;
	Reading.Air = FMath::Lerp(Low, High, 0.5f + 0.5f * FMath::Sin(Phase - PI * 0.5f)) + ClimateOffsetC + RegionOffsetAt(Where);

	UWorld* World = GetWorld();
	if (!World)
	{
		return Reading;
	}
	// In the river: wet.
	if (ARiverStrip::IsInWater(World, Where - FVector(0.f, 0.f, 90.f)))
	{
		Reading.Wet = -ARiverStrip::WetColdC;
	}
	// Shelter: something built overhead (a roof, a floor above).
	FHitResult Roof;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DayLoopShelter), false);
	if (World->LineTraceSingleByChannel(Roof, Where + FVector(0.f, 0.f, 100.f), Where + FVector(0.f, 0.f, ShelterHeightCm), ECC_Visibility, Params)
		&& Roof.GetActor() && Roof.GetActor()->IsA<ACarvableLog>())
	{
		Reading.Shelter = ShelterBonusC;
	}
	// Fire: the warmest one in reach counts, falling off with distance.
	auto Warmth = [&Where](const FVector& At) { return FireWarmthC * FMath::Clamp(1.f - FVector::Dist(Where, At) / FireReachCm, 0.f, 1.f); };
	for (TActorIterator<ACampfire> It(World); It; ++It)
	{
		if (It->IsBurning())
		{
			Reading.Fire = FMath::Max(Reading.Fire, Warmth(It->GetActorLocation()));
		}
	}
	for (TActorIterator<AStickStructure> It(World); It; ++It)
	{
		if (It->IsLitTorch())
		{
			Reading.Fire = FMath::Max(Reading.Fire, AStickStructure::TorchWarmthC * FMath::Clamp(1.f - FVector::Dist(Where, It->GetActorLocation()) / AStickStructure::TorchReachCm, 0.f, 1.f));
		}
	}
	for (TActorIterator<ACarvableLog> It(World); It; ++It)
	{
		if (It->IsBurning())
		{
			Reading.Fire = FMath::Max(Reading.Fire, Warmth(It->GetActorLocation()));
		}
	}
	return Reading;
}

void UDayLoopSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// The first day starts in the morning, not at midnight.
	if (USimulationSubsystem* Clock = GetClock(); Clock && Clock->GetState() && Clock->GetState()->DayNumber <= 1 && Clock->GetState()->TimeOfDay < 1.f)
	{
		Clock->GetState()->TimeOfDay = 8.f;
		Clock->GetState()->HourOfDay = 8;
	}
}

void UDayLoopSubsystem::SetTimeForTest(int32 Day, float Hour)
{
	if (USimulationSubsystem* Clock = GetClock(); Clock && Clock->GetState())
	{
		Clock->GetState()->DayNumber = FMath::Max(1, Day);
		Clock->GetState()->TimeOfDay = FMath::Fmod(FMath::Max(0.f, Hour), 24.f);
		Clock->GetState()->HourOfDay = FMath::FloorToInt(Clock->GetState()->TimeOfDay);
	}
	UpdateRules();
}

void UDayLoopSubsystem::Tick(float DeltaSeconds)
{
	UpdateRules();
	UpdateSun();
}

void UDayLoopSubsystem::Announce(const FString& Text)
{
	OnAnnouncement.Broadcast(Text);
}

void UDayLoopSubsystem::UpdateRules()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const int32 Day = GetDay();
	const float Hour = GetHour();
	// Warning at dusk of a raid day.
	if (IsRaidDay(Day) && Hour >= 18.f && Hour < RaidStartHour && WarnedDay != Day)
	{
		WarnedDay = Day;
		Announce(FString::Printf(TEXT("Gün %d: bu gece baskın var (22:00). Ateşi besle, duvarları kontrol et."), Day));
	}
	// The raid at 22:00, once per raid night.
	if (IsRaidDay(Day) && Hour >= RaidStartHour && StartedDay != Day)
	{
		StartedDay = Day;
		if (RaidKit::StartRaid(World, *FString::Printf(TEXT("Night%d"), Day), RaidSizeForDay(Day), RaidAtOnceForDay(Day)))
		{
			RaidDay = Day;
			Announce(FString::Printf(TEXT("Baskın başladı: %d düşman geliyor"), RaidSizeForDay(Day)));
		}
	}
	// Dawn: the raiders that are left go away.
	if (RaidDay > 0 && Day > RaidDay && Hour >= DawnHour)
	{
		RaidKit::StopRaid(World);
		Announce(FString::Printf(TEXT("Şafak: %d. gece atlatıldı"), RaidDay));
		RaidDay = 0;
	}
}

void UDayLoopSubsystem::UpdateSun()
{
	if (!Sun.IsValid())
	{
		for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
		{
			Sun = *It;
			break;
		}
		if (!Sun.IsValid())
		{
			return;
		}
	}
	ULightComponent* Light = Sun->GetLightComponent();
	if (!Light || Light->Mobility != EComponentMobility::Movable)
	{
		// A static sun cannot move; the clock still runs.
		return;
	}
	if (SunIntensity < 0.f)
	{
		SunIntensity = Light->Intensity;
	}
	// Overhead at noon, under the horizon at night.
	const float Hour = GetHour();
	Sun->SetActorRotation(FRotator(-90.f + (Hour - 12.f) * 15.f, 30.f, 0.f));
	Light->SetIntensity(IsNight() ? SunIntensity * 0.05f : SunIntensity);
}

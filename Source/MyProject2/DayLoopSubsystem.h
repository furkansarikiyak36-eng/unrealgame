// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DayLoopSubsystem.generated.h"

class ADirectionalLight;
class USimulationSubsystem;

/** The air a player feels at a spot, and what made it so. */
struct FTemperatureReading
{
	/** The day's weather at this hour, shifted by the cold climate and the region. */
	float Air = 0.f;
	/** A roof overhead keeps some cold off. */
	float Shelter = 0.f;
	/** A fire nearby: a lit fire pit or burning wood. */
	float Fire = 0.f;
	/** Standing in water: wet clothes take the warmth away. */
	float Wet = 0.f;
	float Total() const { return Air + Shelter + Fire + Wet; }
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnDayLoopAnnouncement, const FString&);

/**
 * The day the player has to get through (Frostpunk, The Long Dark): it reads the shared simulation clock (day, hour, the
 * day's low and high), makes the climate cold, moves the sun, says how warm a spot is (region, shelter, fires) and sends
 * the raiders on raid nights (7 Days to Die): every third night at 22:00, more of them each time, gone at dawn.
 */
UCLASS()
class MYPROJECT2_API UDayLoopSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UDayLoopSubsystem* Get(const UWorld* World);

	/** The cold climate: the simulation's mild weather shifted down by this much. */
	static constexpr float ClimateOffsetC = -14.f;
	static constexpr int32 RaidEveryNights = 3;
	static constexpr float RaidStartHour = 22.f;
	static constexpr float DawnHour = 6.f;
	static constexpr float DuskHour = 20.f;

	int32 GetDay() const;
	float GetHour() const;
	bool IsNight() const;
	/** Night of a raid: the day number is a multiple of RaidEveryNights. */
	bool IsRaidDay(int32 Day) const { return Day > 0 && Day % RaidEveryNights == 0; }
	int32 GetNextRaidDay() const;

	/** Region names and how much colder (or warmer) each is: rocks colder and windier, the river damp. */
	static FString RegionNameAt(const FVector& Where);
	static float RegionOffsetAt(const FVector& Where);

	FTemperatureReading GetTemperatureAt(const FVector& Where) const;

	/** Raiders of a raid night: three plus one per day survived; more at once as the days go by. */
	static int32 RaidSizeForDay(int32 Day) { return 3 + Day; }
	static int32 RaidAtOnceForDay(int32 Day) { return 2 + Day / 3; }

	/** Messages for the player: a raid coming, dawn after a raid. */
	FOnDayLoopAnnouncement OnAnnouncement;

	/** For the mechanics test: puts the clock at Hour of Day, then runs the day's rules once. */
	void SetTimeForTest(int32 Day, float Hour);
	/** For the mechanics test: the raid of the night that is running, if any. */
	bool IsRaidRunning() const { return RaidDay > 0; }

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UDayLoopSubsystem, STATGROUP_Tickables); }
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	USimulationSubsystem* GetClock() const;
	void UpdateRules();
	void UpdateSun();
	void Announce(const FString& Text);

	TWeakObjectPtr<ADirectionalLight> Sun;
	float SunIntensity = -1.f;
	/** Day whose raid is running (0: none), the day a warning was given, the day the raid was started. */
	int32 RaidDay = 0;
	int32 WarnedDay = 0;
	int32 StartedDay = 0;
};

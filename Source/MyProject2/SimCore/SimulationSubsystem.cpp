#include "SimulationSubsystem.h"
#include "Engine/World.h"
void USimulationSubsystem::Initialize(FSubsystemCollectionBase& Collection){Super::Initialize(Collection); State=NewObject<USimulationState>(this); if(State) Random.SetSeed(uint32(State->RandomSeed)); TickRates.Reset();}
void USimulationSubsystem::Deinitialize(){State=nullptr; Super::Deinitialize();}
TStatId USimulationSubsystem::GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(USimulationSubsystem,STATGROUP_Tickables); }
void USimulationSubsystem::SetRandomSeed(int32 Seed){Random.SetSeed(uint32(Seed)); if(State) State->RandomSeed=Seed;}
float USimulationSubsystem::RandomRange(float Min,float Max){return Random.Range(Min,Max);}
int32 USimulationSubsystem::RandomRangeInt(int32 Min,int32 Max){return Random.Range(Min,Max);}
bool USimulationSubsystem::RandomRoll(float Probability){return Random.Roll(Probability);}
void USimulationSubsystem::Tick(float DeltaTime){Super::Tick(DeltaTime); if(!State||DeltaTime<=0.f)return; TickRates.FrameAccumulator+=DeltaTime; TickRates.HourAccumulator+=DeltaTime; TickRates.DayAccumulator+=DeltaTime;
 const float SecPerHour=FMath::Max(0.01f,TickRates.SecondsPerHour); const float HoursPerDay=FMath::Max(1.f,TickRates.HoursPerDay); const float SecPerDay=SecPerHour*HoursPerDay;
 while(TickRates.HourAccumulator>=SecPerHour){TickRates.HourAccumulator-=SecPerHour; AdvanceOneHour();}
 while(TickRates.DayAccumulator>=SecPerDay){TickRates.DayAccumulator-=SecPerDay; AdvanceOneDay();}
 State->TimeOfDay=FMath::Fmod(State->TimeOfDay+DeltaTime/SecPerHour,24.f); UpdateWeather(); }
void USimulationSubsystem::AdvanceOneHour(){if(!State)return; State->HourOfDay=(State->HourOfDay+1)%FMath::Max(1,int32(TickRates.HoursPerDay)); if(OnHourTick.IsBound())OnHourTick.Broadcast(State->HourOfDay);}
void USimulationSubsystem::AdvanceOneDay(){if(!State)return; ++State->DayNumber; TickRates.WeekAccumulator+=1.f;
 FDeterministicRandom DailyRng(uint32(State->RandomSeed)^uint32(State->DayNumber*7919)); const float R=DailyRng.NextFloat(); bool bStormWind=false;
 State->Rainfall=0.f; State->bIsRaining=false; State->bIsSnowing=false;
 if(R>=.75f&&R<.90f){State->Rainfall=DailyRng.Range(3.f,15.f);State->bIsRaining=true;}
 else if(R>=.90f&&R<.98f){State->Rainfall=DailyRng.Range(15.f,40.f);State->bIsRaining=true;State->WindSpeed=DailyRng.Range(15.f,45.f);bStormWind=true;}
 else if(R>=.98f){State->Rainfall=DailyRng.Range(2.f,10.f);State->bIsSnowing=true;}
 State->TempMax=DailyRng.Range(10.f,32.f);State->TempMin=State->TempMax-DailyRng.Range(5.f,15.f);
 if(!bStormWind) State->WindSpeed=DailyRng.Range(0.f,12.f);
 const float WindAngle=DailyRng.Range(0.f,2.f*PI);
 State->WindDirection=FVector2D(FMath::Cos(WindAngle),FMath::Sin(WindAngle)).GetSafeNormal();
 if(State->WindDirection.IsNearlyZero()) State->WindDirection=FVector2D(1.f,0.f);
 if(TickRates.WeekAccumulator>=FMath::Max(1.f,TickRates.DaysPerWeek))TickRates.WeekAccumulator-=FMath::Max(1.f,TickRates.DaysPerWeek);
 UpdateWeather(); if(OnDayTick.IsBound())OnDayTick.Broadcast(State->DayNumber); }
void USimulationSubsystem::AdvanceDays(int32 Days){if(!State||Days<=0)return; for(int32 I=0;I<Days;++I)AdvanceOneDay();}
void USimulationSubsystem::UpdateWeather(){if(!State)return; const float Phase=State->TimeOfDay/24.f*2.f*PI; const float NewTemp=FMath::Lerp(State->TempMin,State->TempMax,0.5f+0.5f*FMath::Sin(Phase-PI/2.f)); if(FMath::Abs(NewTemp-State->Temperature)>0.1f){State->Temperature=NewTemp;if(OnWeatherChanged.IsBound())OnWeatherChanged.Broadcast(NewTemp);} }

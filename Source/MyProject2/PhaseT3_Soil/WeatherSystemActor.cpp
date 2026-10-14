#include "WeatherSystemActor.h"
#include "SimCore/DeterministicRandom.h"
AWeatherSystemActor::AWeatherSystemActor(){PrimaryActorTick.bCanEverTick=false;}
void AWeatherSystemActor::RollDailyWeather(int32 Day,int32 Seed){FDeterministicRandom R(uint32(Seed)^uint32(Day*7919));float X=R.NextFloat();if(X<.5f){CurrentWeather=EWeatherType::Clear;Rainfall=0.f;}else if(X<.75f){CurrentWeather=EWeatherType::Cloudy;Rainfall=0.f;}else if(X<.9f){CurrentWeather=EWeatherType::Rainy;Rainfall=R.Range(3.f,15.f);}else if(X<.98f){CurrentWeather=EWeatherType::Stormy;Rainfall=R.Range(15.f,40.f);}else{CurrentWeather=EWeatherType::Snowy;Rainfall=R.Range(2.f,10.f);}TempMax=R.Range(10.f,32.f);TempMin=TempMax-R.Range(5.f,15.f);}

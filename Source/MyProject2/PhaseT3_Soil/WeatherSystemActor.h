#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeatherSystemActor.generated.h"
UENUM(BlueprintType) enum class EWeatherType:uint8{Clear,Cloudy,Rainy,Stormy,Snowy};
UCLASS() class MYPROJECT2_API AWeatherSystemActor:public AActor{GENERATED_BODY()
public: AWeatherSystemActor(); UFUNCTION(BlueprintCallable,Category="Weather") void RollDailyWeather(int32 DayNumber,int32 Seed); UFUNCTION(BlueprintPure,Category="Weather") EWeatherType GetWeather()const{return CurrentWeather;} UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) EWeatherType CurrentWeather=EWeatherType::Clear; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float Rainfall=0.f; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float TempMax=25.f; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float TempMin=15.f;};

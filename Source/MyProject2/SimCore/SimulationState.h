#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SimulationTypes.h"
#include "SimulationState.generated.h"
UCLASS(BlueprintType)
class MYPROJECT2_API USimulationState : public UObject { GENERATED_BODY()
public:
 UPROPERTY(BlueprintReadOnly, Category="Time") int32 DayNumber=1;
 UPROPERTY(BlueprintReadOnly, Category="Time") int32 HourOfDay=0;
 UPROPERTY(BlueprintReadOnly, Category="Time") float TimeOfDay=0.f;
 UPROPERTY(BlueprintReadOnly, Category="Weather") float Temperature=20.f;
 UPROPERTY(BlueprintReadOnly, Category="Weather") float TempMin=15.f;
 UPROPERTY(BlueprintReadOnly, Category="Weather") float TempMax=25.f;
 UPROPERTY(BlueprintReadOnly, Category="Weather") float Rainfall=0.f;
 UPROPERTY(BlueprintReadOnly, Category="Weather") bool bIsRaining=false;
 UPROPERTY(BlueprintReadOnly, Category="Weather") bool bIsSnowing=false;
 UPROPERTY(BlueprintReadOnly, Category="Weather") float WindSpeed=0.f;
 UPROPERTY(BlueprintReadOnly, Category="RNG") int32 RandomSeed=12345;
 UFUNCTION(BlueprintCallable, Category="Simulation") void Reset();
};

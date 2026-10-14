#pragma once
#include "CoreMinimal.h"
#include "SimulationTickRates.generated.h"
USTRUCT(BlueprintType) struct FSimulationTickRates { GENERATED_BODY()
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float FrameAccumulator=0.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float HourAccumulator=0.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float DayAccumulator=0.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float WeekAccumulator=0.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Time Scale", meta=(ClampMin="0.01")) float SecondsPerHour=60.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Time Scale", meta=(ClampMin="1")) float HoursPerDay=24.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Time Scale", meta=(ClampMin="1")) float DaysPerWeek=7.f;
 void Reset(){FrameAccumulator=HourAccumulator=DayAccumulator=WeekAccumulator=0.f;}
};

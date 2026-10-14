#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "SimulationTypes.h"
#include "SimulationState.h"
#include "SimulationTickRates.h"
#include "DeterministicRandom.h"
#include "SimulationSubsystem.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSimOnDayTick,int32,DayNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSimOnHourTick,int32,HourNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSimOnWeatherChanged,float,NewTemp);
UCLASS()
class MYPROJECT2_API USimulationSubsystem : public UTickableWorldSubsystem { GENERATED_BODY()
public:
 virtual void Initialize(FSubsystemCollectionBase& Collection) override;
 virtual void Deinitialize() override;
 virtual void Tick(float DeltaTime) override;
 virtual TStatId GetStatId() const override;
 virtual bool IsTickable() const override { return !IsTemplate(); }
 UFUNCTION(BlueprintCallable,Category="Simulation") void AdvanceDays(int32 Days);
 UFUNCTION(BlueprintPure,Category="Simulation") USimulationState* GetState() const{return State;}
 // C++ only: a native reference is deliberately not exposed to Blueprint reflection.
 FDeterministicRandom& GetRandom(){return Random;}
 UFUNCTION(BlueprintCallable,Category="Simulation") void SetRandomSeed(int32 Seed);
 UFUNCTION(BlueprintCallable,Category="Simulation") float RandomRange(float Min,float Max);
 UFUNCTION(BlueprintCallable,Category="Simulation") int32 RandomRangeInt(int32 Min,int32 Max);
 UFUNCTION(BlueprintCallable,Category="Simulation") bool RandomRoll(float Probability);
 UPROPERTY(BlueprintAssignable,Category="Events") FSimOnDayTick OnDayTick;
 UPROPERTY(BlueprintAssignable,Category="Events") FSimOnHourTick OnHourTick;
 UPROPERTY(BlueprintAssignable,Category="Events") FSimOnWeatherChanged OnWeatherChanged;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Configuration") FSimulationTickRates TickRates;
protected:
 UPROPERTY() TObjectPtr<USimulationState> State=nullptr;
 FDeterministicRandom Random;
 void AdvanceOneHour(); void AdvanceOneDay(); void UpdateWeather();
};

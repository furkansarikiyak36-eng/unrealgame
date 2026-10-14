#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RaidTypes.h"
#include "RaidWaveStateMachine.generated.h"
UCLASS() class MYPROJECT2_API URaidWaveStateMachine : public UObject { GENERATED_BODY() public: UPROPERTY(BlueprintReadWrite) EWaveState CurrentState=EWaveState::Preparation; UPROPERTY(BlueprintReadWrite) int32 CurrentWaveIndex=0; UPROPERTY(BlueprintReadWrite) int32 TotalWaves=0; UPROPERTY(BlueprintReadWrite) float WaveStartTime=0.f; UPROPERTY(BlueprintReadWrite) float IntermissionStartTime=0.f; UPROPERTY(BlueprintReadWrite) float IntermissionDuration=10.f; void Initialize(int32 InTotalWaves); void Tick(float CurrentTime,int32 RemainingBudget,int32 LiveCount,float WaveDuration); void TransitionTo(EWaveState NewState); UFUNCTION(BlueprintPure) bool IsComplete() const{return CurrentState==EWaveState::Completed;} };

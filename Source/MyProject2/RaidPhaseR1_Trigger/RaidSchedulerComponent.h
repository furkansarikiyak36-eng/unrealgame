#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RaidTypes.h"
#include "RaidSchedulerComponent.generated.h"
UCLASS(ClassGroup=(Raid),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API URaidSchedulerComponent : public UActorComponent { GENERATED_BODY() public: URaidSchedulerComponent(); UFUNCTION(BlueprintCallable) void ScheduleNextEvent(float BaseTime,float InJitterMin,float InJitterMax); UFUNCTION(BlueprintPure) float GetNextEventTime() const{return NextEventTime;} UFUNCTION(BlueprintPure) bool IsEventDue(float CurrentTime) const{return CurrentTime>=NextEventTime;} UFUNCTION(BlueprintCallable) void ResetSchedule(); UPROPERTY(EditAnywhere,BlueprintReadWrite) float BaseInterval=1800.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float JitterMin=-300.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float JitterMax=300.f; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float NextEventTime=0.f; };

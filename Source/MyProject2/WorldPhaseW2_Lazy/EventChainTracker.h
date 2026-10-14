#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WorldTypes.h"
#include "EventChainTracker.generated.h"
UCLASS()
class MYPROJECT2_API UEventChainTracker : public UObject
{
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintPure,Category="Chain") bool HasInterruptingEvent(const FIntPoint& Coord,float SinceDay) const;
 UFUNCTION(BlueprintCallable,Category="Chain") void RegisterEvent(const FWorldEvent& Event,float CurrentDay);
 UFUNCTION(BlueprintCallable,Category="Chain") void ClearOldEvents(float CurrentDay,float MaxAgeDays);
protected:
 struct FTrackedEvent { FWorldEvent Event; float RegisteredDay=0; };
 TArray<FTrackedEvent> TrackedEvents;
};

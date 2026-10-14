#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WorldTypes.h"
#include "WorldEvent.generated.h"
UCLASS()
class MYPROJECT2_API UWorldEventLibrary : public UObject
{
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable,Category="Event") static FWorldEvent CreateEvent(EWorldEventType Type,const FVector& Location,float Radius,float Magnitude,FName Source);
 UFUNCTION(BlueprintPure,Category="Event") static bool IsInRadius(const FWorldEvent& Event,const FVector& Location);
};

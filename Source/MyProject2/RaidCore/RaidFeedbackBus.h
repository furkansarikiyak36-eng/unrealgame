#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RaidTypes.h"
#include "RaidFeedbackBus.generated.h"
USTRUCT(BlueprintType) struct FRaidFeedbackEvent { GENERATED_BODY() UPROPERTY(BlueprintReadWrite) ERaidEventType Type=ERaidEventType::None; UPROPERTY(BlueprintReadWrite) FName SourceModule; UPROPERTY(BlueprintReadWrite) FName TargetModule; UPROPERTY(BlueprintReadWrite) TMap<FName,float> NumericData; UPROPERTY(BlueprintReadWrite) TMap<FName,TObjectPtr<AActor>> ActorData; UPROPERTY(BlueprintReadWrite) double Timestamp=0.0; };
DECLARE_DELEGATE_OneParam(FOnFeedbackEvent, const FRaidFeedbackEvent&);
UCLASS() class MYPROJECT2_API URaidFeedbackBus : public UObject { GENERATED_BODY() public: void Publish(FName EventTag, const FRaidFeedbackEvent& Event); void Subscribe(FName EventTag, FOnFeedbackEvent Callback); void Unsubscribe(FName EventTag); private: TMap<FName,FOnFeedbackEvent> Subscribers; };

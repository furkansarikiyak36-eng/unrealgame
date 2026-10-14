#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RaidTypes.h"
#include "RaidPriorityArbiter.generated.h"
USTRUCT() struct FRaidRequest { GENERATED_BODY() ERaidRequestType Type=ERaidRequestType::None; float Priority=0.f; double SubmitTime=0.0; };
UCLASS() class MYPROJECT2_API URaidPriorityArbiter : public UObject { GENERATED_BODY() public: void SubmitRequest(ERaidRequestType Type,float Priority); UFUNCTION(BlueprintCallable) ERaidRequestType GetNextRequest(); protected: TArray<FRaidRequest> PendingRequests; };

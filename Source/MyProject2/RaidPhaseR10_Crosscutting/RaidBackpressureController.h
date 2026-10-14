#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RaidBackpressureController.generated.h"
UCLASS() class MYPROJECT2_API URaidBackpressureController : public UObject { GENERATED_BODY() public: UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 MaxPendingRequests=50; UFUNCTION(BlueprintPure) bool CanAcceptRequest(int32 CurrentPending) const; UFUNCTION(BlueprintCallable) void SignalOverload(); UFUNCTION(BlueprintCallable) void ClearOverload(); UFUNCTION(BlueprintPure) bool IsOverloaded() const{return bOverloaded;} protected: bool bOverloaded=false; };

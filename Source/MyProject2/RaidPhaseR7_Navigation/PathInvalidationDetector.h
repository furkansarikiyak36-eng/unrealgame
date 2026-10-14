#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PathInvalidationDetector.generated.h"
UCLASS() class MYPROJECT2_API UPathInvalidationDetector : public UObject { GENERATED_BODY() public: UFUNCTION(BlueprintCallable) bool IsPathStillValid(UWorld* World,const TArray<FVector>& Path) const; UFUNCTION(BlueprintCallable) void RequestPathRecalculation(AActor* Entity); UFUNCTION(BlueprintCallable) bool ShouldRecalculate(AActor* Entity,float DeltaTime); protected: TSet<TWeakObjectPtr<AActor>> PendingRecalculation; };

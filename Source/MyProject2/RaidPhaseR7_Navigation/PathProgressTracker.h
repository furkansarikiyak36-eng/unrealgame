#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RaidTypes.h"
#include "PathProgressTracker.generated.h"
UCLASS() class MYPROJECT2_API UPathProgressTracker : public UObject { GENERATED_BODY() public: UPROPERTY(EditAnywhere,BlueprintReadWrite) float StuckThreshold=.5f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float StuckDistance=20.f; UFUNCTION(BlueprintCallable) bool IsStuck(AActor* Entity,float DeltaTime); UFUNCTION(BlueprintCallable) EAlternativeAction ChooseAlternativeAction(AActor* Entity); protected: TMap<TWeakObjectPtr<AActor>,FVector> LastPositions; TMap<TWeakObjectPtr<AActor>,float> StuckTimers; };

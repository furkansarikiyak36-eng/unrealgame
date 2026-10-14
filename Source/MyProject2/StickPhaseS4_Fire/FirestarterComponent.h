#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FirestarterComponent.generated.h"
UCLASS(ClassGroup=(Fire),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UFirestarterComponent:public UActorComponent{GENERATED_BODY() public: UFirestarterComponent(); UFUNCTION(BlueprintCallable) bool StrikeStone(const FVector& Location); UFUNCTION(BlueprintCallable) bool RubSticks(const FVector& Location); UFUNCTION(BlueprintCallable) bool UseFlint(const FVector& Location); UPROPERTY(EditAnywhere,BlueprintReadWrite) float IgnitionChance=.6f; UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 AttemptsRequired=3; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) int32 AttemptCount=0;};

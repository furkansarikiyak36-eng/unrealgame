#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StickFlexComponent.generated.h"
UCLASS(ClassGroup=(Stick),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UStickFlexComponent:public UActorComponent{GENERATED_BODY() public: UStickFlexComponent(); UFUNCTION(BlueprintCallable) float ComputeBendAmount(float AppliedForce)const; UFUNCTION(BlueprintCallable) bool WillSnap(float AppliedForce)const; UFUNCTION(BlueprintCallable) FVector ComputeTipOffset(float AppliedForce)const; UPROPERTY(EditAnywhere,BlueprintReadWrite) float Flexibility=.3f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float SnapThreshold=1;};

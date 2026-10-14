#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZiplineActor.generated.h"
class USplineComponent; class ALogActor;
UCLASS() class MYPROJECT2_API AZiplineActor:public AActor
{GENERATED_BODY() public: AZiplineActor(); virtual void Tick(float DeltaSeconds) override; UFUNCTION(BlueprintCallable) bool AttachLog(ALogActor* Log); UFUNCTION(BlueprintCallable) void ReleaseAtEnd(); UPROPERTY(VisibleAnywhere) TObjectPtr<USplineComponent> Cable; UPROPERTY(EditAnywhere,BlueprintReadWrite) float SlideSpeed=400.0f; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) TObjectPtr<ALogActor> AttachedLog; protected: float DistanceOnSpline=0.0f; };

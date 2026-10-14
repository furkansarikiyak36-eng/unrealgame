#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LogPlacementComponent.generated.h"
class ALogActor;
UCLASS(ClassGroup=(Log),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API ULogPlacementComponent:public UActorComponent
{GENERATED_BODY() public: ULogPlacementComponent(); UFUNCTION(BlueprintCallable) void EnterAimMode(); UFUNCTION(BlueprintCallable) void ExitAimMode(); UFUNCTION(BlueprintCallable) void CycleSnap(); UFUNCTION(BlueprintCallable) void RotateLog(float Delta); UFUNCTION(BlueprintCallable) bool ConfirmPlacement(); UPROPERTY(EditAnywhere,BlueprintReadWrite) float ReachDistance=250.0f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float SnapRadius=20.0f; protected: UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) bool bAiming=false; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) TObjectPtr<ALogActor> PreviewLog=nullptr; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) FTransform TargetTransform=FTransform::Identity; virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction) override; };

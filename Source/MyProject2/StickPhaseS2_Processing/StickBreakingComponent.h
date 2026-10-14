#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StickBreakingComponent.generated.h"
class AStickActor;
UCLASS(ClassGroup=(Stick),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UStickBreakingComponent:public UActorComponent{GENERATED_BODY() public: UStickBreakingComponent(); UFUNCTION(BlueprintCallable) bool BeginBreak(AStickActor* Stick); UFUNCTION(BlueprintCallable) bool CompleteBreak(); UFUNCTION(BlueprintCallable) bool CancelBreak(); UPROPERTY(EditAnywhere,BlueprintReadWrite) float BreakDuration=1.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 PiecesPerBreak=2; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) bool bIsBreaking=false; protected: UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) TObjectPtr<AStickActor> BreakingStick=nullptr; float BreakProgress=0; virtual void TickComponent(float,ELevelTick,FActorComponentTickFunction*)override;};

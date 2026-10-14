#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RaidContext.h"
#include "RaidLifecycleComponent.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLifecycleChanged,ERaidLifecycleState,NewState);
UCLASS(ClassGroup=(Raid),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API URaidLifecycleComponent : public UActorComponent { GENERATED_BODY() public: URaidLifecycleComponent(); UFUNCTION(BlueprintCallable) void BeginEvent(FRaidContext& Context); UFUNCTION(BlueprintCallable) void EndEvent(FRaidContext& Context); UFUNCTION(BlueprintCallable) void CancelEvent(FRaidContext& Context,FName Reason); UFUNCTION(BlueprintCallable) void OnPlayerDeath(FRaidContext& Context,int32 PlayerId); UFUNCTION(BlueprintCallable) void OnServerShutdown(FRaidContext& Context); UFUNCTION(BlueprintPure) ERaidLifecycleState GetCurrentPhase() const{return CurrentPhase;} UPROPERTY(BlueprintAssignable) FOnLifecycleChanged OnLifecycleChanged; protected: UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) ERaidLifecycleState CurrentPhase=ERaidLifecycleState::Idle; void TransitionTo(ERaidLifecycleState NewState); };

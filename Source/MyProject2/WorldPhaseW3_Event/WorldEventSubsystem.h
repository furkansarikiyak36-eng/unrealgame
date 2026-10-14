#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldTypes.h"
#include "EventChain.h"
#include "WorldEventResponder.h"
#include "WorldEventSubsystem.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldEvent,const FWorldEvent&,Event);
UCLASS()
class MYPROJECT2_API UWorldEventSubsystem : public UTickableWorldSubsystem
{
 GENERATED_BODY()
public:
 virtual void Initialize(FSubsystemCollectionBase& Collection) override;
 virtual void Tick(float DeltaTime) override;
 virtual TStatId GetStatId() const override;
 UFUNCTION(BlueprintCallable,Category="Event") void BroadcastEvent(const FWorldEvent& Event);
 UFUNCTION(BlueprintCallable,Category="Event") void RegisterResponder(TScriptInterface<IWorldEventResponder> Responder);
 UFUNCTION(BlueprintCallable,Category="Event") void UnregisterResponder(TScriptInterface<IWorldEventResponder> Responder);
 UPROPERTY(BlueprintAssignable,Category="Event") FOnWorldEvent OnWorldEventBroadcast;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 MaxEventsPerFrame=16;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 MaxChainDepth=5;
protected:
 UPROPERTY() TArray<TScriptInterface<IWorldEventResponder>> Responders;
 UPROPERTY() TArray<FWorldEvent> PendingEvents;
 UPROPERTY() TArray<TObjectPtr<UEventChain>> ActiveChains;
 void ProcessEvent(const FWorldEvent& Event,int32 Depth);
};

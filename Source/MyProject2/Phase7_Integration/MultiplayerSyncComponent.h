#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LogTypes.h"
#include "MultiplayerSyncComponent.generated.h"
UCLASS(ClassGroup=(Net),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UMultiplayerSyncComponent:public UActorComponent
{GENERATED_BODY() public: UMultiplayerSyncComponent(); UFUNCTION(Server,Reliable) void ServerSetState(AActor* Log,ELogState NewState); UFUNCTION(NetMulticast,Reliable) void MulticastOnStateChanged(AActor* Log,ELogState NewState); };

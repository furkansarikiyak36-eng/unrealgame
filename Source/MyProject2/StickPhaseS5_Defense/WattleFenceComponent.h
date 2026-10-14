#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WattleFenceComponent.generated.h"
class AStickActor;
UCLASS(ClassGroup=(Defense),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UWattleFenceComponent:public UActorComponent{GENERATED_BODY() public: UWattleFenceComponent(); UFUNCTION(BlueprintCallable) bool BuildFence(const TArray<FVector>& PostPositions,int32 WeaveRows); UFUNCTION(BlueprintCallable) bool AddClayFill(AStickActor* FenceSection); UPROPERTY(EditAnywhere,BlueprintReadWrite) float PostHeight=120; UPROPERTY(EditAnywhere,BlueprintReadWrite) TSubclassOf<AStickActor> StickClass;};

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StickPathComponent.generated.h"
class AStickActor;
UCLASS(ClassGroup=(Environment),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UStickPathComponent:public UActorComponent{GENERATED_BODY() public: UStickPathComponent(); UFUNCTION(BlueprintCallable) bool BuildPath(const TArray<FVector>& Waypoints,float Width); UPROPERTY(EditAnywhere,BlueprintReadWrite) float StickSpacing=25; UPROPERTY(EditAnywhere,BlueprintReadWrite) TSubclassOf<AStickActor> StickClass;};

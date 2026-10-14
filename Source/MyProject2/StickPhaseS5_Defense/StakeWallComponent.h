#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StakeWallComponent.generated.h"
class AStakeActor;
UCLASS(ClassGroup=(Defense),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UStakeWallComponent:public UActorComponent{GENERATED_BODY() public: UStakeWallComponent(); UFUNCTION(BlueprintCallable) bool BuildStakeWall(const FVector& Start,const FVector& End,int32 StakeCount); UPROPERTY(EditAnywhere,BlueprintReadWrite) float StakeSpacing=30; UPROPERTY(EditAnywhere,BlueprintReadWrite) float StakeHeight=150; UPROPERTY(EditAnywhere,BlueprintReadWrite) TSubclassOf<AStakeActor> StakeClass; protected: UPROPERTY() TArray<TObjectPtr<AStakeActor>> Stakes;};

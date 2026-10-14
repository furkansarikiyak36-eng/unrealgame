#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StakeReinforcementComponent.generated.h"
class AStakeActor;
UCLASS(ClassGroup=(Defense),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UStakeReinforcementComponent:public UActorComponent{GENERATED_BODY() public: UStakeReinforcementComponent(); UFUNCTION(BlueprintCallable) bool ReinforceWithStones(AStakeActor* Stake,int32 StoneCount); UPROPERTY(EditAnywhere,BlueprintReadWrite) float StrengthBonusPerStone=.15f;};

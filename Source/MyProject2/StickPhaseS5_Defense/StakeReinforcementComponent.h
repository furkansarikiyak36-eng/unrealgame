#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StakeReinforcementComponent.generated.h"

class AStakeActor;
class AStoneActor;

UCLASS(ClassGroup = (Defense), meta = (BlueprintSpawnableComponent))
class MYPROJECT2_API UStakeReinforcementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStakeReinforcementComponent();

	UFUNCTION(BlueprintCallable, Category = "Defense")
	bool ReinforceWithStones(AStakeActor* Stake, const TArray<AStoneActor*>& Stones);

	/** Count-only compatibility path is disabled by default to prevent free reinforcement. */
	UFUNCTION(BlueprintCallable, Category = "Defense")
	bool ReinforceWithStones(AStakeActor* Stake, int32 StoneCount);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense", meta = (ClampMin = "0.0"))
	float StrengthBonusPerStone = 0.15f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense", meta = (ClampMin = "1", ClampMax = "8"))
	int32 StonesRequired = 3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense")
	bool bAllowCountOnlyReinforcement = false;
};

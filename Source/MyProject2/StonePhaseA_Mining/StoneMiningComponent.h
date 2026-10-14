#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StoneMiningComponent.generated.h"

class ARockBedActor;

UCLASS(ClassGroup=(Stone), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UStoneMiningComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UStoneMiningComponent();
    UFUNCTION(BlueprintCallable) bool MineRock(ARockBedActor* RockBed, float Strength);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float MiningRange = 200.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float MiningPower = 1.0f;
};

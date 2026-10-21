#pragma once

#include "CoreMinimal.h"
#include "StonePhaseF_Environment/StoneTrapActor.h"
#include "UndergroundTrapActor.generated.h"

/** A physical trap that only triggers when placed inside a real excavated cavity. */
UCLASS(BlueprintType, Blueprintable)
class MYPROJECT2_API AUndergroundTrapActor : public AStoneTrapActor
{
    GENERATED_BODY()

public:
    AUndergroundTrapActor();

    virtual void Trigger(AActor* Victim) override;

    UFUNCTION(BlueprintCallable, Category = "Underground|Traps")
    bool TriggerFromWaterPocket(AActor* Victim, float MinimumWaterVolumeM3 = 0.05f);

    UFUNCTION(BlueprintPure, Category = "Underground|Traps")
    bool HasValidCavityClearance() const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Underground|Traps", meta = (ClampMin = "25.0", ClampMax = "500.0"))
    float MinimumCavityClearanceCm = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Underground|Traps")
    bool bRequireCavityClearance = true;
};

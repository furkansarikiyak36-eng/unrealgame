#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StoneRubbleComponent.generated.h"
class AStoneActor;

UCLASS(ClassGroup=(Stone), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UStoneRubbleComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UStoneRubbleComponent();
    UFUNCTION(BlueprintCallable) int32 RecoverMaterial(AStoneActor* Rubble);
    UFUNCTION(BlueprintCallable) bool ReuseAsGravel(AStoneActor* Rubble);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float RecoveryRate = 0.7f;
};

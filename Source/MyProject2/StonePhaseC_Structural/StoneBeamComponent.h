#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StoneBeamComponent.generated.h"
class AStoneActor;

UCLASS(ClassGroup=(Stone), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UStoneBeamComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UStoneBeamComponent();
    UFUNCTION(BlueprintCallable) AStoneActor* BuildBeam(AStoneActor* LeftPillar, AStoneActor* RightPillar);
    UFUNCTION(BlueprintPure) bool IsBeamSupported(AStoneActor* Beam) const;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaxBeamSpan = 400.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSubclassOf<AStoneActor> StoneClass;
};

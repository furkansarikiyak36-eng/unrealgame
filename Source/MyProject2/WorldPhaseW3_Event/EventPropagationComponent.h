#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WorldTypes.h"
#include "EventPropagationComponent.generated.h"
UCLASS(ClassGroup=(World),meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UEventPropagationComponent : public UActorComponent
{
 GENERATED_BODY()
public:
 UEventPropagationComponent();
 UFUNCTION(BlueprintCallable,Category="Propagation") void PropagateToNeighbors(const FWorldEvent& Event);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 MaxPropagationDepth=3;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float PropagationAttenuation=0.5f;
};

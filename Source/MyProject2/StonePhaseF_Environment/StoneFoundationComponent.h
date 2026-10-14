#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StoneFoundationComponent.generated.h"
class AStoneActor;
UCLASS(ClassGroup=(Stone), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UStoneFoundationComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UStoneFoundationComponent();
    UFUNCTION(BlueprintCallable) bool BuildFoundation(const FVector& Center, float Width, float Depth);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float SlopeTolerance = 0.3f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float StoneSpacing = 30.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSubclassOf<AStoneActor> StoneClass;
};

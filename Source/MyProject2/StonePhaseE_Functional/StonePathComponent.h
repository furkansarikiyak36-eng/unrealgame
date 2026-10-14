#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StonePathComponent.generated.h"
class AStoneActor;
UCLASS(ClassGroup=(Stone), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UStonePathComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UStonePathComponent();
    UFUNCTION(BlueprintCallable) bool BuildPath(const TArray<FVector>& Waypoints, float Width);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float StoneSpacing = 40.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSubclassOf<AStoneActor> StoneClass;
};

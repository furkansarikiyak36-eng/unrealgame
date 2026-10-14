#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StonePillarComponent.generated.h"
class AStoneActor;

UCLASS(ClassGroup=(Stone), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UStonePillarComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UStonePillarComponent();
    UFUNCTION(BlueprintCallable) AStoneActor* BuildPillar(const FVector& Base, float Height, int32 StoneCount);
    UFUNCTION(BlueprintCallable) bool IsStandalonePillar(AStoneActor* Pillar) const;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float StoneHeight = 20.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSubclassOf<AStoneActor> StoneClass;
};

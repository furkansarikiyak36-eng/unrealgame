#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StoneWallComponent.generated.h"
class AStoneActor;

UCLASS(ClassGroup=(Stone), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UStoneWallComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UStoneWallComponent();
    UFUNCTION(BlueprintCallable) bool BuildWall(const FVector& Start, const FVector& End, float Height, int32 Layers);
    UFUNCTION(BlueprintCallable) bool IncreaseThickness(AStoneActor* WallStone);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float StoneSpacing = 30.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float LayerHeight = 20.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSubclassOf<AStoneActor> StoneClass;
protected:
    UPROPERTY() TArray<TObjectPtr<AStoneActor>> WallStones;
};

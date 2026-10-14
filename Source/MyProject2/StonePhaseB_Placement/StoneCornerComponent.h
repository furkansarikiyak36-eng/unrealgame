#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StoneCornerComponent.generated.h"
class AStoneActor;

UCLASS(ClassGroup=(Stone), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UStoneCornerComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UStoneCornerComponent();
    UFUNCTION(BlueprintCallable) bool BuildCorner(const FVector& CornerLocation, const FVector& DirA, const FVector& DirB, int32 Layers);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSubclassOf<AStoneActor> StoneClass;
};
